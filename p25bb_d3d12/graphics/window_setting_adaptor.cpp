#include "window_setting_adaptor.hpp"

using PameECS::Graphics::WindowSettingAdaptor;
using PameECS::Graphics::Window;

WindowSettingAdaptor::WindowSettingAdaptor() {
	if (std::filesystem::exists(m_setting_file_path)) {
		std::ifstream ifs(m_setting_file_path);
		nlohmann::json json;
		ifs >> json;
		m_setting.Deserialize(json);
	}
}

Window::Properties WindowSettingAdaptor::CreateWindowProperty(const Configs::WindowConfig& config) {
	Window::Properties ret;

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

	bool sizeIgnore = config.sizeIgnores.contains(m_setting.windowStyle);

	if (!sizeIgnore) {
		if (m_setting.width == 0xFFFFFFFF) {
			ret.width = config.defaultWidth;
		}
		else {
			ret.width = m_setting.width;
		}

		if (m_setting.height == 0xFFFFFFFF) {
			ret.height = config.defaultHeight;
		}
		else {
			ret.height = m_setting.height;
		}
	}
	else {
		HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {};
		mi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &mi)) {
			ret.width = mi.rcMonitor.right - mi.rcMonitor.left;
			ret.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}
		else {
			ret.width = GetSystemMetrics(SM_CXSCREEN);
			ret.height = GetSystemMetrics(SM_CYSCREEN);
		}
	}
	m_setting.width = ret.width.value();
	m_setting.height = ret.height.value();

	ret.className = config.className;
	ret.windowName = config.windowName;

	m_config = config;

	m_save();

	return ret;
}

void WindowSettingAdaptor::SetStyle(const std::string& style) {
	Window::Properties properties;
	auto it = m_config.windowStyles.find(style);
	if (it != m_config.windowStyles.end()) {
		properties.windowStyle = it->second;
		m_setting.windowStyle = style;
		m_save();
	}

	assert(m_window);
	m_window->SetProperties(properties);
}

void WindowSettingAdaptor::SetWidth(const uint32_t width) {
	Window::Properties properties;
	properties.width = width;
	m_setting.width = width;
	m_save();

	assert(m_window);
	m_window->SetProperties(properties);
}

void WindowSettingAdaptor::SetHeight(const uint32_t height) {
	Window::Properties properties;
	properties.height = height;
	m_setting.height = height;
	m_save();

	assert(m_window);
	m_window->SetProperties(properties);
}
