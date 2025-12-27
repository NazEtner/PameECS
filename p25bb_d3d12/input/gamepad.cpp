#include "gamepad.hpp"
#include <xinput.h>
#include <windows.h>
#include <algorithm>
#include <imgui/imgui.h>

using PameECS::Input::Gamepad;

extern "C" {
	PECS_DLL_SHARED bool InputGamepadIsButtonDown(const Gamepad* gamepad, uint16_t button) {
		return gamepad->IsButtonDown(button);
	}
	PECS_DLL_SHARED bool InputGamepadWasButtonPressed(const Gamepad* gamepad, uint16_t button) {
		return gamepad->WasButtonPressed(button);
	}
	PECS_DLL_SHARED bool InputGamepadWasButtonReleased(const Gamepad* gamepad, uint16_t button) {
		return gamepad->WasButtonReleased(button);
	}

	PECS_DLL_SHARED int16_t InputGamepadGetLeftThumbX(const Gamepad* gamepad) {
		return gamepad->GetLeftThumbX();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetLeftThumbY(const Gamepad* gamepad) {
		return gamepad->GetLeftThumbY();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetRightThumbX(const Gamepad* gamepad) {
		return gamepad->GetRightThumbX();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetRightThumbY(const Gamepad* gamepad) {
		return gamepad->GetRightThumbY();
	}

	PECS_DLL_SHARED int16_t InputGamepadGetPrevLeftThumbX(const Gamepad* gamepad) {
		return gamepad->GetPrevLeftThumbX();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetPrevLeftThumbY(const Gamepad* gamepad) {
		return gamepad->GetPrevLeftThumbY();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetPrevRightThumbX(const Gamepad* gamepad) {
		return gamepad->GetPrevRightThumbX();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetPrevRightThumbY(const Gamepad* gamepad) {
		return gamepad->GetPrevRightThumbY();
	}

	PECS_DLL_SHARED uint8_t InputGamepadGetLeftTrigger(const Gamepad* gamepad) {
		return gamepad->GetLeftTrigger();
	}
	PECS_DLL_SHARED uint8_t InputGamepadGetRightTrigger(const Gamepad* gamepad) {
		return gamepad->GetRightTrigger();
	}
	PECS_DLL_SHARED uint8_t InputGamepadGetPrevLeftTrigger(const Gamepad* gamepad) {
		return gamepad->GetPrevLeftTrigger();
	}
	PECS_DLL_SHARED uint8_t InputGamepadGetPrevRightTrigger(const Gamepad* gamepad) {
		return gamepad->GetPrevRightTrigger();
	}

	PECS_DLL_SHARED void InputGamepadSetLeftThumbDeadZone(Gamepad* gamepad, int16_t deadZone) {
		gamepad->SetLeftThumbDeadZone(deadZone);
	}
	PECS_DLL_SHARED void InputGamepadSetRightThumbDeadZone(Gamepad* gamepad, int16_t deadZone) {
		gamepad->SetRightThumbDeadZone(deadZone);
	}
	PECS_DLL_SHARED void InputGamepadSetTriggerThreshold(Gamepad* gamepad, uint8_t threshold) {
		gamepad->SetTriggerThreshold(threshold);
	}

	PECS_DLL_SHARED int16_t InputGamepadGetLeftThumbDeadZone(const Gamepad* gamepad) {
		return gamepad->GetLeftThumbDeadZone();
	}
	PECS_DLL_SHARED int16_t InputGamepadGetRightThumbDeadZone(const Gamepad* gamepad) {
		return gamepad->GetRightThumbDeadZone();
	}
	PECS_DLL_SHARED uint8_t InputGamepadGetTriggerThreshold(const Gamepad* gamepad) {
		return gamepad->GetTriggerThreshold();
	}

	PECS_DLL_SHARED bool InputGamepadIsConnected(const Gamepad* gamepad) {
		return gamepad->IsConnected();
	}
}

struct Gamepad::Impl {
	XINPUT_STATE inputState = {};
	XINPUT_STATE prevInputState = {};
	int userIndex = -1;
	bool autoIndexEnabled = true;
	const int maxIndex = 4;
	std::shared_ptr<spdlog::logger> logger = nullptr;
	int32_t byNextPolling = 0;
	const int32_t pollingInterval = 30;
	bool isSucceeded = false;

	struct Deadzone {
		short leftThumb = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		short rightThumb = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		uint8_t trigger = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	} deadZones;
};

Gamepad::Gamepad(std::shared_ptr<spdlog::logger> logger, int id) {
	assert(logger);
	m_impl = std::make_unique<Impl>();
	m_impl->logger = logger;
	if (0 <= id && id < m_impl->maxIndex) {
		m_impl->autoIndexEnabled = false;
		m_impl->userIndex = id;
	}
}

Gamepad::~Gamepad() {

}

void Gamepad::ShowDebug() {
	if (ImGui::TreeNode(("Gamepad " + std::to_string(m_impl->userIndex) + (m_impl->autoIndexEnabled ? " AutoIndex" : "")).c_str())) {
		if (!IsConnected()) {
			ImGui::Text("Disconnected.");
			ImGui::TreePop();
			return;
		}
		ImGui::Text("Connected : %d, AutoIndex = %s", m_impl->userIndex, m_impl->autoIndexEnabled ? "true" : "false");
		std::vector<std::pair<WORD, std::string>> buttons = {
			{XINPUT_GAMEPAD_DPAD_UP, "XINPUT_GAMEPAD_DPAD_UP"},
			{XINPUT_GAMEPAD_DPAD_DOWN, "XINPUT_GAMEPAD_DPAD_DOWN"},
			{XINPUT_GAMEPAD_DPAD_LEFT, "XINPUT_GAMEPAD_DPAD_LEFT"},
			{XINPUT_GAMEPAD_DPAD_RIGHT, "XINPUT_GAMEPAD_DPAD_RIGHT"},
			{XINPUT_GAMEPAD_START, "XINPUT_GAMEPAD_START"},
			{XINPUT_GAMEPAD_BACK, "XINPUT_GAMEPAD_BACK"},
			{XINPUT_GAMEPAD_LEFT_THUMB, "XINPUT_GAMEPAD_LEFT_THUMB"},
			{XINPUT_GAMEPAD_RIGHT_THUMB, "XINPUT_GAMEPAD_RIGHT_THUMB"},
			{XINPUT_GAMEPAD_LEFT_SHOULDER, "XINPUT_GAMEPAD_LEFT_SHOULDER"},
			{XINPUT_GAMEPAD_RIGHT_SHOULDER, "XINPUT_GAMEPAD_RIGHT_SHOULDER"},
			{XINPUT_GAMEPAD_A, "XINPUT_GAMEPAD_A"},
			{XINPUT_GAMEPAD_B, "XINPUT_GAMEPAD_B"},
			{XINPUT_GAMEPAD_X, "XINPUT_GAMEPAD_X"},
			{XINPUT_GAMEPAD_Y, "XINPUT_GAMEPAD_Y"},
		};
		for (const auto& button : buttons) {
			bool buttonDown = IsButtonDown(button.first);
			bool buttonPressed = WasButtonPressed(button.first);
			bool buttonReleased = WasButtonReleased(button.first);
			ImVec4 color = { 1.0, 1.0, 1.0, 1.0 };
			if (buttonPressed) {
				color = { 0.0, 0.0, 1.0, 1.0 };
			}
			else if (buttonDown) {
				color = { 0.0, 1.0, 0.0, 1.0 };
			}
			else if (buttonReleased) {
				color = { 1.0, 0.0, 0.0, 1.0 };
			}

			ImGui::TextColored(color, button.second.c_str());
		}
		auto drawThumbStick = [](const char* label, float x, float y, int16_t rawX, int16_t rawY, float size = 100.0f) {
			ImGui::Text("%s", label);

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImDrawList* draw = ImGui::GetWindowDrawList();

			ImVec2 center = ImVec2(p.x + size * 0.5f, p.y + size * 0.5f);
			float radius = size * 0.45f;

			// 背景
			draw->AddRect(p, ImVec2(p.x + size, p.y + size), IM_COL32(255, 255, 255, 255));
			draw->AddLine(
				ImVec2(center.x, p.y),
				ImVec2(center.x, p.y + size),
				IM_COL32(128, 128, 128, 255)
			);
			draw->AddLine(
				ImVec2(p.x, center.y),
				ImVec2(p.x + size, center.y),
				IM_COL32(128, 128, 128, 255)
			);

			// スティック位置
			ImVec2 stickPos = ImVec2(
				center.x + x * radius,
				center.y - y * radius   // y軸反転
			);

			auto rawXNormalized = rawX >= 0 ? rawX / 32767.0f : rawX / 32768.0f;
			auto rawYNormalized = rawY >= 0 ? rawY / 32767.0f : rawY / 32768.0f;

			ImVec2 rawStickPos = ImVec2(
				center.x + rawXNormalized * radius,
				center.y - rawYNormalized * radius
			);

			draw->AddCircleFilled(stickPos, 5.0f, IM_COL32(255, 0, 0, 255));
			draw->AddCircleFilled(rawStickPos, 5.0f, IM_COL32(0, 255, 0, 255));

			ImGui::Dummy(ImVec2(size, size));
		};

		ImGui::Text("Left thumb    : %d, %d(%f, %f)",
			GetLeftThumbX(), GetLeftThumbY(),
			GetNormalizedLeftThumbX<float>(), GetNormalizedLeftThumbY<float>());

		ImGui::Text("Right thumb   : %d, %d(%f, %f)",
			GetRightThumbX(), GetRightThumbY(),
			GetNormalizedRightThumbX<float>(), GetNormalizedRightThumbY<float>());

		if (ImGui::BeginTable("Thumbs", 2, ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableNextColumn();

			drawThumbStick("Left Thumb",
				GetNormalizedLeftThumbX<float>(), GetNormalizedLeftThumbY<float>(),
				GetLeftThumbX(), GetLeftThumbY());

			ImGui::TableNextColumn();

			drawThumbStick("Right Thumb",
				GetNormalizedRightThumbX<float>(), GetNormalizedRightThumbY<float>(),
				GetRightThumbX(), GetRightThumbY());

			ImGui::EndTable();
		}
		ImGui::Text("Left trigger  : %d(%f)", GetLeftTrigger(), GetNormalizedLeftTrigger<float>());
		ImGui::Text("Right trigger : %d(%f)", GetRightTrigger(), GetNormalizedRightTrigger<float>());
		ImGui::TreePop();
	}
}

void Gamepad::Update() {
	m_impl->prevInputState = m_impl->inputState;
	m_impl->inputState = {};
	XINPUT_STATE currentState = {};
	bool isSucceeded = false;
	if (m_impl->autoIndexEnabled) {
		if (!IsConnected() && m_impl->byNextPolling <= 0) {
			m_impl->byNextPolling = m_impl->pollingInterval;
			for (int i = 0; i < m_impl->maxIndex; ++i) {
				if (XInputGetState(i, &currentState) == ERROR_SUCCESS) {
					m_impl->userIndex = i;
					isSucceeded = true;
					break;
				}
			}
		}
		else {
			if (XInputGetState(m_impl->userIndex, &currentState) != ERROR_SUCCESS) {
				if (m_impl->isSucceeded) m_impl->logger->info("Gamepad<AutoIndex> is removed");
				m_impl->userIndex = -1;
			}
			else {
				isSucceeded = true;
			}
		}
	}
	else {
		if (XInputGetState(m_impl->userIndex, &currentState) == ERROR_SUCCESS) {
			isSucceeded = true;
		}
		else if (m_impl->isSucceeded) {
			m_impl->logger->info("Gamepad<{}> is removed", m_impl->userIndex);
		}
	}
	if (isSucceeded) m_impl->inputState = currentState;
	m_impl->isSucceeded = isSucceeded;

	if (m_impl->byNextPolling > 0) {
		--m_impl->byNextPolling;
	}
}

bool Gamepad::IsButtonDown(uint16_t button) const noexcept {
	return m_impl->inputState.Gamepad.wButtons & button;
}

bool Gamepad::WasButtonPressed(uint16_t button) const noexcept {
	bool now = m_impl->inputState.Gamepad.wButtons & button;
	bool prev = m_impl->prevInputState.Gamepad.wButtons & button;

	return now && !prev;
}

bool Gamepad::WasButtonReleased(uint16_t button) const noexcept {
	bool now = m_impl->inputState.Gamepad.wButtons & button;
	bool prev = m_impl->prevInputState.Gamepad.wButtons & button;

	return !now && prev;
}

int16_t Gamepad::GetLeftThumbX() const noexcept {
	return m_impl->inputState.Gamepad.sThumbLX;
}

int16_t Gamepad::GetLeftThumbY() const noexcept {
	return m_impl->inputState.Gamepad.sThumbLY;
}

int16_t Gamepad::GetRightThumbX() const noexcept {
	return m_impl->inputState.Gamepad.sThumbRX;
}

int16_t Gamepad::GetRightThumbY() const noexcept {
	return m_impl->inputState.Gamepad.sThumbRY;
}

int16_t Gamepad::GetPrevLeftThumbX() const noexcept {
	return m_impl->prevInputState.Gamepad.sThumbLX;
}

int16_t Gamepad::GetPrevLeftThumbY() const noexcept {
	return m_impl->prevInputState.Gamepad.sThumbLY;
}

int16_t Gamepad::GetPrevRightThumbX() const noexcept {
	return m_impl->prevInputState.Gamepad.sThumbRX;
}

int16_t Gamepad::GetPrevRightThumbY() const noexcept {
	return m_impl->prevInputState.Gamepad.sThumbRY;
}

uint8_t Gamepad::GetLeftTrigger() const noexcept {
	return m_impl->inputState.Gamepad.bLeftTrigger;
}

uint8_t Gamepad::GetRightTrigger() const noexcept {
	return m_impl->inputState.Gamepad.bRightTrigger;
}

uint8_t Gamepad::GetPrevLeftTrigger() const noexcept {
	return m_impl->prevInputState.Gamepad.bLeftTrigger;
}

uint8_t Gamepad::GetPrevRightTrigger() const noexcept {
	return m_impl->prevInputState.Gamepad.bRightTrigger;
}

void Gamepad::SetLeftThumbDeadZone(int16_t deadZone) noexcept {
	assert(deadZone >= 0);
	m_impl->deadZones.leftThumb = deadZone;
}

void Gamepad::SetRightThumbDeadZone(int16_t deadZone) noexcept {
	assert(deadZone >= 0);
	m_impl->deadZones.rightThumb = deadZone;
}

void Gamepad::SetTriggerThreshold(uint8_t threshold) noexcept {
	assert(threshold >= 0);
	m_impl->deadZones.trigger = threshold;
}

int16_t Gamepad::GetLeftThumbDeadZone() const noexcept {
	return m_impl->deadZones.leftThumb;
}

int16_t Gamepad::GetRightThumbDeadZone() const noexcept {
	return m_impl->deadZones.rightThumb;
}

uint8_t Gamepad::GetTriggerThreshold() const noexcept {
	return m_impl->deadZones.trigger;
}

bool Gamepad::IsConnected() const noexcept {
	return m_impl->isSucceeded;
}
