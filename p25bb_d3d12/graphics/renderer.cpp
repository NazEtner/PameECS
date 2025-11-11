#include "renderer.hpp"
#include "../exceptions/invalid_argument.hpp"

using PameECS::Graphics::Renderer;

Renderer::Renderer(
	std::shared_ptr<spdlog::logger> logger,
	std::shared_ptr<PameECS::Graphics::Window> window,
	std::shared_ptr<BS::thread_pool<0U>> threadPool,
	bool useDebugLayer, bool useAdvancedDebug) :
	IRenderer(), m_logger(logger), m_window(window), m_thread_pool(threadPool) {
	if (!m_logger) throw PameECS::Exceptions::InvalidArgument("Logger is null.");
	if (!m_window) throw PameECS::Exceptions::InvalidArgument("Window is null.");
	if (!m_thread_pool) throw PameECS::Exceptions::InvalidArgument("Thread pool is null.");

	m_initDXGIFactory(useDebugLayer, useAdvancedDebug);
	m_initD3D12(0);
}

Renderer::~Renderer() {

}

bool Renderer::Render() noexcept {
	return true;
}

bool Renderer::Present() noexcept {
	return true;
}

void Renderer::Recovery() {

}

void Renderer::m_initDXGIFactory(bool useDebugLayer, bool useAdvancedDebugLayer) {

}

void Renderer::m_initD3D12(uint32_t flags) {

}

void Renderer::m_release(uint32_t flags) {

}
