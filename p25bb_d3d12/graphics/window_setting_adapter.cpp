#include "window_setting_adapter.hpp"
#include <algorithm>

using PameECS::Graphics::WindowSettingAdapter;
using PameECS::Graphics::Window;

WindowSettingAdapter::WindowSettingAdapter() {
	if (std::filesystem::exists(m_setting_file_path)) {
		std::ifstream ifs(m_setting_file_path);
		nlohmann::json json;
		ifs >> json;
		m_setting.Deserialize(json);
	}
}

Window::Properties WindowSettingAdapter::CreateWindowProperty(const Configs::WindowConfig& config) {
	Window::Properties ret;
	m_config = config;
	{
		auto settingValueIt = config.windowStyles.find(m_setting.windowStyle);
		if (settingValueIt != config.windowStyles.end()) {
			ret.windowStyle = settingValueIt->second;
		}
		else {
			auto configValueIt = config.windowStyles.find(config.defaultWindowStyle);
			if (configValueIt != config.windowStyles.end()) {
				ret.windowStyle = configValueIt->second;
				m_setting.windowStyle = configValueIt->first;
			}
			// どちらにも見つからなかった場合は、何もしない（std::nulloptのままにして、Windowのデフォルトを使う）
		}
	}

	if (m_isSizeIgnored(m_setting.windowStyle)) {
		RECT monitor = m_getMonitorRect();
		ret.width = static_cast<uint32_t>(monitor.right - monitor.left);
		ret.height = static_cast<uint32_t>(monitor.bottom - monitor.top);
		ret.x = monitor.left;
		ret.y = monitor.top;
	}
	else {
		auto [width, height] = m_getWindowedSize(ret.windowStyle.value_or(WS_OVERLAPPEDWINDOW | WS_VISIBLE));
		ret.width = width;
		ret.height = height;
		m_setting.width = width;
		m_setting.height = height;
	}

	ret.className = config.className;
	ret.windowName = config.windowName;

	m_save();

	return ret;
}

void WindowSettingAdapter::SetStyle(const std::string& style) {
	auto it = m_config.windowStyles.find(style);
	if (it == m_config.windowStyles.end()) {
		return;
	}

	assert(m_window);
	m_captureWindowedState();

	Window::Properties properties;
	properties.windowStyle = it->second;
	if (m_isSizeIgnored(style)) {
		RECT monitor = m_getMonitorRect();
		properties.width = static_cast<uint32_t>(monitor.right - monitor.left);
		properties.height = static_cast<uint32_t>(monitor.bottom - monitor.top);
		properties.x = monitor.left;
		properties.y = monitor.top;
	}
	else {
		auto [width, height] = m_getWindowedSize(it->second);
		properties.width = width;
		properties.height = height;
		m_setting.width = width;
		m_setting.height = height;

		RECT workArea = m_getMonitorRect(true);
		if (m_windowed_position) {
			properties.x = m_windowed_position->x;
			properties.y = m_windowed_position->y;
		}
		else {
			// 覚えている位置がない (フルスクリーンで起動した) ときは作業領域の中央に置く
			RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
			AdjustWindowRect(&rect, it->second, FALSE);
			properties.x = workArea.left + ((workArea.right - workArea.left) - (rect.right - rect.left)) / 2;
			properties.y = workArea.top + ((workArea.bottom - workArea.top) - (rect.bottom - rect.top)) / 2;
		}
	}

	m_setting.windowStyle = style;
	m_save();

	m_window->SetProperties(properties);
}

void WindowSettingAdapter::SetWidth(const uint32_t width) {
	m_setting.width = width;
	m_save();

	if (m_isSizeIgnored(m_setting.windowStyle)) return;

	Window::Properties properties;
	properties.width = width;
	assert(m_window);
	m_window->SetProperties(properties);
}

void WindowSettingAdapter::SetHeight(const uint32_t height) {
	m_setting.height = height;
	m_save();

	if (m_isSizeIgnored(m_setting.windowStyle)) return;

	Window::Properties properties;
	properties.height = height;
	assert(m_window);
	m_window->SetProperties(properties);
}

void WindowSettingAdapter::SetAltEnterSwitchables(const std::vector<std::string>& switchables) {
	m_config.altEnterSwitchables = switchables;
}

void WindowSettingAdapter::SwitchToNextAltEnterStyle() {
	const auto& switchables = m_config.altEnterSwitchables;
	if (switchables.empty()) return;

	auto it = std::find(switchables.begin(), switchables.end(), m_setting.windowStyle);
	// 今のスタイルが一覧になければ先頭へ
	if (it == switchables.end() || ++it == switchables.end()) {
		it = switchables.begin();
	}
	SetStyle(*it);
}

RECT WindowSettingAdapter::m_getMonitorRect(bool workArea) const {
	HMONITOR hMonitor = nullptr;
	if (m_window && m_window->GetWindowHandle()) {
		hMonitor = MonitorFromWindow(m_window->GetWindowHandle(), MONITOR_DEFAULTTONEAREST);
	}
	else {
		hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
	}

	MONITORINFO mi = {};
	mi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(hMonitor, &mi)) {
		return workArea ? mi.rcWork : mi.rcMonitor;
	}
	return { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
}

std::pair<uint32_t, uint32_t> WindowSettingAdapter::m_getWindowedSize(DWORD style) const {
	uint32_t width = m_setting.width != 0xFFFFFFFF ? m_setting.width : m_config.defaultWidth;
	uint32_t height = m_setting.height != 0xFFFFFFFF ? m_setting.height : m_config.defaultHeight;

	// 以前はフルスクリーン中にモニタサイズが保存されていたので、そのままだとタイトルバーや下端が画面外に出る
	RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	RECT workArea = m_getMonitorRect(true);
	if (AdjustWindowRect(&rect, style, FALSE) &&
		(rect.right - rect.left > workArea.right - workArea.left ||
		 rect.bottom - rect.top > workArea.bottom - workArea.top)) {
		width = m_config.defaultWidth;
		height = m_config.defaultHeight;
	}

	return { width, height };
}

void WindowSettingAdapter::m_captureWindowedState() {
	if (!m_window || m_isSizeIgnored(m_setting.windowStyle)) return;

	HWND hWnd = m_window->GetWindowHandle();
	// 最大化・最小化中のサイズは覚えても仕方がない
	if (!hWnd || IsZoomed(hWnd) || IsIconic(hWnd)) return;

	auto current = m_window->GetProperties(Window::NoClassName | Window::NoWindowName | Window::NoWindowStyle);
	if (current.width && current.height) {
		m_setting.width = current.width.value();
		m_setting.height = current.height.value();
	}
	if (current.x && current.y) {
		m_windowed_position = POINT{ current.x.value(), current.y.value() };
	}
}
