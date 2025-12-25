#pragma once
#include <memory>
#include <concepts>
#include <limits>
#include <cmath>
#include <cassert>
#include <spdlog/logger.h>
#include "../macros/dll.hpp"

namespace PameECS::Input {
	class Gamepad {
	public:
		Gamepad(std::shared_ptr<spdlog::logger> logger,
			int id = -1 // Auto: id < 0 or 3 < id
		);
		~Gamepad();
		Gamepad(const Gamepad&) = delete;
		Gamepad& operator=(const Gamepad&) = delete;

		void Update();

		PECS_DLL_SHARED bool IsButtonDown(uint16_t button) const noexcept;
		PECS_DLL_SHARED bool WasButtonPressed(uint16_t button) const noexcept;
		PECS_DLL_SHARED bool WasButtonReleased(uint16_t button) const noexcept;

		PECS_DLL_SHARED int16_t GetLeftThumbX() const noexcept;
		template <std::floating_point T>
		T GetNormalizedLeftThumbX() const noexcept {
			return m_normalize<T>(GetLeftThumbX(), GetLeftThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetLeftThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedLeftThumbY() const noexcept {
			return m_normalize<T>(GetLeftThumbY(), GetLeftThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetRightThumbX() const noexcept;
		template <std::floating_point T>
		T GetNormalizedRightThumbX() const noexcept {
			return m_normalize<T>(GetRightThumbX(), GetRightThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetRightThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedRightThumbY() const noexcept {
			return m_normalize<T>(GetRightThumbY(), GetRightThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetPrevLeftThumbX() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevLeftThumbX() const noexcept {
			return m_normalize<T>(GetPrevLeftThumbX(), GetLeftThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetPrevLeftThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevLeftThumbY() const noexcept {
			return m_normalize<T>(GetPrevLeftThumbY(), GetLeftThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetPrevRightThumbX() const noexcept;		
		template <std::floating_point T>
		T GetNormalizedPrevRightThumbX() const noexcept {
			return m_normalize<T>(GetPrevRightThumbX(), GetRightThumbDeadZone());
		}

		PECS_DLL_SHARED int16_t GetPrevRightThumbY() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevRightThumbY() const noexcept {
			return m_normalize<T>(GetPrevRightThumbY(), GetRightThumbDeadZone());
		}

		PECS_DLL_SHARED uint8_t GetLeftTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedLeftTrigger() const noexcept {
			return m_normalize<T>(GetLeftTrigger(), GetTriggerThreshold());
		}

		PECS_DLL_SHARED uint8_t GetRightTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedRightTrigger() const noexcept {
			return m_normalize<T>(GetRightTrigger(), GetTriggerThreshold());
		}

		PECS_DLL_SHARED uint8_t GetPrevLeftTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevLeftTrigger() const noexcept {
			return m_normalize<T>(GetPrevLeftTrigger(), GetTriggerThreshold());
		}

		PECS_DLL_SHARED uint8_t GetPrevRightTrigger() const noexcept;
		template <std::floating_point T>
		T GetNormalizedPrevRightTrigger() const noexcept {
			return m_normalize<T>(GetPrevRightTrigger(), GetTriggerThreshold());
		}

		PECS_DLL_SHARED void SetLeftThumbDeadZone(int16_t deadZone) noexcept;
		PECS_DLL_SHARED void SetRightThumbDeadZone(int16_t deadZone) noexcept;
		PECS_DLL_SHARED void SetTriggerThreshold(uint8_t threshold) noexcept;

		PECS_DLL_SHARED int16_t GetLeftThumbDeadZone() const noexcept;
		PECS_DLL_SHARED int16_t GetRightThumbDeadZone() const noexcept;
		PECS_DLL_SHARED uint8_t GetTriggerThreshold() const noexcept;

		PECS_DLL_SHARED bool IsConnected() const noexcept;
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
