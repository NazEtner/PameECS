#include "flac.hpp"

using PameECS::Audio::Callbacks::Flac;
using CallbackFunction = PameECS::Audio::Callback::CallbackFunction;

Flac::Flac(const std::string& filename, std::optional<size_t> loopStart, bool holdBuffer) {
	File::File<0, 0> file;
	m_file_future = file.LoadAsync(filename);
	m_loop_start = loopStart;
	m_hold_buffer = holdBuffer;
}

Flac::~Flac() {
	if (m_flac) drflac_close(m_flac);
}

void Flac::m_loadFlac() {
	auto ss = m_file_future.get();
	auto view = ss.view();
	m_file_data = std::vector<uint8_t>(view.begin(), view.end());
	m_flac = drflac_open_memory(m_file_data.data(), m_file_data.size(), nullptr);
	if (!m_flac) return;
	m_format = m_fillFormat(m_flac->channels, 32, m_flac->sampleRate, true);
}

CallbackFunction Flac::GetCallback() {
	return [](void* userData, uint8_t* dest, size_t samples) -> size_t {
		Flac* self = reinterpret_cast<Flac*>(userData);
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
