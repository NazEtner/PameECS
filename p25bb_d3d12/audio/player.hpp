#pragma once
#include "pcm_entry.hpp"
#include "file_entry.hpp"
#include <memory>

namespace PameECS::Audio {
	class Player {
	public:
		Player();
		~Player();
		size_t GetVoiceHandle();
		void ReleaseVoiceHandle(size_t voiceHandle);
		void Submit(size_t voiceHandle, const PCMEntry* entry);
		void Submit(size_t voiceHandle, const FileEntry* entry);
		void Play(size_t voiceHandle);
		void Pause(size_t voiceHandle);
		void Stop(size_t voiceHandle);
		void SetVolume(size_t voiceHandle, float volume);
		size_t GetQueuedVoiceCount(size_t voiceHandle);
		uint32_t GetOutputChannels();
		uint32_t GetVoiceChannels(size_t voiceHandle);
		void SetOutputMatrix(size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix);
	private:
		// プラットフォーム依存なので、純粋なpImpl(プライベートメソッドもここには書かない)
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
