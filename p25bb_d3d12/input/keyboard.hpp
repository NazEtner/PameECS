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
		bool IsKeyDown(uint32_t keyCode) const;
		bool WasKeyPressed(uint32_t keyCode) const;
		bool WasKeyReleased(uint32_t keyCode) const;

		bool IsAnyKeyDown() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PECS_DLL_SHARED bool InputKeyboardIsKeyDown(const PameECS::Input::Keyboard* keyboard, uint32_t keyCode);
	PECS_DLL_SHARED bool InputKeyboardWasKeyPressed(const PameECS::Input::Keyboard* keyboard, uint32_t keyCode);
	PECS_DLL_SHARED bool InputKeyboardWasKeyReleased(const PameECS::Input::Keyboard* keyboard, uint32_t keyCode);

	PECS_DLL_SHARED bool InputKeyboardIsAnyKeyDown(const PameECS::Input::Keyboard* keyboard);
}
