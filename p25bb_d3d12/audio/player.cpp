#include "player.hpp"
#include "../exceptions/audio_error.hpp"
#include "../helpers/container.hpp"
#include "../file/file.hpp"
#include "callbacks/long_flac.hpp"
#include "callbacks/long_wav.hpp"
#include "callbacks/pcm.hpp"
#include "callbacks/flac.hpp"
#include "callbacks/wav.hpp"
#include "player_backend.hpp"
#include <mutex>
#include <queue>
#include <vector>
#include <stack>
#include <imgui/imgui.h>
#include <unordered_set>

namespace {
	WAVEFORMATEXTENSIBLE FillFormat(uint16_t channels, uint16_t bits, uint32_t rate, bool isFloat) {
		WAVEFORMATEXTENSIBLE format;
		ZeroMemory(&format, sizeof(format));
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.nChannels = channels;
		format.Format.nSamplesPerSec = rate;
		format.Format.wBitsPerSample = bits;
		format.Format.nBlockAlign = (channels * bits) / 8;
		format.Format.nAvgBytesPerSec = rate * format.Format.nBlockAlign;
		format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		format.Samples.wValidBitsPerSample = bits;
		format.SubFormat = isFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;

		return format;
	}
}

using PameECS::Audio::Player;

struct Player::Impl {
	std::mutex mutex;

	std::queue<size_t> emptySlots;
	size_t nextSlot = 0;
	PlayerBackend playerBackend;
};

Player::Player() : m_impl(std::make_unique<Impl>()) {}

Player::~Player() {

}

size_t Player::GetVoiceHandle() {
	std::lock_guard<std::mutex> lock(m_impl->mutex);

	size_t handle;
	if (!m_impl->emptySlots.empty()) {
		// 再利用可能なスロットがある場合
		handle = m_impl->emptySlots.front();
		m_impl->emptySlots.pop();
	}
	else {
		// 新規スロットを発行
		handle = m_impl->nextSlot++;
	}

	return handle;
}

void Player::ReleaseVoiceHandle(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);

	m_impl->playerBackend.Stop(voiceHandle);
	m_impl->emptySlots.push(voiceHandle);
}

void Player::Submit(size_t voiceHandle, const PCMEntry* entry) {
	if (!entry) return;
	std::lock_guard<std::mutex> lock(m_impl->mutex);

	Submit<Callbacks::PCM>(voiceHandle, *entry);
}

void Player::Submit(size_t voiceHandle, const FileEntry* entry, bool useCallback) {
	if (!entry) return;
	std::error_code ec;
	auto filename = std::string(entry->fileName, entry->nameSize);
	if (useCallback && std::filesystem::is_regular_file(filename, ec)) {
		std::optional<size_t> loopPos;
		if (entry->loopEnable) loopPos = entry->loopStart;
		switch (entry->codec) {
		case FileEntry::Codec::FLAC:
			Submit<Callbacks::LongFlac>(voiceHandle, filename, loopPos, entry->holdBuffer);
			break;
		case FileEntry::Codec::WAV:
			Submit<Callbacks::LongWav>(voiceHandle, filename, loopPos, entry->holdBuffer);
			break;
		default: break;
		}
	}
	else {
		std::optional<size_t> loopPos;
		if (entry->loopEnable) loopPos = entry->loopStart;
		switch (entry->codec) {
		case FileEntry::Codec::FLAC:
			Submit<Callbacks::Flac>(voiceHandle, filename, loopPos, entry->holdBuffer);
			break;
		case FileEntry::Codec::WAV:
			Submit<Callbacks::Wav>(voiceHandle, filename, loopPos, entry->holdBuffer);
			break;
		default: break;
		}
	}
}

void Player::Submit(size_t voiceHandle, Callback* callback, void(*deleter)(Callback*)) {
	if (!callback) return;
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->playerBackend.Register(voiceHandle, callback, deleter);
}

bool Player::Play(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	return m_impl->playerBackend.Start(voiceHandle);
}

void Player::Pause(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	auto voice = m_impl->playerBackend.GetSourceVoice(voiceHandle);
	if (!voice) return;
	voice->Stop();
}

void Player::Stop(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->playerBackend.Stop(voiceHandle);
	auto callback = m_impl->playerBackend.GetCallback(voiceHandle);
	if (!callback) return;
	callback->Seek(0);
}

void Player::SetVolume(size_t voiceHandle, float volume) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	auto voice = m_impl->playerBackend.GetSourceVoice(voiceHandle);
	if (!voice) return;
	voice->SetVolume(volume);
}

uint32_t Player::GetOutputChannels() {
	auto master = m_impl->playerBackend.GetMasteringVoice();
	XAUDIO2_VOICE_DETAILS details;
	master->GetVoiceDetails(&details);
	return details.InputChannels;
}

uint32_t Player::GetVoiceChannels(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	auto callback = m_impl->playerBackend.GetCallback(voiceHandle);
	if (!callback) return 0;
	return callback->GetFormat().Format.nChannels;
}

void Player::SetOutputMatrix(size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix) {
	if (!matrix) return;
	std::lock_guard<std::mutex> lock(m_impl->mutex);

	auto voice = m_impl->playerBackend.GetSourceVoice(voiceHandle);
	if (!voice) return;
	voice->SetOutputMatrix(nullptr, sourceChannels, destChannels, matrix);
}

void Player::ShowDebug() {
	if (ImGui::TreeNodeEx("Quick Submitter", ImGuiTreeNodeFlags_DefaultOpen)) {
		static char filePath[256] = "";
		static bool useCallback = true;
		static int loopPosition = 0;
		static bool loopEnabled = false;

		ImGui::InputText("Source Path", filePath, IM_ARRAYSIZE(filePath));
		ImGui::Checkbox("Streaming (Callback)", &useCallback);
		ImGui::Checkbox("Loop enabled", &loopEnabled);
		if (loopEnabled) {
			ImGui::InputInt("Loop Position", &loopPosition);
		}

		if (ImGui::Button("Submit & Play New Handle")) {
			if (filePath[0] != '\0') {
				size_t h = this->GetVoiceHandle();
				FileEntry entry;
				entry.fileName = filePath;
				entry.nameSize = static_cast<uint32_t>(strlen(filePath));
				entry.loopEnable = loopEnabled;
				entry.loopStart = static_cast<size_t>(loopPosition);
				entry.codec = std::string(filePath).ends_with(".flac") ? FileEntry::Codec::FLAC : FileEntry::Codec::WAV;

				this->Submit(h, &entry, useCallback);
				this->Play(h); // ここで失敗してもハンドル状態として監視し続ける
			}
		}
		ImGui::TreePop();
	}

	ImGui::Separator();

	// --- ボイスリスト：実態（Voice）の有無を問わず「スロット」として表示 ---
	ImGui::TextDisabled("Registered Voice Slots");

	std::unordered_set<size_t> pooledIds;
	auto tempQueue = m_impl->emptySlots;
	while (!tempQueue.empty()) {
		pooledIds.insert(tempQueue.front());
		tempQueue.pop();
	}

	for (size_t i = 0; i < m_impl->nextSlot; ++i) {
		if (pooledIds.contains(i)) continue;

		auto callback = m_impl->playerBackend.GetCallback(i);
		auto voice = m_impl->playerBackend.GetSourceVoice(i);

		// コールバックすら無いスロットは完全に空（未使用）なのでスキップ
		if (!callback) continue;

		ImGui::PushID(static_cast<int>(i));

		// ラベル構成：ボイス実態の有無をステータスに含める
		char label[128];
		const char* voiceStatus = voice ? "READY" : "NOT_INSTANTIATED";
		snprintf(label, sizeof(label), "Handle %zu [%s]", i, voiceStatus);

		if (ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label)) {

			// --- 情報表示 ---
			auto f = callback->GetFormat();
			ImGui::BulletText("Format: %uHz %uch", f.Format.nSamplesPerSec, f.Format.nChannels);

			if (voice) {
				XAUDIO2_VOICE_STATE state;
				voice->GetState(&state);
				ImGui::BulletText("Buffers Queued: %u", state.BuffersQueued);

				float vol;
				voice->GetVolume(&vol);
				if (ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f)) voice->SetVolume(vol);
			}
			else {
				ImGui::TextDisabled("(Voice will be created on Play)");
			}

			// --- 操作：ボイスの有無に関わらず常に可能 ---
			// Play() を呼べば内部で SourceVoice が生成される挙動に合わせる
			if (ImGui::Button("Play")) {
				this->Play(i);
			}
			ImGui::SameLine();
			if (ImGui::Button("Pause")) {
				this->Pause(i);
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				this->Stop(i);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Release")) {
				this->ReleaseVoiceHandle(i);
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

extern "C" {
	PECS_DLL_SHARED size_t AudioPlayerGetVoiceHandle(PameECS::Audio::Player* player) {
		return player->GetVoiceHandle();
	}
	PECS_DLL_SHARED void AudioPlayerReleaseVoiceHandle(PameECS::Audio::Player* player, size_t voice) {
		player->ReleaseVoiceHandle(voice);
	}
	PECS_DLL_SHARED void AudioPlayerSubmitPCM(PameECS::Audio::Player* player, size_t voice, const PameECS::Audio::PCMEntry* entry) {
		player->Submit(voice, entry);
	}
	PECS_DLL_SHARED void AudioPlayerSubmitFile(PameECS::Audio::Player* player, size_t voice, const PameECS::Audio::FileEntry* entry, bool useCallback) {
		player->Submit(voice, entry, useCallback);
	}
	PECS_DLL_SHARED void AudioPlayerSubmitCallbackObject(PameECS::Audio::Player* player, size_t voiceHandle, PameECS::Audio::Callback* callback, void(*deleter)(PameECS::Audio::Callback*)) {
		player->Submit(voiceHandle, callback, deleter);
	}
	PECS_DLL_SHARED bool AudioPlayerPlay(PameECS::Audio::Player* player, size_t voiceHandle) {
		return player->Play(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerPause(PameECS::Audio::Player* player, size_t voiceHandle) {
		player->Pause(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerStop(PameECS::Audio::Player* player, size_t voiceHandle) {
		player->Stop(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerSetVolume(PameECS::Audio::Player* player, size_t voiceHandle, float volume) {
		player->SetVolume(voiceHandle, volume);
	}
	PECS_DLL_SHARED uint32_t AudioPlayerGetOutputChannels(PameECS::Audio::Player* player) {
		return player->GetOutputChannels();
	}
	PECS_DLL_SHARED uint32_t AudioPlayerGetVoiceChannels(PameECS::Audio::Player* player, size_t voiceHandle) {
		return player->GetVoiceChannels(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerSetOutputMatrix(PameECS::Audio::Player* player, size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix) {
		player->SetOutputMatrix(voiceHandle, sourceChannels, destChannels, matrix);
	}
}
