#pragma once
#include <memory>
#include <concepts>
#include <limits>
#include <cmath>
#include <cassert>
#include <spdlog/logger.h>
#include "../macros/dll.hpp"

namespace PameECS::Input {
	class Gamepad;
}

extern "C" {
	PECS_DLL_SHARED bool InputGamepadIsButtonDown(const PameECS::Input::Gamepad* gamepad, uint16_t button);
	PECS_DLL_SHARED bool InputGamepadWasButtonPressed(const PameECS::Input::Gamepad* gamepad, uint16_t button);
	PECS_DLL_SHARED bool InputGamepadWasButtonReleased(const PameECS::Input::Gamepad* gamepad, uint16_t button);

	PECS_DLL_SHARED int16_t InputGamepadGetLeftThumbX(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetLeftThumbY(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetRightThumbX(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetRightThumbY(const PameECS::Input::Gamepad* gamepad);

	PECS_DLL_SHARED int16_t InputGamepadGetPrevLeftThumbX(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetPrevLeftThumbY(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetPrevRightThumbX(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetPrevRightThumbY(const PameECS::Input::Gamepad* gamepad);

	PECS_DLL_SHARED uint8_t InputGamepadGetLeftTrigger(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED uint8_t InputGamepadGetRightTrigger(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED uint8_t InputGamepadGetPrevLeftTrigger(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED uint8_t InputGamepadGetPrevRightTrigger(const PameECS::Input::Gamepad* gamepad);

	PECS_DLL_SHARED void InputGamepadSetLeftThumbDeadZone(PameECS::Input::Gamepad* gamepad, int16_t deadZone);
	PECS_DLL_SHARED void InputGamepadSetRightThumbDeadZone(PameECS::Input::Gamepad* gamepad, int16_t deadZone);
	PECS_DLL_SHARED void InputGamepadSetTriggerThreshold(PameECS::Input::Gamepad* gamepad, uint8_t threshold);

	PECS_DLL_SHARED int16_t InputGamepadGetLeftThumbDeadZone(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED int16_t InputGamepadGetRightThumbDeadZone(const PameECS::Input::Gamepad* gamepad);
	PECS_DLL_SHARED uint8_t InputGamepadGetTriggerThreshold(const PameECS::Input::Gamepad* gamepad);

	PECS_DLL_SHARED bool InputGamepadIsConnected(const PameECS::Input::Gamepad* gamepad);
}

namespace PameECS::Input {
	class Gamepad {
	public:
		Gamepad(std::shared_ptr<spdlog::logger> logger,
			int id = -1 // Auto: id < 0 or 3 < id
		);
		~Gamepad();
		Gamepad(const Gamepad&) = delete;
		Gamepad& operator=(const Gamepad&) = delete;

		void ShowDebug();
		
		void Update();

		bool IsButtonDown(uint16_t button) const noexcept;
		bool WasButtonPressed(uint16_t button) const noexcept;
		bool WasButtonReleased(uint16_t button) const noexcept;

		int16_t GetLeftThumbX() const noexcept;
		template <std::floating_point T>
		T GetNormalizedLeftThumbX() const noexcept {
			return m_normalize<T>(InputGamepadGetLeftThumbX(this), InputGamepadGetLeftThumbDeadZone(this));
		}

		int16_t GetLeftThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedLeftThumbY() const noexcept {
			return m_normalize<T>(InputGamepadGetLeftThumbY(this), InputGamepadGetLeftThumbDeadZone(this));
		}

		int16_t GetRightThumbX() const noexcept;
		template <std::floating_point T>
		T GetNormalizedRightThumbX() const noexcept {
			return m_normalize<T>(InputGamepadGetRightThumbX(this), InputGamepadGetRightThumbDeadZone(this));
		}

		int16_t GetRightThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedRightThumbY() const noexcept {
			return m_normalize<T>(InputGamepadGetRightThumbY(this), InputGamepadGetRightThumbDeadZone(this));
		}

		int16_t GetPrevLeftThumbX() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevLeftThumbX() const noexcept {
			return m_normalize<T>(InputGamepadGetPrevLeftThumbX(this), InputGamepadGetLeftThumbDeadZone(this));
		}

		int16_t GetPrevLeftThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevLeftThumbY() const noexcept {
			return m_normalize<T>(InputGamepadGetPrevLeftThumbY(this), InputGamepadGetLeftThumbDeadZone(this));
		}

		int16_t GetPrevRightThumbX() const noexcept;		
		template <std::floating_point T>
		T GetNormalizedPrevRightThumbX() const noexcept {
			return m_normalize<T>(InputGamepadGetPrevRightThumbX(this), InputGamepadGetRightThumbDeadZone(this));
		}

		int16_t GetPrevRightThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevRightThumbY() const noexcept {
			return m_normalize<T>(InputGamepadGetPrevRightThumbY(this), InputGamepadGetRightThumbDeadZone(this));
		}

		uint8_t GetLeftTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedLeftTrigger() const noexcept {
			return m_normalize<T>(InputGamepadGetLeftTrigger(this), InputGamepadGetTriggerThreshold(this));
		}

		uint8_t GetRightTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedRightTrigger() const noexcept {
			return m_normalize<T>(InputGamepadGetRightTrigger(this), InputGamepadGetTriggerThreshold(this));
		}

		uint8_t GetPrevLeftTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevLeftTrigger() const noexcept {
			return m_normalize<T>(InputGamepadGetPrevLeftTrigger(this), InputGamepadGetTriggerThreshold(this));
		}

		uint8_t GetPrevRightTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevRightTrigger() const noexcept {
			return m_normalize<T>(InputGamepadGetPrevRightTrigger(this), InputGamepadGetTriggerThreshold(this));
		}

		void SetLeftThumbDeadZone(int16_t deadZone) noexcept;
		void SetRightThumbDeadZone(int16_t deadZone) noexcept;
		void SetTriggerThreshold(uint8_t threshold) noexcept;

		int16_t GetLeftThumbDeadZone() const noexcept;
		int16_t GetRightThumbDeadZone() const noexcept;
		uint8_t GetTriggerThreshold() const noexcept;

		bool IsConnected() const noexcept;
	private:
		template <std::floating_point To, std::integral From>
		To m_normalize(From from, From min = 0, From max = std::numeric_limits<From>::max()) const noexcept {
			assert(min >= 0 && max > min);
			if constexpr (std::is_signed_v<From>) {
				if(-min <= from && from <= min) return To(0);
			}
			else {
				if (from <= min) return To(0);
			}
			return std::clamp(
				static_cast<To>(from - (from >= 0 ? min : -min))
				/ (static_cast<To>(max) - static_cast<To>(min)),
				To(-1.0), To(1.0));
		}
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
