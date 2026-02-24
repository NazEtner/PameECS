#pragma once
#include <xaudio2.h>
#include <wrl/client.h>
#include <xhash>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <memory>
#include <vector>
#include <array>
#include <mutex>
#include "callback.hpp"

namespace PameECS::Audio {
	// ハッシュ計算用のヘルパー
	struct WaveFormatHash {
		size_t operator()(const WAVEFORMATEXTENSIBLE& f) const {
			// 主要なパラメータを組み合わせてハッシュを作る
			size_t h = 0;
			auto hashCombine = [&](auto v) {
				h ^= std::hash<decltype(v)>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
				};
			hashCombine(f.Format.nChannels);
			hashCombine(f.Format.nSamplesPerSec);
			hashCombine(f.Format.wBitsPerSample);
			hashCombine(f.SubFormat.Data1); // GUIDの一部を利用
			return h;
		}
	};

	// 比較演算子の定義
	struct WaveFormatEqual {
		bool operator()(const WAVEFORMATEXTENSIBLE& lhs, const WAVEFORMATEXTENSIBLE& rhs) const {
			return std::memcmp(&lhs, &rhs, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
		}
	};

	class VoicePool {
	public:
		~VoicePool() {
			for (auto& pool : m_free_voices) {
				for (auto& voice : pool.second) {
					if (voice) voice->DestroyVoice();
					voice = nullptr;
				}
				pool.second.clear();
			}
			m_free_voices.clear();
		}

		IXAudio2SourceVoice* Acquire(IXAudio2* device, const WAVEFORMATEXTENSIBLE& format, IXAudio2VoiceCallback* callback) {
			assert(device);
			assert(callback);
			auto& list = m_free_voices[format];
			if (!list.empty()) {
				auto* voice = list.back();
				list.pop_back();
				return voice;
			}

			IXAudio2SourceVoice* newVoice = nullptr;
			device->CreateSourceVoice(&newVoice, &format.Format, 0, 2.0f, callback);

			return newVoice;
		}

		void Release(const WAVEFORMATEXTENSIBLE& format, IXAudio2SourceVoice* voice) {
			assert(voice);
			voice->Stop();
			voice->FlushSourceBuffers();
			m_free_voices[format].push_back(voice);
		}
	private:
		std::unordered_map<WAVEFORMATEXTENSIBLE, std::vector<IXAudio2SourceVoice*>, WaveFormatHash, WaveFormatEqual> m_free_voices;
	};

	class PlayerBackend {
	public:
		PlayerBackend();
		~PlayerBackend();
		IXAudio2SourceVoice* GetSourceVoice(size_t slotIndex) {
			if (slotIndex >= m_voice_slots.size()) return nullptr;
			return m_voice_slots[slotIndex].sourceVoice;
		}
		Callback* GetCallback(size_t slotIndex) {
			if (slotIndex >= m_voice_slots.size()) return nullptr;
			return m_voice_slots[slotIndex].callback.get();
		}
		IXAudio2MasteringVoice* GetMasteringVoice() {
			return m_master_voice;
		}
		void Register(size_t slotIndex, Callback* callback, void(*deleter)(Callback*));
		bool Start(size_t slotIndex);
		void Stop(size_t slotIndex);
	private:
		void m_fillAndSubmit(size_t slotIndex);

		class VoiceCallbackHandler : public IXAudio2VoiceCallback {
			PlayerBackend* m_backend;
		public:
			explicit VoiceCallbackHandler(PlayerBackend* backend) : m_backend(backend) {}

			// バッファが終了したときに呼ばれる
			void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override {
				size_t slotIndex = reinterpret_cast<size_t>(pBufferContext) - 1;

				// バックエンドに次のデータを要求する
				m_backend->m_fillAndSubmit(slotIndex);
			}

			// 他のメソッド（OnStreamEnd等）は空実装
			void STDMETHODCALLTYPE OnStreamEnd() override {}
			void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
			void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
			void STDMETHODCALLTYPE OnBufferStart(void*) override {}
			void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
			void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
		};

		struct VoiceSlot {
			VoiceSlot() = default;
			VoiceSlot(VoiceSlot&&) noexcept = default;
			VoiceSlot& operator=(VoiceSlot&&) noexcept = default;

			IXAudio2SourceVoice* sourceVoice = nullptr;
			std::unique_ptr<Callback, void(*)(Callback*)> callback = { nullptr, nullptr };
			std::array<std::vector<uint8_t>, 2> soundBuffer;
			bool stopped = false;
			size_t currentBuffer = 0;
		};

		VoiceCallbackHandler m_voice_callback_handler;
		Microsoft::WRL::ComPtr<IXAudio2> m_xaudio2;
		IXAudio2MasteringVoice* m_master_voice = nullptr;
		std::vector<VoiceSlot> m_voice_slots;
		VoicePool m_voice_pool;
	};
}
