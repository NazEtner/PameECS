#include "graphics_service.hpp"

using PameECS::Services::GraphicsService;

struct GraphicsService::Impl {
	std::shared_ptr<PameECS::Graphics::Adapter> adapter;
	std::shared_ptr<PameECS::Graphics::WindowView> windowView;
};

GraphicsService::GraphicsService(
	std::shared_ptr<Graphics::Adapter> adapter,
	std::shared_ptr<Graphics::WindowView> windowView) {
	m_impl = std::make_unique<Impl>();
	m_impl->adapter = adapter;
	m_impl->windowView = windowView;
}

GraphicsService::~GraphicsService() = default;

std::shared_ptr<PameECS::Graphics::Adapter> GraphicsService::GetAdapter() {
	return m_impl->adapter;
}

std::shared_ptr<PameECS::Graphics::WindowView> GraphicsService::GetWindowView() {
	return m_impl->windowView;
}

extern "C" {
	PameECS::Graphics::Adapter* ServicesGraphicsGetAdapter(PameECS::Services::GraphicsService* service) {
		return service->GetAdapter().get();
	}
	PameECS::Graphics::WindowView* ServicesGraphicsGetWindowView(PameECS::Services::GraphicsService* service) {
		return service->GetWindowView().get();
	}
}
