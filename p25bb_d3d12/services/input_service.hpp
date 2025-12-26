#pragma once
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"
#include "../input/gamepad.hpp"

namespace PameECS::Services {
	class InputService {
	public:
		InputService(HWND windowHandle, std::shared_ptr<spdlog::logger> logger);
		Input::Keyboard* GetKeyboard() const;
		Input::Mouse* GetMouse() const;
		// インデックス（0~3）を指定してGamepadクラスのポインタを取得する
		Input::Gamepad* GetGamepad(int index) const;
		// 自動でインデックスを指定するGamepadクラスのポインタを取得する
		// GetGamepadで無効なインデックスを指定した場合と同じ動作
		Input::Gamepad* GetGamepadAutoIndex() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PECS_DLL_SHARED PameECS::Input::Keyboard* ServiceInputGetKeyboard(const PameECS::Services::InputService* inputService);
	PECS_DLL_SHARED PameECS::Input::Mouse* ServiceInputGetMouse(const PameECS::Services::InputService* inputService);
	PECS_DLL_SHARED PameECS::Input::Gamepad* ServiceInputGetGamepad(const PameECS::Services::InputService* inputService, int index);
	PECS_DLL_SHARED PameECS::Input::Gamepad* ServiceInputGetGamepadAutoIndex(const PameECS::Services::InputService* inputService);
}
