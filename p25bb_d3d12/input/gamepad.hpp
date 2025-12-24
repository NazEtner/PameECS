#pragma once
#include <memory>
#include <spdlog/logger.h>
#include "../macros/dll.hpp"

namespace PameECS::Input {
	class Gamepad {
	public:
		Gamepad(std::shared_ptr<spdlog::logger> logger);
		~Gamepad();
		Gamepad(const Gamepad&) = delete;
		Gamepad& operator=(const Gamepad&) = delete;

		void Update();

		PECS_DLL_SHARED bool IsButtonDown(uint16_t button) const noexcept;
		PECS_DLL_SHARED bool WasButtonPressed(uint16_t button) const noexcept;
		PECS_DLL_SHARED bool WasButtonReleased(uint16_t button) const noexcept;

		PECS_DLL_SHARED float GetLeftThumbX() const noexcept;
		PECS_DLL_SHARED float GetLeftThumbY() const noexcept;
		PECS_DLL_SHARED float GetRightThumbX() const noexcept;
		PECS_DLL_SHARED float GetRightThumbX() const noexcept;
		PECS_DLL_SHARED float GetPrevLeftThumbX() const noexcept;
		PECS_DLL_SHARED float GetPrevLeftThumbY() const noexcept;
		PECS_DLL_SHARED float GetPrevRightThumbX() const noexcept;
		PECS_DLL_SHARED float GetPrevRightThumbX() const noexcept;

		PECS_DLL_SHARED float GetLeftTrigger() const noexcept;
		PECS_DLL_SHARED float GetRightTrigger() const noexcept;
		PECS_DLL_SHARED float GetPrevLeftTrigger() const noexcept;
		PECS_DLL_SHARED float GetPrevRightTrigger() const noexcept;

		PECS_DLL_SHARED void SetLeftThumbDeadZone(float deadZone) noexcept;
		PECS_DLL_SHARED void SetRightThumbDeadZone(float deadZone) noexcept;
		PECS_DLL_SHARED void SetTriggerThreshold(float threshold) noexcept;

		PECS_DLL_SHARED bool IsConnected() const noexcept;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
