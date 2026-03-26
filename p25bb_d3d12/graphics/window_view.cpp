#include "window_view.hpp"

using PameECS::Graphics::WindowView;
using PameECS::Graphics::Window;
using PameECS::Graphics::WindowSettingAdapter;
using PameECS::Graphics::Renderer;

extern "C" {
	PECS_DLL_SHARED void GraphicsWindowViewSetVSyncEnabled(PameECS::Graphics::WindowView* windowView, bool isVSyncEnabled) {
		windowView->SetVSyncEnabled(isVSyncEnabled);
	}
	PECS_DLL_SHARED bool GraphicsWindowViewIsVSyncEnabled(const PameECS::Graphics::WindowView* windowView) {
		return windowView->IsVSyncEnabled();
	}
	PECS_DLL_SHARED void GraphicsWindowViewSetStyle(PameECS::Graphics::WindowView* windowView, const char* name, size_t nameSize) {
		windowView->SetStyle(name, nameSize);
	}
	PECS_DLL_SHARED void GraphicsWindowViewSetWidth(PameECS::Graphics::WindowView* windowView, uint32_t width) {
		windowView->SetWidth(width);
	}
	PECS_DLL_SHARED uint32_t GraphicsWindowViewGetWidth(const PameECS::Graphics::WindowView* windowView) {
		return windowView->GetWidth();
	}
	PECS_DLL_SHARED void GraphicsWindowViewSetHeight(PameECS::Graphics::WindowView* windowView, uint32_t height) {
		windowView->SetHeight(height);
	}
	PECS_DLL_SHARED uint32_t GraphicsWindowViewGetHeight(const PameECS::Graphics::WindowView* windowView) {
		return windowView->GetHeight();
	}
	PECS_DLL_SHARED void GraphicsWindowViewSetProtectedContentEnabled(PameECS::Graphics::WindowView* windowView, bool enabled, bool excludeFromCapture) {
		windowView->SetProtectedContentEnabled(enabled, excludeFromCapture);
	}
}

struct WindowView::Impl {
	std::shared_ptr<Window> window;
	std::shared_ptr<WindowSettingAdapter> adapter;
	std::shared_ptr<Renderer> renderer;
};

WindowView::WindowView(
	std::shared_ptr<Window>& window,
	std::shared_ptr<WindowSettingAdapter> adapter,
	std::shared_ptr<Renderer> renderer) {
	m_impl = std::make_unique<Impl>();
	m_impl->window = window;
	m_impl->adapter = adapter;
	m_impl->renderer = renderer;
}

WindowView::~WindowView() {

}

void WindowView::SetVSyncEnabled(const bool isVSyncEnabled) {
	m_impl->renderer->EnableVerticalSync(isVSyncEnabled);
}

bool WindowView::IsVSyncEnabled() const {
	return m_impl->renderer->EnableVerticalSync(nullptr);
}

void WindowView::SetStyle(const char* name, size_t nameSize) {
	std::string styleName(name, nameSize);
	m_impl->adapter->SetStyle(styleName);
}

void WindowView::SetWidth(const uint32_t width) {
	m_impl->adapter->SetWidth(width);
}

uint32_t WindowView::GetWidth() const {
	auto properties = m_impl->window->GetProperties();
	if (properties.width) {
		return properties.width.value();
	}
	return 0;
}

void WindowView::SetHeight(const uint32_t height) {
	m_impl->adapter->SetHeight(height);
}

uint32_t WindowView::GetHeight() const {
	auto properties = m_impl->window->GetProperties();
	if (properties.height) {
		return properties.height.value();
	}
	return 0;
}

void WindowView::SetProtectedContentEnabled(const bool enabled, const bool excludeFromCapture) {
	m_impl->renderer->SetProtectedContent(enabled);
	m_impl->renderer->Reset(RendererFlags::NoDeviceReset);
	if (enabled && excludeFromCapture) {
		if (!SetWindowDisplayAffinity(
			m_impl->window->GetWindowHandle(),
			WDA_EXCLUDEFROMCAPTURE)) {
			SetWindowDisplayAffinity(
				m_impl->window->GetWindowHandle(),
				WDA_MONITOR
			);
		}
	}
	else {
		SetWindowDisplayAffinity(
			m_impl->window->GetWindowHandle(),
			WDA_NONE
		);
	}
}
