#include "player.hpp"
#include "../exceptions/audio_error.hpp"
#include "../helpers/container.hpp"
#include "../file/file.hpp"
#include <mutex>
#include <queue>
#include <vector>
#include <stack>
#include <format>
#include <thread>
#include <chrono>
#include <xaudio2.h>
#include <wrl/client.h>
#include <dr_libs/dr_wav.h>
#include <dr_libs/dr_flac.h>
#include <ks.h>
#include <ksmedia.h>

namespace {
	struct VoiceSlot {
		VoiceSlot(IXAudio2* xaudio, const WAVEFORMATEXTENSIBLE* waveFormat) {
			Initialize(xaudio, waveFormat);
		}

		~VoiceSlot() {
			Finalize();
		}

		void Initialize(IXAudio2* xaudio, const WAVEFORMATEXTENSIBLE* waveFormat) {
			if (!sourceVoice || memcmp(&this->waveFormat, waveFormat, sizeof(WAVEFORMATEX)) != 0) {
				Finalize();
				auto hr = xaudio->CreateSourceVoice(&sourceVoice, reinterpret_cast<const WAVEFORMATEX*>(waveFormat));
				if (FAILED(hr)) {
					throw PameECS::Exceptions::AudioError(std::format("Failed to create source voice: waveFormat = {}", static_cast<const void*>(waveFormat)));
				}
				this->waveFormat = *waveFormat;
				loop = false;
				loopStart = 0;
			}
		}

		void Finalize() {
			Clear();
			if (sourceVoice) {
				sourceVoice->DestroyVoice();
			}
		}

		constexpr static size_t chunkSize = 64 * 1024;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		std::queue<std::vector<uint8_t>> bufferQueue;
		std::mutex mutex;
		std::atomic<bool> inUse = false;
		WAVEFORMATEXTENSIBLE waveFormat;
		std::queue<std::vector<uint8_t>> bufferQueueCopy;
		bool holdBuffer = true;
		bool loop = false;
		size_t samplesProcessed = 0;
		size_t loopStart = 0;
		size_t audioEnd = 0;
		std::array<std::vector<uint8_t>, 2> playing;
		size_t primary = 0;
		size_t secondary = 1;

		void Clear() {
			if (sourceVoice) {
				sourceVoice->Stop();
				sourceVoice->FlushSourceBuffers();
			}
			std::lock_guard<std::mutex> lock(mutex);
			while (!bufferQueue.empty()) bufferQueue.pop();
			audioEnd = 0;
		}
	};

	uint64_t LoadWav(const std::string& fileName, std::vector<float>& audioBuffer, WAVEFORMATEXTENSIBLE* waveFormatEx) {
		PameECS::File::File<0, 0> file;
		auto ss = file.LoadSync(fileName);
		std::vector<uint8_t> fileBinary((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());
		drwav wav;
		if (!drwav_init_memory(&wav, fileBinary.data(), fileBinary.size(), nullptr)) {
			return 0;
		}
		const size_t dataSize = wav.totalPCMFrameCount * wav.channels;
		audioBuffer.resize(dataSize, 0);
		uint64_t framesRead = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, audioBuffer.data());

		auto& waveFormat = waveFormatEx->Format;

		waveFormat.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		waveFormat.nChannels = wav.channels;
		waveFormat.nSamplesPerSec = wav.sampleRate;
		waveFormat.wBitsPerSample = 32;
		waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

		waveFormatEx->Samples.wValidBitsPerSample = 32;
		// XAudio2が自動でチャンネルマスクを設定するようにする
		// 詳細は以下を参照
		// https://learn.microsoft.com/ja-jp/windows/win32/xaudio2/xaudio2-default-channel-mapping
		waveFormatEx->dwChannelMask = 0;

		waveFormatEx->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

		drwav_uninit(&wav);

		return framesRead;
	}

	uint64_t LoadFlac(const std::string& fileName, std::vector<float>& audioBuffer, WAVEFORMATEXTENSIBLE* waveFormatEx) {
		PameECS::File::File<0, 0> file;
		auto ss = file.LoadSync(fileName);
		std::vector<uint8_t> fileBinary((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());

		drflac_uint64 totalPCMFrameCount = 0;
		unsigned int channels = 0;
		unsigned int sampleRate = 0;

		float* pcmData = drflac_open_memory_and_read_pcm_frames_f32(
			fileBinary.data(), fileBinary.size(),
			&channels, &sampleRate, &totalPCMFrameCount, nullptr
		);

		if (!pcmData) {
			return 0;
		}

		audioBuffer.resize(totalPCMFrameCount * channels);
		memcpy(audioBuffer.data(), pcmData, totalPCMFrameCount * channels * sizeof(float));

		drflac_free(pcmData, nullptr);

		auto& waveFormat = waveFormatEx->Format;

		waveFormat.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		waveFormat.nChannels = channels;
		waveFormat.nSamplesPerSec = sampleRate;
		waveFormat.wBitsPerSample = 32;
		waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

		waveFormatEx->Samples.wValidBitsPerSample = 32;
		// XAudio2が自動でチャンネルマスクを設定するようにする
		// 詳細は以下を参照
		// https://learn.microsoft.com/ja-jp/windows/win32/xaudio2/xaudio2-default-channel-mapping
		waveFormatEx->dwChannelMask = 0;

		waveFormatEx->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

		return totalPCMFrameCount;
	}
}

using PameECS::Audio::Player;

struct Player::Impl {
	Impl() {
		auto result = XAudio2Create(xaudio2.GetAddressOf());
		if (FAILED(result)) {
			throw Exceptions::AudioError("Failed to create XAudio2 object.");
		}

		result = xaudio2->CreateMasteringVoice(&masterVoice);
		if (FAILED(result)) {
			throw Exceptions::AudioError("Failed to create mastering voice");
		}
		running = true;
		worker = std::thread([this]() -> void {UpdateVoices(); });
	}
	~Impl() {
		running = false;
		if (worker.joinable()) worker.join();
		if (masterVoice) {
			masterVoice->DestroyVoice();
			masterVoice = nullptr;
		}
	}

	// マルチスレッドで動かす物(スレッドプールは使わない)
	void UpdateVoices();
	std::atomic<bool> running;
	std::thread worker;
	Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice* masterVoice;
	std::mutex mutex;
	std::vector<std::unique_ptr<VoiceSlot>> voiceSlots;
	std::stack<size_t> emptySlots;
	size_t lastSlotIndex = 0;
};

void Player::Impl::UpdateVoices() {
	while (running) {
		{
			// このロック本当に必要？
			std::lock_guard lock(mutex);
			for (auto& slot : voiceSlots) {
				if (!slot || !slot->inUse || !slot->sourceVoice) continue;
				XAUDIO2_VOICE_STATE state;
				slot->sourceVoice->GetState(&state);
				if (state.BuffersQueued < 2) {
					std::lock_guard lock(slot->mutex);
					if (!slot->bufferQueue.empty()) {
						auto nextChunk = std::move(slot->bufferQueue.front());
						slot->bufferQueue.pop();

						slot->playing[slot->secondary] = nextChunk;
						XAUDIO2_BUFFER buffer = { 0 };
						buffer.AudioBytes = static_cast<UINT32>(slot->playing[slot->secondary].size());
						buffer.pAudioData = slot->playing[slot->secondary].data();
						slot->sourceVoice->SubmitSourceBuffer(&buffer);

						auto bufferEndPos = slot->samplesProcessed + nextChunk.size() / slot->waveFormat.Format.nBlockAlign;

						if (slot->loop && slot->loopStart >= slot->samplesProcessed && slot->loopStart < bufferEndPos) {
							auto eraseBytes = (slot->loopStart - slot->samplesProcessed) * slot->waveFormat.Format.nBlockAlign;
							auto loopBuf = std::vector<uint8_t>(nextChunk.data() + eraseBytes, nextChunk.data() + nextChunk.size());
							slot->bufferQueue.push(loopBuf);
						}

						slot->samplesProcessed = bufferEndPos;
						if (slot->samplesProcessed >= slot->audioEnd) {
							slot->samplesProcessed = slot->loopStart + slot->samplesProcessed - slot->audioEnd;
						}
					}
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

Player::Player() {
	m_impl = std::make_unique<Impl>();
}

Player::~Player() {

}

size_t Player::GetVoiceHandle() {
	if (!m_impl->emptySlots.empty()) {
		auto ret = m_impl->emptySlots.top();
		m_impl->emptySlots.pop();
		return ret;
	}

	auto ret = m_impl->lastSlotIndex++;
	if (ret >= m_impl->voiceSlots.size()) {
		std::lock_guard<std::mutex> lock(m_impl->mutex);
		Helpers::Container::ResizePow2<std::vector<std::unique_ptr<VoiceSlot>>>(m_impl->voiceSlots, ret);
	}
	return ret;
}

void Player::ReleaseVoiceHandle(size_t voiceHandle) {
	if (voiceHandle < m_impl->voiceSlots.size()) {
		m_impl->voiceSlots[voiceHandle] = nullptr;
		m_impl->emptySlots.push(voiceHandle);
	}
}

void Player::Submit(size_t voiceHandle, const PCMEntry* entry) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !entry) {
		return;
	}

	auto& slot = m_impl->voiceSlots[voiceHandle];

	if (!slot) {
		slot = std::make_unique<VoiceSlot>(m_impl->xaudio2.Get(), &entry->waveFormat);
	}
	else {
		slot->Initialize(m_impl->xaudio2.Get(), &entry->waveFormat);
	}

	const uint8_t* source = entry->data;
	size_t remaining = entry->dataSize;
	size_t samples = entry->dataSize / slot->waveFormat.Format.nBlockAlign;

	{
		std::lock_guard<std::mutex> lock(slot->mutex);
		while (remaining > 0) {
			size_t chunkSize = std::min(remaining, VoiceSlot::chunkSize);
			auto buffer = std::vector<uint8_t>(source, source + chunkSize);
			slot->bufferQueue.push(std::move(buffer));

			source += chunkSize;
			remaining -= chunkSize;
		}

		slot->loop = entry->loopEnable;
		slot->loopStart = entry->loopStart;
		slot->inUse = true;
		slot->audioEnd = samples;
		slot->holdBuffer = !entry->loopEnable ? entry->holdBuffer : false;

		if (slot->holdBuffer) {
			slot->bufferQueueCopy = slot->bufferQueue;
		}
		else {
			slot->bufferQueueCopy = {};
		}
	}
}

void Player::Submit(size_t voiceHandle, const FileEntry* entry) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !entry) {
		return;
	}

	PCMEntry pcmEntry = {};
	std::vector<float> pcmData;
	std::string fileName = std::string(entry->fileName, entry->nameSize);
	switch (entry->codec) {
	case FileEntry::Codec::WAV:
		if (LoadWav(fileName, pcmData, &pcmEntry.waveFormat) == 0) return;
		break;
	case FileEntry::Codec::FLAC:
		if (LoadFlac(fileName, pcmData, &pcmEntry.waveFormat) == 0) return;
		break;
	default:
		return;
	}

	pcmEntry.data = reinterpret_cast<uint8_t*>(pcmData.data());
	pcmEntry.dataSize = pcmData.size() * sizeof(float);
	pcmEntry.loopEnable = entry->loopEnable;
	pcmEntry.loopStart = entry->loopStart;
	pcmEntry.holdBuffer = entry->holdBuffer;

	// 以下で呼ぶSubmitのオーバーロードでは必ずpcmDataをコピーすること
	Submit(voiceHandle, &pcmEntry);
}

void Player::Play(size_t voiceHandle) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle]) {
		return;
	}
	auto& voice = m_impl->voiceSlots[voiceHandle];
	if (voice->inUse) {
		std::lock_guard<std::mutex> lock(voice->mutex);
		if (voice->holdBuffer) {
			voice->bufferQueue = voice->bufferQueueCopy;
			voice->sourceVoice->Stop();
			voice->sourceVoice->FlushSourceBuffers();
		}
		voice->sourceVoice->Start();
	}
}

void Player::Pause(size_t voiceHandle) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle] || !m_impl->voiceSlots[voiceHandle]->inUse) {
		return;
	}
	m_impl->voiceSlots[voiceHandle]->sourceVoice->Stop();
}

void Player::Stop(size_t voiceHandle) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle] || !m_impl->voiceSlots[voiceHandle]->inUse) {
		return;
	}
	m_impl->voiceSlots[voiceHandle]->Clear();
}

void Player::SetVolume(size_t voiceHandle, float volume) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle] || !m_impl->voiceSlots[voiceHandle]->inUse) {
		return;
	}
	m_impl->voiceSlots[voiceHandle]->sourceVoice->SetVolume(volume);
}

size_t Player::GetQueuedVoiceCount(size_t voiceHandle) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle] || !m_impl->voiceSlots[voiceHandle]->inUse) {
		return 0;
	}
	return m_impl->voiceSlots[voiceHandle]->bufferQueue.size();
}

uint32_t Player::GetOutputChannels() {
	DWORD mask = 0;
	auto result = m_impl->masterVoice->GetChannelMask(&mask);
	if (FAILED(result)) {
		throw Exceptions::AudioError("Failed to get channel mask.");
	}
	uint32_t count = 0;
	while (mask != 0) {
		if ((mask & 1) == 1) count++;
		mask >>= 1;
	}
	return count;
}

uint32_t Player::GetVoiceChannels(size_t voiceHandle) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle] || !m_impl->voiceSlots[voiceHandle]->inUse) {
		return 0;
	}
	XAUDIO2_VOICE_DETAILS details;
	m_impl->voiceSlots[voiceHandle]->sourceVoice->GetVoiceDetails(&details);
	return details.InputChannels;
}

void Player::SetOutputMatrix(size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix) {
	if (voiceHandle >= m_impl->voiceSlots.size() || !m_impl->voiceSlots[voiceHandle] || !m_impl->voiceSlots[voiceHandle]->inUse) {
		return;
	}
	m_impl->voiceSlots[voiceHandle]->sourceVoice->SetOutputMatrix(nullptr, sourceChannels, destChannels, matrix);
}
