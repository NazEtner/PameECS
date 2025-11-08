#pragma once
#include <cstdint>

namespace PameECS::Graphics::RendererFlags {
	enum ResetFlags : uint32_t {
		None = 0,
		NoDeviceReset = 1 << 0,
	};
}
