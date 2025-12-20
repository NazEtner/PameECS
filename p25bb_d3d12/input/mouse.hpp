#pragma once
#include "../macros/dll.hpp"
#include <memory>
#include <windows.h>

namespace PameECS::Input {
	class Mouse {
	public:
		Mouse(HWND windowHandle);
		~Mouse();
		Mouse(const Mouse&) = delete;
		Mouse& operator=(const Mouse&) = delete;

		void Update();

		PECS_DLL_SHARED bool IsLeftButtonDown() const;
		PECS_DLL_SHARED bool IsRightButtonDown() const;
		PECS_DLL_SHARED bool IsMiddleButtonDown() const;

		PECS_DLL_SHARED bool WasLeftButtonPressed() const;
		PECS_DLL_SHARED bool WasRightButtonPressed() const;
		PECS_DLL_SHARED bool WasMiddleButtonPressed() const;

		PECS_DLL_SHARED bool WasLeftButtonReleased() const;
		PECS_DLL_SHARED bool WasRightButtonReleased() const;
		PECS_DLL_SHARED bool WasMiddleButtonReleased() const;

		PECS_DLL_SHARED int GetWheelDelta() const;
		PECS_DLL_SHARED float GetCursorPositionX() const;
		PECS_DLL_SHARED float GetCursorPositionY() const;

		PECS_DLL_SHARED bool IsMouseInWindow() const;

		PECS_DLL_SHARED void AddRect(
			const char* name,
			float left, float top,
			float width, float height);

		PECS_DLL_SHARED void RemoveRect(const char* name);

		PECS_DLL_SHARED const char* GetHoveredRectName() const;

		void OnMouseDelta(int delta);
	private:
		bool m_commonPressed(size_t index) const;
		bool m_commonReleased(size_t index) const;
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
