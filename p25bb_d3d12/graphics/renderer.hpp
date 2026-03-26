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
#include "command_list_pool.hpp"
#include "../helpers/errors/windows.hpp"
#include "../exceptions/renderer_error.hpp"

namespace PameECS::Graphics {
	class Renderer final : public Pame::Graphics::IRenderer {
	public:
		Renderer(
			std::shared_ptr<spdlog::logger> logger,
			std::shared_ptr<PameECS::Graphics::Window> window,
			std::shared_ptr<BS::thread_pool<0U>> threadPool,
			bool useDebugLayer, bool useAdvancedDebug, bool protectedContent = false, bool vSync = true);

		virtual ~Renderer();

		template<bool IsPretreatmentTask = false>
		void EnqueueRenderTask(RendererTypes::RenderTask&& renderTask) {
			auto renderTaskQueue = &m_render_tasks;
			if constexpr (IsPretreatmentTask) {
				renderTaskQueue = &m_pretreatment_render_tasks;
			}

			renderTaskQueue->push(std::move(renderTask));
		}

		bool Render() override;
		bool Present() override;
		void Recovery() override;
		bool Reset(uint32_t flags) noexcept override {
			if (!(flags & (RendererFlags::ResetFlags::NoReset))) {
				m_reset = true;
				m_reset_flags |= flags;
			}
			return m_reset;
		}

		void EnableVerticalSync(bool enable) noexcept {
			m_vertical_sync_enabled = enable;
		}

		bool EnableVerticalSync(bool* enable) noexcept {
			if (enable) {
				EnableVerticalSync(*enable);
			}
			return m_vertical_sync_enabled;
		}

		Microsoft::WRL::ComPtr<ID3D12Device3>& GetDevice() { return m_device; }
		size_t GetNumFrameBuffers()noexcept { return m_back_buffers.size(); }
		size_t GetCurrentBufferIndex() { return m_swap_chain->GetCurrentBackBufferIndex(); }
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& GetRenderTargetHandles() { return m_rtv_handles; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetHandle() { return GetRenderTargetHandles()[GetCurrentBufferIndex()]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilHandle() { return m_dsv_handle; }
		const std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& GetBackBuffers() { return m_back_buffers; }
		Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBuffer() { return GetBackBuffers()[GetCurrentBufferIndex()]; }
		bool NeedReleaseResources() const { return m_is_recovery_pending; }
		bool IsDeviceRecoveredOnPreviousFrame() const { return m_is_device_recovered_on_previous_frame; }
		D3D12_VIEWPORT GetViewport() const { return m_viewport; }
		D3D12_RECT GetScissorRect() const { return m_scissor_rect; }
		void SetProtectedContent(bool enabled) noexcept { m_protected_content = enabled; }
	private:
		// Initialize functions
		[[nodiscard]]
		HRESULT m_createDXGIFactory() noexcept;
		[[nodiscard]]
		HRESULT m_createDebugDXGIFactory() noexcept;
		[[nodiscard]]
		HRESULT m_createD3D12Device() noexcept;
		[[nodiscard]]
		HRESULT m_createCommandQueue() noexcept;
		[[nodiscard]]
		HRESULT m_createFence() noexcept;
		[[nodiscard]]
		HRESULT m_createSwapChain() noexcept;
		[[nodiscard]]
		HRESULT m_createRTVHeap() noexcept;
		[[nodiscard]]
		HRESULT m_createDSVHeap() noexcept;
		[[nodiscard]]
		HRESULT m_enableDebugLayer() noexcept;
		[[nodiscard]]
		HRESULT m_enableGPUBasedValidation() noexcept;
		[[nodiscard]]
		HRESULT m_enableSynchronizedCommandQueueValidation() noexcept;

		void m_initDXGIFactory(bool useDebugLayer, bool useAdvancedDebugLayer, uint32_t flags = 0);

		void m_initD3D12(uint32_t flags);
		// End of initialize functions

		void m_release(uint32_t flags);

		void m_clearAndPreparationBackBuffer(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
		void m_transitionBackBufferToPresent(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
		
		void m_resetD3D12() {
			using RendererFlags::ResetFlags;

			// デバイスリセットが必要ない場合、またはフラグでスキップされている場合は従来通り
			if (m_reset_flags & ResetFlags::NoDeviceReset) {
				m_release(m_reset_flags);
				m_initDXGIFactory(m_use_debug_layer, m_use_advanced_debug, m_reset_flags);
				m_initD3D12(m_reset_flags);
				m_reset_flags = 0;
				return;
			}

			if (!m_is_recovery_pending) {
				m_logger->info("Device reset requiested.");
				m_logger->info("Releasing all D3D12 resources to clear LUID singleton.");

				m_release(m_reset_flags);

				m_is_recovery_pending = true;
				m_reset_flags_pending = m_reset_flags;

				return;
			}
			else {
				m_logger->info("Recreating DXGI Factory and D3D12 Device.");

				m_initDXGIFactory(m_use_debug_layer, m_use_advanced_debug, m_reset_flags_pending);
				m_initD3D12(m_reset_flags_pending);

				m_is_device_recovered_on_previous_frame = true;
				m_is_recovery_pending = false;
				m_reset_flags = 0;
				m_reset_flags_pending = 0;
			}
		}

		void m_handleError(HRESULT result, const std::string& message) {
			Helpers::Errors::Windows::HandleHRESULTError<Exceptions::RendererError>(result, message);
		}

		void m_waitForGPU(uint32_t frameIndex) noexcept;
		void m_waitForGPU() noexcept;

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
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsv_heap;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_depth_stencil_buffer;
		D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_handle;
		D3D12_VIEWPORT m_viewport = {};
		D3D12_RECT m_scissor_rect = {};
		// RendererTypes::RenderCommandはコマンドリストとアロケーターが入った構造体
		std::vector<std::future<RendererTypes::RenderCommand>> m_command_futures;
		std::vector<ID3D12CommandList*> m_command_lists;
		// End of D3D12 Objects

		const uint32_t m_back_buffers_count = 3;
		std::vector<uint64_t> m_fence_values;
		uint32_t m_current_frame_index = 0;

		struct CommandAllocatorEntry {
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
			uint64_t fenceValue;
		};
		std::queue<CommandAllocatorEntry> m_pending_allocators;
		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocator_to_sync;

		std::shared_ptr<CommandListPool> m_command_list_pool;

		std::queue<RendererTypes::RenderTask> m_render_tasks;
		std::queue<RendererTypes::RenderTask> m_pretreatment_render_tasks;

		std::shared_ptr<BS::thread_pool<0U>> m_thread_pool;
		std::shared_ptr<PameECS::Graphics::Window> m_window;

		uint32_t m_reset_flags = 0;
		uint32_t m_reset_flags_pending = 0;
		bool m_use_debug_layer = false;
		bool m_use_advanced_debug = false;
		bool m_is_device_recovered_on_previous_frame = false;
		bool m_is_recovery_pending = false;
		bool m_reset = false;
		bool m_protected_content = false;
		bool m_need_release_resources = false;

		HANDLE m_fence_event = nullptr;
		uint64_t m_fence_value = 0;

		bool m_vertical_sync_enabled = true;
	};
}
