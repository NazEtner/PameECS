#include "pcm.hpp"

using PameECS::Audio::Callbacks::PCM;
using CallbackFunction = PameECS::Audio::Callback::CallbackFunction;

PCM::PCM(const PCMEntry& pcmEntry) {
	m_pcm_data = std::vector<uint8_t>(pcmEntry.data, pcmEntry.data + pcmEntry.dataSize);
	m_format = pcmEntry.waveFormat;
	m_hold_buffer = pcmEntry.holdBuffer;
	if (pcmEntry.loopEnable) {
		m_loop_start = pcmEntry.loopStart;
	}
}

CallbackFunction PCM::GetCallback() {
	return[](void* userData, uint8_t* dest, size_t samples) -> size_t {
		auto self = reinterpret_cast<PCM*>(userData);
		size_t read = 0;

		while (read < samples) {
			auto samplesRemaining = samples - read;
			auto bytes = std::min(samplesRemaining * self->GetFormat().Format.nBlockAlign, self->m_pcm_data.size() - self->m_next);

			memcpy(dest + read * self->GetFormat().Format.nBlockAlign, self->m_pcm_data.data() + self->m_next, bytes);

			read += bytes / self->GetFormat().Format.nBlockAlign;
			self->m_next += bytes;
			if (read < samples && self->m_loop_start.has_value()) {
				self->Seek(self->m_loop_start.value());
			}
			else if (read < samples) {
				self->m_is_end = true;
				break;
			}
		}

		return read;
	};
}
