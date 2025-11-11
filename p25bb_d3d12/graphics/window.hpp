#pragma once
#include <graphics/window_interface.hpp>
#include <optional>
#include <string>
#include <windows.h>
#include <variant>
#include <spdlog/spdlog.h>

#include "../helpers/errors/windows.hpp"

namespace PameECS::Graphics {
	class Window : public Pame::Graphics::IWindow {
	public:
		enum PropertyGetFlags : uint32_t {
			None = 0,
			NoClassName = 1 << 0,
			NoWindowName = 1 << 1,
			NoWidth = 1 << 2,
			NoHeight = 1 << 3,
			NoWindowStyle = 1 << 4,
			// ウィンドウプロシージャの関数ポインタを取得する意味がないので取得させない
		};

		struct Properties {
			// コンストラクタ呼び出しでのみ使用可能、nulloptであれば"DefaultWindowClassName"
			std::optional<std::string> className;
			//  nulloptであれば"DefaultWindowName"
			std::optional<std::string> windowName;
			// nulloptであれば800
			std::optional<uint32_t> width;
			// nulloptであれば600
			std::optional<uint32_t> height;
			// nulloptであればWS_OVERLAPPEDWINDOW | WS_VISIBLE
			std::optional<DWORD> windowStyle;
			// コンストラクタ呼び出しでのみ使用可能、nulloptであればDefWindowProc
			// 値を入れるならDestroyWindow()を使用せずにthis->Destroy()を使うか、WM_CLOSEでPostQuitMessage(0)を呼ぶこと
			// 値を入れなければDestroyWindow()が二回呼ばれてしまって、Windowsのバグとかで変な挙動をする可能性もあるから、何かしらのプロシージャを入れるのを推奨
			std::optional<WNDPROC> windowProcedure;
		};

		Window(const Properties& properties, std::shared_ptr<spdlog::logger>& logger);
		virtual ~Window();

		void Show() override;
		bool Update() override;

		Properties GetProperties(const uint32_t flags = None) {
			Properties result;

			if (m_window_handle && !(flags & NoWidth || flags & NoHeight)) {
				RECT rect;
				if (GetClientRect(m_window_handle, &rect)) {
					m_properties.width = static_cast<int>(rect.right - rect.left);
					m_properties.height = static_cast<int>(rect.bottom - rect.top);
				}
			}

			if (!(flags & NoClassName)) {
				result.className = m_properties.className;
			}
			if (!(flags & NoWindowName)) {
				result.windowName = m_properties.windowName;
			}
			if (!(flags & NoWidth)) {
				result.width = m_properties.width;
			}
			if (!(flags & NoHeight)) {
				result.height = m_properties.height;
			}
			if (!(flags & NoWindowStyle)) {
				result.windowStyle = m_properties.windowStyle;
			}
			return result;
		}

		// ウィンドウクラス名とウィンドウプロシージャは設定できないことに注意
		void SetProperties(const Properties& property);

		HWND GetWindowHandle() noexcept {
			return m_window_handle;
		}

		void Destroy() noexcept; // DestroyWindow()のラッパー的なやつ
	private:
		void m_setDefaultProperties(const Properties& properties);
		void m_create();
		std::variant<HINSTANCE, std::string> m_getInstanceHandle() {
			HINSTANCE hInstance = GetModuleHandle(nullptr);
			if (hInstance == NULL) {
				std::string message = std::string("hInstance is NULL : ") + Helpers::Errors::Windows::GetLastErrorMessage();
				return message;
			}

			return hInstance;
		}
		HWND m_window_handle = nullptr;
		Properties m_properties;
		std::shared_ptr<spdlog::logger> m_logger;
	};
}
