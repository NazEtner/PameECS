#pragma once

namespace Pame::Graphics {
	class IRenderer {
	public:
		virtual ~IRenderer() = default;
		virtual bool Render() = 0;
		virtual bool Present() = 0;
		virtual void Recovery() = 0;
		virtual bool Reset() = 0;
	};
}
