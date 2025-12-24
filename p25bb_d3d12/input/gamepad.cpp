#include "gamepad.hpp"
#include <xinput.h>
#include <windows.h>
#include <algorithm>

using PameECS::Input::Gamepad;

struct Gamepad::Impl {
	XINPUT_STATE inputState = {};
	XINPUT_STATE prevInputState = {};
	int userIndex = -1;
	const int maxIndex = 4;
	std::shared_ptr<spdlog::logger> logger = nullptr;

	struct Deadzone {
		short leftThumb = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		short rightThumb = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		uint8_t trigger = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	} deadZones;
};

Gamepad::Gamepad(std::shared_ptr<spdlog::logger> logger) {
	assert(logger);
	m_impl = std::make_unique<Impl>();
	m_impl->logger = logger;
}

Gamepad::~Gamepad() {

}

void Gamepad::Update() {
	m_impl->prevInputState = m_impl->inputState;
	memset(&m_impl->inputState, 0, sizeof(XINPUT_STATE));
	if (!IsConnected()) {
		for (int i = 0; i < m_impl->maxIndex; ++i) {
			if (XInputGetState(i, &m_impl->inputState) == ERROR_SUCCESS) {
				m_impl->userIndex = i;
				break;
			}
		}
	}
	else {
		if (XInputGetState(m_impl->userIndex, &m_impl->inputState) != ERROR_SUCCESS) {
			m_impl->logger->info("Gamepad is removed");
			m_impl->userIndex = -1;
		}
	}

	if (m_impl->inputState.Gamepad.sThumbLX < m_impl->deadZones.leftThumb
		&& m_impl->inputState.Gamepad.sThumbLX > -m_impl->deadZones.leftThumb) {
		m_impl->inputState.Gamepad.sThumbLX = 0;
	}
	if (m_impl->inputState.Gamepad.sThumbLY < m_impl->deadZones.leftThumb
		&& m_impl->inputState.Gamepad.sThumbLY > -m_impl->deadZones.leftThumb) {
		m_impl->inputState.Gamepad.sThumbLY = 0;
	}

	if (m_impl->inputState.Gamepad.sThumbRX < m_impl->deadZones.rightThumb
		&& m_impl->inputState.Gamepad.sThumbRX > -m_impl->deadZones.rightThumb) {
		m_impl->inputState.Gamepad.sThumbRX = 0;
	}
	if (m_impl->inputState.Gamepad.sThumbRY < m_impl->deadZones.rightThumb
		&& m_impl->inputState.Gamepad.sThumbRY > -m_impl->deadZones.rightThumb) {
		m_impl->inputState.Gamepad.sThumbRY = 0;
	}

	if (m_impl->inputState.Gamepad.bLeftTrigger <= m_impl->deadZones.trigger) {
		m_impl->inputState.Gamepad.bLeftTrigger = 0;
	}
	if (m_impl->inputState.Gamepad.bRightTrigger <= m_impl->deadZones.trigger) {
		m_impl->inputState.Gamepad.bRightTrigger = 0;
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
	return m_impl->userIndex >= 0 && m_impl->userIndex < 4;
}
