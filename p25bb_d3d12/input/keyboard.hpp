#pragma once
#include "../macros/dll.hpp"
#include <memory>
#include <windows.h>

namespace PameECS::Input {
	class Keyboard {
	public:
		Keyboard(HWND windowHandle);
		~Keyboard();
		Keyboard(const Keyboard&) = delete;
		Keyboard& operator=(const Keyboard&) = delete;

		void Update();
		// 引数はWindows APIのVK_から始まるキーコードに対応する
		PECS_DLL_SHARED bool IsKeyDown(uint32_t keyCode);
		PECS_DLL_SHARED bool WasKeyPressed(uint32_t keyCode);
		PECS_DLL_SHARED bool WasKeyReleased(uint32_t keyCode);
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
