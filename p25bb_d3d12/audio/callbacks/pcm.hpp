#pragma once
#include "../callback.hpp"
#include "../pcm_entry.hpp"
#include <vector>
#include <cstdint>
#include <optional>

namespace PameECS::Audio::Callbacks {
	class PCM : public Callback {
	public:
		PCM(const PCMEntry& pcmEntry);
		virtual ~PCM() = default;

		CallbackFunction GetCallback() override;

		WAVEFORMATEXTENSIBLE GetFormat() override {
			return m_format;
		}

		bool IsValid() override {
			return true;
		}

		bool IsExpired() override {
			return m_is_end && !m_hold_buffer;
		}

		bool IsReady() override {
			return IsValid();
		}

		void Seek(size_t sample) override {
			m_next = sample * m_format.Format.nBlockAlign;
		}
	private:
		std::vector<uint8_t> m_pcm_data;
		std::optional<size_t> m_loop_start;
		bool m_hold_buffer = false;
		bool m_is_end = false;
		size_t m_next = 0;
		WAVEFORMATEXTENSIBLE m_format = {};
	};
}
