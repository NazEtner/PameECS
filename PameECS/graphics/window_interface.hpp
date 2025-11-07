#pragma once

namespace Pame::Graphics {
	class IWindow {
	public:
		IWindow() = default;
		virtual ~IWindow() = default;
		virtual void Show() = 0;
		virtual bool Update() = 0;
	};
}
