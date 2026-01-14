#pragma once
#include "window.hpp"
#include "window_setting_adapter.hpp"
#include "renderer.hpp"
#include "../macros/dll.hpp"
#include <cstdint>
#include <string>
#include <memory>

namespace PameECS::Graphics {
	class WindowView;
}

extern "C" {
	PECS_DLL_SHARED void GraphicsWindowViewSetVSyncEnabled(PameECS::Graphics::WindowView* windowView, bool isVSyncEnabled);
	PECS_DLL_SHARED bool GraphicsWindowViewIsVSyncEnabled(const PameECS::Graphics::WindowView* windowView);
	PECS_DLL_SHARED void GraphicsWindowViewSetStyle(PameECS::Graphics::WindowView* windowView, const char* name, size_t nameSize);
	PECS_DLL_SHARED void GraphicsWindowViewSetWidth(PameECS::Graphics::WindowView* windowView, uint32_t width);
	PECS_DLL_SHARED uint32_t GraphicsWindowViewGetWidth(const PameECS::Graphics::WindowView* windowView);
	PECS_DLL_SHARED void GraphicsWindowViewSetHeight(PameECS::Graphics::WindowView* windowView, uint32_t height);
	PECS_DLL_SHARED uint32_t GraphicsWindowViewGetHeight(const PameECS::Graphics::WindowView* windowView);
}
 
namespace PameECS::Graphics {
	class WindowView {
	public:
		WindowView(std::shared_ptr<Window>& window, std::shared_ptr<WindowSettingAdapter> adapter, std::shared_ptr<Renderer> renderer);
		~WindowView();
		void SetVSyncEnabled(const bool isVSyncEnabled);
		bool IsVSyncEnabled() const;
		void SetStyle(const std::string& name) {
			SetStyle(name.c_str(), name.size());
		}
		void SetStyle(const char* name, size_t nameSize);
		void SetWidth(const uint32_t width);
		uint32_t GetWidth() const;
		void SetHeight(const uint32_t height);
		uint32_t GetHeight() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
