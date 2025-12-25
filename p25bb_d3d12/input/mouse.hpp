#pragma once
#include "../macros/dll.hpp"
#include <memory>
#include <string>
#include <windows.h>

namespace PameECS::Input {
	class Mouse;
}

extern "C" {
	PECS_DLL_SHARED bool __stdcall InputMouseIsLeftButtonDown(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool __stdcall InputMouseIsRightButtonDown(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool __stdcall InputMouseIsMiddleButtonDown(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED bool __stdcall InputMouseWasLeftButtonPressed(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool __stdcall InputMouseWasRightButtonPressed(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool __stdcall InputMouseWasMiddleButtonPressed(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED bool __stdcall InputMouseWasLeftButtonReleased(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool __stdcall InputMouseWasRightButtonReleased(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED bool __stdcall InputMouseWasMiddleButtonReleased(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED int __stdcall InputMouseGetWheelDelta(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED float __stdcall InputMouseGetCursorPositionX(const PameECS::Input::Mouse* mouse);
	PECS_DLL_SHARED float __stdcall InputMouseGetCursorPositionY(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED bool __stdcall InputMouseIsMouseInWindow(const PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED void __stdcall InputMouseAddRect(
		PameECS::Input::Mouse* mouse,
		const char* name,
		float left, float top,
		float width, float height);

	PECS_DLL_SHARED void __stdcall InputMouseRemoveRect(PameECS::Input::Mouse* mouse, const char* name);

	PECS_DLL_SHARED void __stdcall InputMouseClearRect(PameECS::Input::Mouse* mouse);

	PECS_DLL_SHARED const char* __stdcall InputMouseGetHoveredRectName(const PameECS::Input::Mouse* mouse);
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
