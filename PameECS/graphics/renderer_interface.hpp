#pragma once
#include <cstdint>

namespace Pame::Graphics {
	class IRenderer {
	public:
		virtual ~IRenderer() = default;
		virtual bool Render() = 0;
		virtual bool Present() = 0;
		virtual void Recovery() = 0;
		virtual bool Reset(uint32_t flags = 0) = 0;
	};
}
