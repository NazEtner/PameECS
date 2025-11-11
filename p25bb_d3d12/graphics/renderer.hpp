#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#include <wrl/client.h>
#include <queue>
#include <future>
#include <array>
#include <functional>
#include <graphics/renderer_interface.hpp>
#include <spdlog/spdlog.h>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>

#include "window.hpp"
#include "renderer_types.hpp"
#include "renderer_flags/reset_flags.hpp"

namespace PameECS::Graphics {
	class Renderer final : public Pame::Graphics::IRenderer {
	public:
		Renderer(
			std::shared_ptr<spdlog::logger> logger,
			std::shared_ptr<PameECS::Graphics::Window> window,
			std::shared_ptr<BS::thread_pool<0U>> threadPool,
			bool useDebugLayer, bool useAdvancedDebug);

		virtual ~Renderer();

		template<bool IsPretreatmentTask = false>
		void EnqueueRenderTask(RendererTypes::RenderTask&& renderTask) {
			auto renderTaskQueue = &m_render_tasks;
			if constexpr (IsPretreatmentTask) {
				renderTaskQueue = &m_pretreatment_render_tasks;
			}

			renderTaskQueue->push(std::move(renderTask));
		}

		bool Render() noexcept override;
		bool Present() noexcept override;
		void Recovery() override;
		bool Reset(uint32_t flags) noexcept override {
			if (!(flags & (RendererFlags::ResetFlags::NoReset))) {
				m_reset = true;
				m_reset_flags |= flags;
			}
			return m_reset;
		}
	private:
		// Initialize functions
		[[nodiscard]]
		HRESULT m_createDXGIFactory()noexcept;
		[[nodiscard]]
		HRESULT m_createDebugDXGIFactory()noexcept;
		[[nodiscard]]
		HRESULT m_createD3D12Device()noexcept;
		[[nodiscard]]
		HRESULT m_createCommandQueue()noexcept;
		[[nodiscard]]
		HRESULT m_createFence()noexcept;
		[[nodiscard]]
		HRESULT m_createSwapChain()noexcept;
		[[nodiscard]]
		HRESULT m_createRTVHeap()noexcept;
		[[nodiscard]]
		HRESULT m_enableDebugLayer()noexcept;
		[[nodiscard]]
		HRESULT m_enableGPUBasedValidation()noexcept;
		[[nodiscard]]
		HRESULT m_enableSynchronizedCommandQueueValidation()noexcept;

		void m_initDXGIFactory(bool useDebugLayer, bool useAdvancedDebugLayer);

		void m_initD3D12(uint32_t flags);
		// End of initialize functions

		void m_release(uint32_t flags);

		[[nodiscard]]
		HRESULT m_resetD3D12() {
			m_release(m_reset_flags);
			m_initD3D12(m_reset_flags);
			m_reset_flags = 0;
		}

		std::shared_ptr<spdlog::logger> m_logger;

		// D3D12 Objects
		Microsoft::WRL::ComPtr<ID3D12Debug1> m_debug_interface;
		Microsoft::WRL::ComPtr<IDXGIInfoQueue> m_dxgi_info_queue;
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgi_factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
		Microsoft::WRL::ComPtr<ID3D12Device3> m_device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_back_buffers;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_handles;
		// End of D3D12 Objects

		std::queue<RendererTypes::RenderTask> m_render_tasks;
		std::queue<RendererTypes::RenderTask> m_pretreatment_render_tasks;

		std::shared_ptr<BS::thread_pool<0U>> m_thread_pool;
		std::shared_ptr<PameECS::Graphics::Window> m_window;

		uint32_t m_reset_flags = 0;
		bool m_is_device_removed_on_previous_frame = false;
		bool m_reset = false;
	};
}
