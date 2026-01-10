#pragma once
#include "../callback.hpp"
#include <string>
#include <vector>
#include <optional>
#include <dr_libs/dr_wav.h>

namespace PameECS::Audio::Callbacks {
	// アーカイブファイルに入っていない長いWAVファイルの再生を軽量化するためのコールバック
	class LongWav : public Callback {
	public:
		LongWav(std::string filename, std::optional<size_t> loopStart);

		virtual ~LongWav();

		CallbackFunction GetCallback() override;

		WAVEFORMATEXTENSIBLE GetFormat() override {
			return m_format;
		}
		void* GetUserData() override {
			return this;
		}
		bool IsValid() override {
			return m_is_valid;
		}

		bool IsExpired() override {
			return m_is_expired;
		}
	private:
		WAVEFORMATEXTENSIBLE m_format = {};
		std::optional<size_t> m_loop_start = std::nullopt;
		drwav m_wav = {};
		bool m_is_valid = false;
		bool m_is_expired = false;
		std::vector<float> m_buffer;
	};
}
