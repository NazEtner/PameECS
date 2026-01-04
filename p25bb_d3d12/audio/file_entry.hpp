#pragma once
#include <cstdint>

namespace PameECS::Audio {
	struct FileEntry {
		const char* fileName;
		size_t nameSize;
		enum class Codec {
			WAV,
			FLAC,
		} codec;
	};
}
