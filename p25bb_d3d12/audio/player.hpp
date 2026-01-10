#pragma once
#include "pcm_entry.hpp"
#include "file_entry.hpp"
#include "../macros/dll.hpp"
#include "callback.hpp"
#include <memory>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

namespace PameECS::Audio {
	class Player;
}

extern "C" {
	PECS_DLL_SHARED size_t AudioPlayerGetVoiceHandle(PameECS::Audio::Player* player);
	PECS_DLL_SHARED void AudioPlayerReleaseVoiceHandle(PameECS::Audio::Player* player, size_t voice);
	PECS_DLL_SHARED void AudioPlayerSubmitPCM(PameECS::Audio::Player* player, size_t voice, const PameECS::Audio::PCMEntry* entry);
	PECS_DLL_SHARED void AudioPlayerSubmitFile(PameECS::Audio::Player* player, size_t voice, const PameECS::Audio::FileEntry* entry, bool useCallback = true);
	PECS_DLL_SHARED void AudioPlayerSubmitCallback(PameECS::Audio::Player* player, size_t voice, size_t(*callback)(void* userData, uint8_t* dest, size_t samples), const WAVEFORMATEXTENSIBLE& format, void* userData);
	PECS_DLL_SHARED void AudioPlayerSubmitCallbackObject(PameECS::Audio::Player* player, size_t voiceHandle, PameECS::Audio::Callback* callback, void(*deleter)(PameECS::Audio::Callback*));
	PECS_DLL_SHARED void AudioPlayerPlay(PameECS::Audio::Player* player, size_t voiceHandle);
	PECS_DLL_SHARED void AudioPlayerPause(PameECS::Audio::Player* player, size_t voiceHandle);
	PECS_DLL_SHARED void AudioPlayerStop(PameECS::Audio::Player* player, size_t voiceHandle);
	PECS_DLL_SHARED void AudioPlayerSetVolume(PameECS::Audio::Player* player, size_t voiceHandle, float volume);
	PECS_DLL_SHARED size_t AudioPlayerGetQueuedVoiceCount(PameECS::Audio::Player* player, size_t voiceHandle);
	PECS_DLL_SHARED uint32_t AudioPlayerGetOutputChannels(PameECS::Audio::Player* player);
	PECS_DLL_SHARED uint32_t AudioPlayerGetVoiceChannels(PameECS::Audio::Player* player, size_t voiceHandle);
	PECS_DLL_SHARED void AudioPlayerSetOutputMatrix(PameECS::Audio::Player* player, size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix);
}

namespace PameECS::Audio {
	class Player {
	public:
		Player();
		~Player();
		size_t GetVoiceHandle();
		void ReleaseVoiceHandle(size_t voiceHandle);
		void Submit(size_t voiceHandle, const PCMEntry* entry);
		void Submit(size_t voiceHandle, const FileEntry* entry, bool useCallback = true);
		void Submit(size_t voiceHandle, size_t(*callback)(void* userData, uint8_t* dest, size_t samples), const WAVEFORMATEXTENSIBLE& format, void* userData);

		void Submit(size_t voiceHandle, Callback* callback, void(*deleter)(Callback*));

		template <typename T, typename... Args>
			requires(std::is_base_of_v<Callback, T>)
		void Submit(size_t voiceHandle, Args... args) {
			AudioPlayerSubmitCallbackObject(
				this,
				voiceHandle,
				new T(args...),
				[](Callback* cb) -> void { delete cb; }
			);
		}

		void Play(size_t voiceHandle);
		void Pause(size_t voiceHandle);
		void Stop(size_t voiceHandle);
		void SetVolume(size_t voiceHandle, float volume);
		size_t GetQueuedVoiceCount(size_t voiceHandle);
		uint32_t GetOutputChannels();
		uint32_t GetVoiceChannels(size_t voiceHandle);
		void SetOutputMatrix(size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix);

		void ShowDebug();
	private:
		// プラットフォーム依存なので、純粋なpImpl(プライベートメソッドもここには書かない)
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
