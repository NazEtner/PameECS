#pragma once
#include "../graphics/adapter.hpp"
#include "../graphics/window_view.hpp"

namespace PameECS::Services {
	class GraphicsService {
	public:
		GraphicsService(std::shared_ptr<Graphics::Adapter> adapter, std::shared_ptr<Graphics::WindowView> windowView);
		~GraphicsService();
		
		std::shared_ptr<Graphics::Adapter> GetAdapter();
		std::shared_ptr<Graphics::WindowView> GetWindowView();
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PECS_DLL_SHARED PameECS::Graphics::Adapter* ServicesGraphicsGetAdapter(PameECS::Services::GraphicsService* service);
	PECS_DLL_SHARED PameECS::Graphics::WindowView* ServicesGraphicsGetWindowView(PameECS::Services::GraphicsService* service);
}
