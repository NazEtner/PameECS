#include "wav.hpp"
#include "../../file/file.hpp"

using PameECS::Audio::Callbacks::Wav;
using CallbackFunction = PameECS::Audio::Callback::CallbackFunction;

Wav::Wav(const std::string& filename, std::optional<size_t> loopStart, bool holdBuffer) {
	// if (drwav_init_file(&m_wav, filename.c_str(), nullptr)) {
	//	m_format = m_fillFormat(m_wav.channels, 32, m_wav.sampleRate, true);
	//	m_is_valid = true;
	// }
	File::File<0, 0> file;
	m_file_future = file.LoadAsync(filename);
	m_loop_start = loopStart;
	m_hold_buffer = holdBuffer;
}

Wav::~Wav() {
	if (m_is_valid) {
		drwav_uninit(&m_wav);
	}
}

void Wav::m_loadWav() {
	auto ss = m_file_future.get();
	auto view = ss.view();
	m_file_data = std::vector<uint8_t>(view.begin(), view.end());
	if (drwav_init_memory(&m_wav, m_file_data.data(), m_file_data.size(), nullptr)) {
		m_format = m_fillFormat(m_wav.channels, 32, m_wav.sampleRate, true);
		m_is_valid = true;
	}
}

CallbackFunction Wav::GetCallback() {
	return [](void* userData, uint8_t* dest, size_t samples) -> size_t {
		Wav* self = reinterpret_cast<Wav*>(userData);
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
