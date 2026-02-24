#pragma once
#include "../callback.hpp"
#include <string>
#include <vector>
#include <optional>
#include <dr_libs/dr_wav.h>
#include <future>
#include <sstream>

namespace PameECS::Audio::Callbacks {
	class Wav : public Callback {
	public:
		Wav(const std::string& filename, std::optional<size_t> loopStart, bool holdBuffer);

		virtual ~Wav();

		CallbackFunction GetCallback() override;

		WAVEFORMATEXTENSIBLE GetFormat() override {
			return m_format;
		}

		bool IsValid() override {
			return m_is_valid;
		}

		bool IsExpired() override {
			return m_is_expired && !m_hold_buffer;
		}

		bool IsReady() override {
			if (m_file_future.valid() && m_file_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				m_loadWav();
			}
			return IsValid();
		}

		void Seek(size_t sample) override {
			if (!IsValid()) return;
			drwav_seek_to_pcm_frame(&m_wav, sample);
		}
	private:
		void m_loadWav();
		std::future<std::stringstream> m_file_future;
		std::vector<uint8_t> m_file_data;
		WAVEFORMATEXTENSIBLE m_format = {};
		std::optional<size_t> m_loop_start = std::nullopt;
		drwav m_wav = {};
		bool m_is_valid = false;
		bool m_is_expired = false;
		bool m_hold_buffer = false;
		std::vector<float> m_buffer;
	};
}
