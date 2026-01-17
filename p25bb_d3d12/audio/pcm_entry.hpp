#pragma once
#include <cstdint>
#include <windows.h>
#include <mmreg.h>

namespace PameECS::Audio {
	struct PCMEntry {
		const uint8_t* data;
		size_t dataSize;
		size_t loopStart = 0; // ループする場合、戻る位置(サンプル)
		bool loopEnable = false;
		WAVEFORMATEXTENSIBLE waveFormat;
	};
}
