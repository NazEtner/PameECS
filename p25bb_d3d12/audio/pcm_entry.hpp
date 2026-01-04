#pragma once
#include <cstdint>

namespace PameECS::Audio {
	struct PCMEntry {
		const uint8_t* data;
		size_t dataSize;
		uint8_t channels;
		uint8_t bitsPerSample;
		uint32_t sampleRate;
	};
}
