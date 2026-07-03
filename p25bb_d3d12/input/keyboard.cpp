#include "keyboard.hpp"
#include <cassert>
#include <array>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <imgui/imgui.h>

using PameECS::Input::Keyboard;

extern "C" {
	PECS_DLL_SHARED bool InputKeyboardIsKeyDown(const Keyboard* keyboard, uint32_t keyCode) {
		return keyboard->IsKeyDown(keyCode);
	}
	PECS_DLL_SHARED bool InputKeyboardWasKeyPressed(const Keyboard* keyboard, uint32_t keyCode) {
		return keyboard->WasKeyPressed(keyCode);
	}
	PECS_DLL_SHARED bool InputKeyboardWasKeyReleased(const Keyboard* keyboard, uint32_t keyCode) {
		return keyboard->WasKeyReleased(keyCode);
	}

	PECS_DLL_SHARED bool InputKeyboardIsAnyKeyDown(const Keyboard* keyboard) {
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

void Keyboard::ShowDebug() {
	if (ImGui::TreeNode("Keyboard")) {
		if (ImGui::BeginTable("Keys", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Key code");
			ImGui::TableSetupColumn("Down");
			ImGui::TableSetupColumn("Pressed");
			ImGui::TableSetupColumn("Released");
			ImGui::TableHeadersRow();

			for (auto i = 0ui64; i < m_impl->keyState.size(); i++) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%02x", i);

				bool down = IsKeyDown(i);
				bool pressed = WasKeyPressed(i);
				bool released = WasKeyReleased(i);

				ImGui::TableSetColumnIndex(1);
				if (down) ImGui::TextColored(ImVec4(0, 1, 0, 1), "YES"); else ImGui::Text("-");
				ImGui::TableSetColumnIndex(2);
				if (pressed) ImGui::TextColored(ImVec4(1, 1, 0, 1), "TRG"); else ImGui::Text("-");
				ImGui::TableSetColumnIndex(3);
				if (released) ImGui::TextColored(ImVec4(1, 0, 0, 1), "TRG"); else ImGui::Text("-");
			}
			ImGui::EndTable();
		}
		ImGui::TreePop();
	}
}

void Keyboard::Update() {
	bool isForeground = m_impl->windowHandle == GetForegroundWindow();
	m_impl->isAnyKeyDown = false;
	for (size_t i = 0; i < std::min(m_impl->keyState.size(), m_impl->prevKeyState.size()); i++) {
		m_impl->prevKeyState[i] = m_impl->keyState[i];
		if (isForeground) {
			m_impl->keyState[i] = GetAsyncKeyState(static_cast<int>(i)) & 0x8000;
			m_impl->isAnyKeyDown = m_impl->isAnyKeyDown || m_impl->keyState[i];
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
