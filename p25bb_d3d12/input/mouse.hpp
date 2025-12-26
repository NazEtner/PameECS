#pragma once
#include "../macros/dll.hpp"
#include <memory>
#include <string>
#include <windows.h>

namespace PameECS::Input {
	class Mouse;
}

extern "C" {
	PECS_DLL_SHARED bool InputMouseIsLeftButtonDown(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool InputMouseIsRightButtonDown(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool InputMouseIsMiddleButtonDown(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED bool InputMouseWasLeftButtonPressed(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool InputMouseWasRightButtonPressed(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool InputMouseWasMiddleButtonPressed(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED bool InputMouseWasLeftButtonReleased(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool InputMouseWasRightButtonReleased(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool InputMouseWasMiddleButtonReleased(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED int InputMouseGetWheelDelta(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED float InputMouseGetCursorPositionX(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED float InputMouseGetCursorPositionY(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED bool InputMouseIsMouseInWindow(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED void InputMouseAddRect(
		PameECS::Input::Mouse* mouse,
		const char* name,
		float left, float top,
		float width, float height);

	PECS_DLL_SHARED void InputMouseRemoveRect(PameECS::Input::Mouse* mouse, const char* name);

	PECS_DLL_SHARED void InputMouseClearRect(PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED const char* InputMouseGetHoveredRectName(const PameECS::Input::Mouse* mouse);
}

namespace PameECS::Input {
	class Mouse {
	public:
		Mouse(HWND windowHandle);
		~Mouse();
		Mouse(const Mouse&) = delete;
		Mouse& operator=(const Mouse&) = delete;

		// 他の書き込みがあるAPIとUpdateは同時には呼ばれないはず
		// 同時に呼ばれてたらバグ
		void Update();

		bool IsLeftButtonDown() const;
		bool IsRightButtonDown() const;
		bool IsMiddleButtonDown() const;

		bool WasLeftButtonPressed() const;
		bool WasRightButtonPressed() const;
		bool WasMiddleButtonPressed() const;

		bool WasLeftButtonReleased() const;
		bool WasRightButtonReleased() const;
		bool WasMiddleButtonReleased() const;

		int GetWheelDelta() const;
		float GetCursorPositionX() const;
		float GetCursorPositionY() const;

		bool IsMouseInWindow() const;

		void AddRect(
			const std::string& name,
			float left, float top,
			float width, float height
		) {
			InputMouseAddRect(this, name.c_str(), left, top, width, height);
		}

		void AddRect(
			const char* name,
			float left, float top,
			float width, float height);

		void RemoveRect(const std::string& name) {
			InputMouseRemoveRect(this, name.c_str());
		}
		void RemoveRect(const char* name);

		void ClearRect();

		bool GetHoveredRectName(std::string& name) const {
			if (auto hovered = InputMouseGetHoveredRectName(this); hovered) {
				name = hovered;
				return true;
			}

			return false;
		}
		const char* GetHoveredRectName() const;

		void OnMouseDelta(int delta);
	private:
		bool m_commonPressed(size_t index) const;
		bool m_commonReleased(size_t index) const;
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
