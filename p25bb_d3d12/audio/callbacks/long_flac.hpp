#pragma once
#include "../callback.hpp"
#include <string>
#include <vector>
#include <optional>
#include <dr_libs/dr_flac.h>

namespace PameECS::Audio::Callbacks {
	// アーカイブファイルに入っていない長いFLACファイルの再生を軽量化するためのコールバック
	class LongFlac : public Callback {
	public:
		LongFlac(const std::string& filename, std::optional<size_t> loopStart, bool holdBuffer);

		virtual ~LongFlac();

		CallbackFunction GetCallback() override;

		WAVEFORMATEXTENSIBLE GetFormat() override{
			return m_format;
		}

		bool IsValid() override {
			return m_flac;
		}

		bool IsExpired() override {
			return m_is_expired && !m_hold_buffer;
		}

		bool IsReady() override {
			return IsValid();
		}

		void Seek(size_t sample) override {
			if (!IsValid()) return;
			drflac_seek_to_pcm_frame(m_flac, sample);
		}
	private:
		WAVEFORMATEXTENSIBLE m_format = {};
		std::optional<size_t> m_loop_start = std::nullopt;
		drflac* m_flac = nullptr;
		std::vector<float> m_buffer;
		bool m_is_expired = false;
		bool m_hold_buffer = false;
	};
}
