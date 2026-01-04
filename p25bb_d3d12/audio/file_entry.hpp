#pragma once
#include <cstdint>

namespace PameECS::Audio {
	struct FileEntry {
		const char* fileName;
		size_t nameSize;
		size_t loopStart = 0; // ループする場合、戻る位置(サンプル)
		bool loopEnable = false;
		enum class Codec {
			WAV,
			FLAC,
		} codec;
	};
}
