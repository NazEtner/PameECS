#include "input_service.hpp"

extern "C" {
	PECS_DLL_SHARED PameECS::Input::Keyboard* ServiceInputGetKeyboard(const PameECS::Services::InputService* inputService) {
		return inputService->GetKeyboard();
	}
	PECS_DLL_SHARED PameECS::Input::Mouse* ServiceInputGetMouse(const PameECS::Services::InputService* inputService) {
		return inputService->GetMouse();
	}
	PECS_DLL_SHARED PameECS::Input::Gamepad* ServiceInputGetGamepad(const PameECS::Services::InputService* inputService, int index) {
		return inputService->GetGamepad(index);
	}
	PECS_DLL_SHARED PameECS::Input::Gamepad* ServiceInputGetGamepadAutoIndex(const PameECS::Services::InputService* inputService) {
		return inputService->GetGamepadAutoIndex();
	}
}

using PameECS::Services::InputService;

struct InputService::Impl {
	inline static constexpr int gamepadMax = 4;
	Impl(HWND windowHandle, std::shared_ptr<spdlog::logger> logger) {
		keyboard = std::make_unique<Input::Keyboard>(windowHandle);
		mouse = std::make_unique<Input::Mouse>(windowHandle);
		for (int i = 0; i < gamepads.size(); ++i) {
			auto& pad = gamepads[i];
			pad = std::make_unique<Input::Gamepad>(logger, i);
		}
		gamepadAutoIndex = std::make_unique<Input::Gamepad>(logger);
	}

	std::unique_ptr<Input::Keyboard> keyboard;
	std::unique_ptr<Input::Mouse> mouse;
	std::array<std::unique_ptr<Input::Gamepad>, gamepadMax> gamepads;
	std::unique_ptr<Input::Gamepad> gamepadAutoIndex;
};

InputService::InputService(HWND windowHandle, std::shared_ptr<spdlog::logger> logger) {
	m_impl = std::make_unique<Impl>(windowHandle, logger);
}

InputService::~InputService() {

}

void InputService::ShowDebug() {
	m_impl->gamepadAutoIndex->ShowDebug();
	for (auto& pad : m_impl->gamepads) {
		pad->ShowDebug();
	}
}

void InputService::Update() {
	m_impl->keyboard->Update();
	m_impl->mouse->Update();
	for (auto& pad : m_impl->gamepads) {
		pad->Update();
	}
	m_impl->gamepadAutoIndex->Update();
}

PameECS::Input::Keyboard* InputService::GetKeyboard() const {
	return m_impl->keyboard.get();
}

PameECS::Input::Mouse* InputService::GetMouse() const {
	return m_impl->mouse.get();
}

PameECS::Input::Gamepad* InputService::GetGamepad(int index) const {
	if (index < 0 || index >= Impl::gamepadMax) return GetGamepadAutoIndex();
	return m_impl->gamepads[index].get();
}

PameECS::Input::Gamepad* InputService::GetGamepadAutoIndex() const {
	return m_impl->gamepadAutoIndex.get();
}
