#include "long_wav.hpp"

using PameECS::Audio::Callbacks::LongWav;
using CallbackFunction = PameECS::Audio::Callback::CallbackFunction;

LongWav::LongWav(std::string filename, std::optional<size_t> loopStart) {
	if (drwav_init_file(&m_wav, filename.c_str(), nullptr)) {
		m_format = m_fillFormat(m_wav.channels, 32, m_wav.sampleRate, true);
		m_loop_start = loopStart;
		m_is_valid = true;
	}
}
LongWav::~LongWav() {
	if (m_is_valid) {
		drwav_uninit(&m_wav);
	}
}
CallbackFunction LongWav::GetCallback() {
	return [](void* userData, uint8_t* dest, size_t samples) -> size_t {
		LongWav* self = reinterpret_cast<LongWav*>(userData);
		if (!self) return 0;
		if (!self->m_is_valid) return 0;

		self->m_buffer.resize(samples * self->m_wav.channels);
		size_t samplesRead = drwav_read_pcm_frames_f32(&self->m_wav, samples, self->m_buffer.data());

		if (samplesRead < samples && self->m_loop_start.has_value()) {
			auto seekTo = self->m_loop_start.value();
			// ループ処理
			drwav_seek_to_pcm_frame(&self->m_wav, seekTo);
			auto remainingSamples = samples - samplesRead;
			samplesRead += drwav_read_pcm_frames_f32(&self->m_wav, remainingSamples, &self->m_buffer[samplesRead * self->m_wav.channels]);
		}

		if (samplesRead < samples) {
			self->m_is_expired = true;
		}

		std::memcpy(dest, self->m_buffer.data(), samplesRead * self->m_wav.channels * sizeof(float));
		return samplesRead;
	};
}
