#pragma once
#include <cstdint>

namespace PameECS::Graphics::RendererFlags {
	enum ResetFlags : uint32_t {
		// D3Dデバイスとスワップチェーンをリセットする
		All = 0,
		// デバイスをリセットしない
		NoDeviceReset = 1u << 0,

		// 予約済み : リセットの有無を変更しない
		NoReset = 1u << 31,
	};
}
