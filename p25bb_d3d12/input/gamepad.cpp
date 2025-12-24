#include "gamepad.hpp"
#include <xinput.h>
#include <windows.h>
#include <algorithm>
#include <cmath>

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
	if (!IsConnected) {
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

	if (std::abs(m_impl->inputState.Gamepad.sThumbLX) < m_impl->deadZones.leftThumb) {
		m_impl->inputState.Gamepad.sThumbLX = 0;
	}
	if (std::abs(m_impl->inputState.Gamepad.sThumbLY) < m_impl->deadZones.leftThumb) {
		m_impl->inputState.Gamepad.sThumbLY = 0;
	}

	if (std::abs(m_impl->inputState.Gamepad.sThumbRX) < m_impl->deadZones.rightThumb) {
		m_impl->inputState.Gamepad.sThumbRX = 0;
	}
	if (std::abs(m_impl->inputState.Gamepad.sThumbRY) < m_impl->deadZones.rightThumb) {
		m_impl->inputState.Gamepad.sThumbRY = 0;
	}

	if (m_impl->inputState.Gamepad.bLeftTrigger <= m_impl->deadZones.trigger) {
		m_impl->inputState.Gamepad.bLeftTrigger = 0;
	}
	if (m_impl->inputState.Gamepad.bRightTrigger <= m_impl->deadZones.trigger) {
		m_impl->inputState.Gamepad.bRightTrigger = 0;
	}
}
