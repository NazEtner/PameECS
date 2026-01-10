#include "long_flac.hpp"

using PameECS::Audio::Callbacks::LongFlac;
using CallbackFunction = PameECS::Audio::Callback::CallbackFunction;

LongFlac::LongFlac(std::string filename, std::optional<size_t> loopStart) {
	m_flac = drflac_open_file(filename.c_str(), nullptr);
	if (!m_flac) return;
	m_format = m_fillFormat(m_flac->channels, 32, m_flac->sampleRate, true);
	m_loop_start = loopStart;
}

LongFlac::~LongFlac() {
	if (m_flac) drflac_close(m_flac);
}

CallbackFunction LongFlac::GetCallback() {
	return [](void* userData, uint8_t* dest, size_t samples) -> size_t {
		LongFlac* self = reinterpret_cast<LongFlac*>(userData);
		if (!self) return 0;
		if (!self->m_flac) return 0;
		self->m_buffer.resize(samples * self->m_flac->channels);
		size_t samplesRead = drflac_read_pcm_frames_f32(self->m_flac, samples, self->m_buffer.data());

		if (samplesRead < samples && self->m_loop_start.has_value()) {
			auto seekTo = self->m_loop_start.value();
			// ループ処理
			drflac_seek_to_pcm_frame(self->m_flac, seekTo);
			auto remainingSamples = samples - samplesRead;
			samplesRead += drflac_read_pcm_frames_f32(self->m_flac, remainingSamples, &self->m_buffer[samplesRead * self->m_flac->channels]);
		}

		if (samplesRead < samples) {
			self->m_is_expired = true;
		}

		std::memcpy(dest, self->m_buffer.data(), samplesRead * self->m_flac->channels * sizeof(float));
		return samplesRead;
	};
}
