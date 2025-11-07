#include "window.hpp"
#include "../exceptions/window_error.hpp"
#include "../helpers/errors/windows.hpp"

using PameECS::Graphics::Window;

Window::Window(const Properties& properties, std::shared_ptr<spdlog::logger>& logger) :
	IWindow(), m_logger(logger) {
	m_setDefaultProperties(properties);
	m_create();
}

Window::~Window() {
	Destroy();

	auto instanceOrError = m_getInstanceHandle();
	HINSTANCE hInstance = NULL;
	if (std::holds_alternative<HINSTANCE>(instanceOrError)) {
		hInstance = std::get<HINSTANCE>(instanceOrError);
	}
	else {
		// HINSTANCEを持たないなら、std::string意外に持てる物はない
		m_logger->error(std::get<std::string>(instanceOrError));
		return;
	}

	// 少なくともm_setDefaultPropertiesはされてるはずだから大丈夫なはず
	if (!UnregisterClass(m_properties.className.value().c_str(), hInstance)) {
		std::string message = std::string("Failed to unregister window class : ") + Helpers::Errors::Windows::GetLastErrorMessage();
		m_logger->error(message);
	}
}

void Window::Show() {
	if (ShowWindow(m_window_handle, SW_SHOW) == FALSE) {
		throw Exceptions::WindowError(std::string("ShowWindow() failed: ") + Helpers::Errors::Windows::GetLastErrorMessage());
	}

	if (UpdateWindow(m_window_handle) == FALSE) {
		throw Exceptions::WindowError(std::string("UpdateWindow() failed: ") + Helpers::Errors::Windows::GetLastErrorMessage());
	}
}

bool Window::Update() {
	MSG messageInfo;
	if (PeekMessage(&messageInfo, nullptr, 0, 0, PM_REMOVE) == 0)
		return true;
	if (messageInfo.message == WM_QUIT)
		return false;

	TranslateMessage(&messageInfo);
	DispatchMessage(&messageInfo);

	return true;
}

void Window::Destroy() noexcept {
	if (m_window_handle) {
		if (!DestroyWindow(m_window_handle)) {
			std::string message = std::string("Failed to destroy window : ") + Helpers::Errors::Windows::GetLastErrorMessage();
			m_logger->error(message);
		}

		m_window_handle = nullptr;
	}
}

void Window::m_setDefaultProperties(const Properties& properties) {
	m_properties.className = properties.className.value_or("DefaultWindowClassName");
	m_properties.windowName = properties.windowName.value_or("DefaultWindowName");
	m_properties.width = properties.width.value_or(800);
	m_properties.height = properties.height.value_or(600);
	m_properties.windowStyle = properties.windowStyle.value_or(WS_OVERLAPPEDWINDOW | WS_VISIBLE);
	m_properties.windowProcedure = properties.windowProcedure.value_or(DefWindowProc);
}

void Window::m_create() {
	assert(m_properties.className.has_value());
	assert(m_properties.windowName.has_value());
	assert(m_properties.width.has_value());
	assert(m_properties.height.has_value());
	assert(m_properties.windowStyle.has_value());
	assert(m_properties.windowProcedure.has_value());

	auto instanceOrError = m_getInstanceHandle();
	HINSTANCE hInstance = NULL;
	if (std::holds_alternative<HINSTANCE>(instanceOrError)) {
		hInstance = std::get<HINSTANCE>(instanceOrError);
	}
	else {
		// HINSTANCEを持たないなら、std::string意外に持てる物はない
		throw Exceptions::WindowError(std::get<std::string>(instanceOrError));
	}

	WNDCLASS windowClass;
	ZeroMemory(&windowClass, sizeof(windowClass));

	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = m_properties.className.value().c_str();
	windowClass.lpfnWndProc = m_properties.windowProcedure.value();
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

	if (RegisterClass(&windowClass) == 0) {
		std::string message = std::string("Failed to register window class: ") + Helpers::Errors::Windows::GetLastErrorMessage();
		throw Exceptions::WindowError(message.c_str());
	}

	m_style = m_properties.windowStyle.value();
	RECT windowRect;
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.right = m_properties.width.value();
	windowRect.bottom = m_properties.height.value();

	if (AdjustWindowRect(&windowRect, m_style, FALSE) == 0) {
		std::string message = std::string("Failed to adjust window rect: ") + Helpers::Errors::Windows::GetLastErrorMessage();
		throw Exceptions::WindowError(message.c_str());
	}

	m_window_handle = CreateWindowEx(
		0,
		m_properties.className.value().c_str(),
		m_properties.windowName.value().c_str(),
		m_style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (m_window_handle == nullptr) {
		std::string message = std::string("Failed to create window: ") + Helpers::Errors::Windows::GetLastErrorMessage();
		throw Exceptions::WindowError(message.c_str());
	}

	SetWindowLongPtr(m_window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}
