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
	class WindowSettingAdapter final {
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
		WindowSettingAdapter();
		// プロシージャは呼び出し側で設定すること
		Window::Properties CreateWindowProperty(const Configs::WindowConfig& config);
		void SetWindow(std::shared_ptr<Window>& window) { // 引数にconstをつけられそうだけど、意図と合わないからつけない
			m_window = window;
		}
		void SetStyle(const std::string& style);
		// sizeIgnoresのスタイル中は、次にサイズを持つスタイルへ戻ったときのサイズとして保存だけする
		void SetWidth(const uint32_t width);
		void SetHeight(const uint32_t height);
		void SetAltEnterSwitchables(const std::vector<std::string>& switchables);
		// altEnterSwitchablesの中で、今のスタイルの次のスタイルに切り替える
		void SwitchToNextAltEnterStyle();
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

		bool m_isSizeIgnored(const std::string& style) const {
			return m_config.sizeIgnores.contains(style);
		}

		// sizeIgnoresのスタイルのときに使うモニタの矩形
		// ウィンドウができていればウィンドウが一番多く乗っているモニタ、なければプライマリモニタ
		RECT m_getMonitorRect(bool workArea = false) const;
		// サイズを持つスタイルのときのクライアントサイズ
		// 設定値がモニタの作業領域に収まらなければデフォルトサイズにする
		std::pair<uint32_t, uint32_t> m_getWindowedSize(DWORD style) const;
		// サイズを持つスタイルのときに、ドラッグで変わったサイズと位置を覚えておく
		void m_captureWindowedState();
		// ファイルシステムにはFile::File<1, 0>ではなく、fstreamを使う
		const std::filesystem::path m_setting_file_path = "window_setting.json";
		// width, heightはサイズを持つスタイルのときのクライアントサイズで、sizeIgnoresのスタイル中は書き換えない
		WindowSetting m_setting;
		Configs::WindowConfig m_config;
		std::shared_ptr<Window> m_window;
		// フルスクリーンから戻ったときに元の位置に戻すため (起動をまたいでは覚えない)
		std::optional<POINT> m_windowed_position;
	};
}
