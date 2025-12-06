#pragma once
#include "window.hpp"
#include "../configs/window_config.hpp"
#include "../json/json_object.hpp"
#include "../exceptions/file_error.hpp"
#include <memory>
#include <optional>
#include <filesystem>
#include <fstream>

namespace PameECS::Graphics {
	class WindowSettingAdaptor final {
		struct WindowSetting : JSON::JSONObject<WindowSetting> {
			std::string windowStyle = "_defaultWindowStyleName";
			uint32_t width = 0xFFFFFFFF;
			uint32_t height = 0xFFFFFFFF;
			template<typename Func>
			void ForEachMember(Func&& func) {
				func.operator()<"windowStyle">(windowStyle);
				func.operator()<"width">(width);
				func.operator()<"height">(height);
			}
		};
	public:
		WindowSettingAdaptor();
		// プロシージャは呼び出し側で設定すること
		Window::Properties CreateWindowProperty(const Configs::WindowConfig& config);
		void SetWindow(std::shared_ptr<Window>& window) { // 引数にconstをつけられそうだけど、意図と合わないからつけない
			m_window = window;
		}
		void SetStyle(const std::string& style);
		void SetWidth(const uint32_t width);
		void SetHeight(const uint32_t height);
	private:
		void m_save() {
			try {
				auto json = m_setting.Serialize();
				std::ofstream ofs(m_setting_file_path);
				ofs << json;
			}
			catch (const std::exception& e) {
				throw Exceptions::FileError(std::string("Failed to save setting file : ") + e.what());
			}
		}
		// ファイルシステムにはFile::File<1, 0>ではなく、fstreamを使う
		const std::filesystem::path m_setting_file_path = "window_setting.json";
		WindowSetting m_setting;
		Configs::WindowConfig m_config;
		std::shared_ptr<Window> m_window;
	};
}
