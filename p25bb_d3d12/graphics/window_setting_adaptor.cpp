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

	auto [calculatedWidth, calculatedHeight] = m_calculateWindowSize(m_setting.windowStyle, m_setting);
	ret.width = calculatedWidth;
	ret.height = calculatedHeight;
	m_setting.width = ret.width.value();
	m_setting.height = ret.height.value();

	ret.className = config.className;
	ret.windowName = config.windowName;

	ret.altEnterSwitchables = m_convertStyleNamesToStyles(config.altEnterSwitchables);

	m_save();

	return ret;
}

void WindowSettingAdaptor::SetStyle(const std::string& style) {
	Window::Properties properties;
	auto it = m_config.windowStyles.find(style);
	if (it != m_config.windowStyles.end()) {
		properties.windowStyle = it->second;
		m_setting.windowStyle = style;
		auto [calculatedWidth, calculatedHeight] = m_calculateWindowSize(style, m_setting);
		properties.width = calculatedWidth;
		properties.height = calculatedHeight;
		m_setting.width = calculatedWidth;
		m_setting.height = calculatedHeight;
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

void WindowSettingAdaptor::SetAltEnterSwitchables(const std::vector<std::string>& switchables) {
	Window::Properties properties;
	properties.altEnterSwitchables = m_convertStyleNamesToStyles(switchables);
	assert(m_window);
	m_window->SetProperties(properties);
}

std::pair<uint32_t, uint32_t> WindowSettingAdaptor::m_calculateWindowSize(const std::string& style, const WindowSetting& setting) const {
	uint32_t width = 0;
	uint32_t height = 0;

	auto it = m_config.sizeIgnores.find(style);
	if (it == m_config.sizeIgnores.end()) {
		if (setting.width != 0xFFFFFFFF) {
			width = setting.width;
		}
		else {
			width = m_config.defaultWidth;
		}
		if (setting.height != 0xFFFFFFFF) {
			height = setting.height;
		}
		else {
			height = m_config.defaultHeight;
		}
	}
	else {
		HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {};
		mi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &mi)) {
			width = mi.rcMonitor.right - mi.rcMonitor.left;
			height = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}
		else {
			width = GetSystemMetrics(SM_CXSCREEN);
			height = GetSystemMetrics(SM_CYSCREEN);
		}
	}

	return { width, height };
}
