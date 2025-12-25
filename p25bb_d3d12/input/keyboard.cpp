#include "keyboard.hpp"
#include <cassert>
#include <array>
#include <algorithm>

using PameECS::Input::Keyboard;

extern "C" {
	PECS_DLL_SHARED bool __stdcall InputKeyboardIsKeyDown(const Keyboard* keyboard, uint32_t keyCode) {
		return keyboard->IsKeyDown(keyCode);
	}
	PECS_DLL_SHARED bool __stdcall InputKeyboardWasKeyPressed(const Keyboard* keyboard, uint32_t keyCode) {
		return keyboard->WasKeyPressed(keyCode);
	}
	PECS_DLL_SHARED bool __stdcall InputKeyboardWasKeyReleased(const Keyboard* keyboard, uint32_t keyCode) {
		return keyboard->WasKeyReleased(keyCode);
	}

	PECS_DLL_SHARED bool __stdcall InputKeyboardIsAnyKeyDown(const Keyboard* keyboard) {
		return keyboard->IsAnyKeyDown();
	}
}

struct Keyboard::Impl {
	HWND windowHandle = nullptr;
	std::array<bool, 256> keyState = {};
	std::array<bool, 256> prevKeyState = {};
	bool isAnyKeyDown = false;
};

Keyboard::Keyboard(HWND windowHandle) {
	m_impl = std::make_unique<Impl>();

	assert(windowHandle);

	m_impl->windowHandle = windowHandle;
}

Keyboard::~Keyboard() {
	// nothing to do.
}

void Keyboard::Update() {
	bool isForeground = m_impl->windowHandle == GetForegroundWindow();
	m_impl->isAnyKeyDown = false;
	for (size_t i = 0; i < std::min(m_impl->keyState.size(), m_impl->prevKeyState.size()); i++) {
		m_impl->prevKeyState[i] = m_impl->keyState[i];
		if (isForeground) {
			m_impl->keyState[i] = GetAsyncKeyState(static_cast<int>(i)) & 0x8000;
			m_impl->isAnyKeyDown = true;
		}
		else {
			m_impl->keyState[i] = false;
		}
	}
}

bool Keyboard::IsKeyDown(uint32_t keyCode) const {
	if (keyCode >= m_impl->keyState.size()) return false;

	return m_impl->keyState[keyCode];
}

bool Keyboard::WasKeyPressed(uint32_t keyCode) const {
	if (keyCode >= std::min(m_impl->keyState.size(), m_impl->prevKeyState.size())) return false;

	return m_impl->keyState[keyCode] && !m_impl->prevKeyState[keyCode];
}

bool Keyboard::WasKeyReleased(uint32_t keyCode) const {
	if (keyCode >= std::min(m_impl->keyState.size(), m_impl->prevKeyState.size())) return false;

	return !m_impl->keyState[keyCode] && m_impl->prevKeyState[keyCode];
}

bool Keyboard::IsAnyKeyDown() const {
	return m_impl->isAnyKeyDown;
}
