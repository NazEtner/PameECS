#include "player.hpp"
#include "../exceptions/audio_error.hpp"
#include "../helpers/container.hpp"
#include "../file/file.hpp"
#include <mutex>
#include <queue>
#include <vector>
#include <stack>
#include <xaudio2.h>
#include <wrl/client.h>
#include <dr_libs/dr_wav.h>
#include <dr_libs/dr_flac.h>
#include <ks.h>
#include <ksmedia.h>

namespace {
	struct VoiceSlot {
		VoiceSlot(IXAudio2* xaudio, const WAVEFORMATEX& waveFormat) {
			Initialize(xaudio, waveFormat);
		}

		~VoiceSlot() {
			Finalize();
		}

		void Initialize(IXAudio2* xaudio, const WAVEFORMATEX& waveFormat) {
			if (!sourceVoice || memcmp(&this->waveFormat, &waveFormat, sizeof(WAVEFORMATEX)) != 0) {
				Finalize();
				xaudio->CreateSourceVoice(&sourceVoice, &waveFormat);
				this->waveFormat = waveFormat;
			}
		}

		void Finalize() {
			Clear();
			if (sourceVoice) {
				sourceVoice->DestroyVoice();
			}
		}
		constexpr static size_t CHUNK_SIZE = 64 * 1024;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		std::queue<std::vector<uint8_t>> bufferQueue;
		std::mutex queueMutex;
		bool inUse = false;
		WAVEFORMATEX waveFormat;

		void Clear() {
			if (sourceVoice) {
				sourceVoice->Stop();
				sourceVoice->FlushSourceBuffers();
			}
			std::lock_guard<std::mutex> lock(queueMutex);
			while (!bufferQueue.empty()) bufferQueue.pop();
		}
	};

	uint64_t LoadWav(std::string& fileName, std::vector<float>& audioBuffer, WAVEFORMATEXTENSIBLE* waveFormatEx) {
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

	uint64_t LoadFlac(std::string& fileName, std::vector<float>& audioBuffer, WAVEFORMATEXTENSIBLE* waveFormatEx) {
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
		XAudio2Create(xaudio2.GetAddressOf());
	}
	~Impl() {

	}
	Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
	std::vector<std::unique_ptr<VoiceSlot>> voiceSlots;
	std::stack<size_t> emptySlots;
	size_t lastSlotIndex = 0;
};

Player::Player() {
	m_impl = std::make_unique<Impl>();
}

Player::~Player() {

}

void Player::Update() {

}

size_t Player::GetVoiceHandle() {
	if (!m_impl->emptySlots.empty()) {
		auto ret = m_impl->emptySlots.top();
		m_impl->emptySlots.pop();
		return ret;
	}

	auto ret = m_impl->lastSlotIndex++;
	if (ret >= m_impl->voiceSlots.size()) {
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

}

void Player::Submit(size_t voiceHandle, const FileEntry* entry) {

}

uint32_t Player::GetOutputChannels() {
	return 0;
}

uint32_t Player::GetVoiceChannels(size_t voiceHandle) {
	return 0;
}

void Player::SetOutputMatrix(size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, const float* matrix) {

}
