#pragma once
#include "../callback.hpp"
#include "../../file/file.hpp"
#include <string>
#include <vector>
#include <optional>
#include <dr_libs/dr_flac.h>

namespace PameECS::Audio::Callbacks {
	class Flac : public Callback {
	public:
		Flac(const std::string& filename, std::optional<size_t> loopStart, bool holdBuffer);

		virtual ~Flac();

		CallbackFunction GetCallback() override;

		WAVEFORMATEXTENSIBLE GetFormat() override {
			return m_format;
		}

		bool IsValid() override {
			return m_flac;
		}

		bool IsExpired() override {
			return m_is_expired && !m_hold_buffer;
		}

		bool IsReady() override {
			if (m_file_future.valid() && m_file_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				m_loadFlac();
			}
			return IsValid();
		}

		void Seek(size_t sample) override {
			if (!IsValid()) return;
			drflac_seek_to_pcm_frame(m_flac, sample);
		}
	private:
		void m_loadFlac();
		std::future<std::stringstream> m_file_future;
		std::vector<uint8_t> m_file_data;
		WAVEFORMATEXTENSIBLE m_format = {};
		std::optional<size_t> m_loop_start = std::nullopt;
		drflac* m_flac = nullptr;
		std::vector<float> m_buffer;
		bool m_is_expired = false;
		bool m_hold_buffer = false;
	};
}
