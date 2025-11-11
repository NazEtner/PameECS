#include "renderer.hpp"
#include "../macros/debug.hpp"
#include "../exceptions/invalid_argument.hpp"

using PameECS::Graphics::Renderer;

Renderer::Renderer(
	std::shared_ptr<spdlog::logger> logger,
	std::shared_ptr<PameECS::Graphics::Window> window,
	std::shared_ptr<BS::thread_pool<0U>> threadPool,
	bool useDebugLayer, bool useAdvancedDebug) :
	IRenderer(), m_logger(std::move(logger)), m_window(std::move(window)), m_thread_pool(std::move(threadPool)) {
	if (!m_logger) throw PameECS::Exceptions::InvalidArgument("Logger is null.");
	if (!m_window) throw PameECS::Exceptions::InvalidArgument("Window is null.");
	if (!m_thread_pool) throw PameECS::Exceptions::InvalidArgument("Thread pool is null.");

	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_initDXGIFactory(useDebugLayer, useAdvancedDebug);
	m_initD3D12(0);
}

Renderer::~Renderer() {
	// リソースはComPtrかshared_ptrなので明示的には解放しない
	m_waitForGPU();

	CloseHandle(m_fence_event);
}

bool Renderer::Render() noexcept {
	try {
		auto clearCommandAllocator = m_command_list_pool->GetCommandAllocator();
		clearCommandAllocator->Reset();
		auto clearCommandList = m_command_list_pool->GetCommandList(clearCommandAllocator.Get());
		clearCommandList->Reset(clearCommandAllocator.Get(), nullptr);

		m_clearAndPreparationBackBuffer(clearCommandList);

		auto distributer = [this](std::queue<RendererTypes::RenderTask>& tasks) -> void {
			while (!tasks.empty()) {
				auto renderTask = std::move(tasks.front());
				tasks.pop();
				auto commandAllocator = m_command_list_pool->GetCommandAllocator();
				commandAllocator->Reset();
				auto commandList = m_command_list_pool->GetCommandList(commandAllocator.Get());

				RendererTypes::RenderCommand renderCommand;
				renderCommand.commandList = std::move(commandList);
				renderCommand.commandAllocator = std::move(commandAllocator);

				auto future = m_thread_pool->submit_task(
					[task = std::move(renderTask), command = std::move(renderCommand)] {
						return task(command);
					}
				);

				m_command_futures.emplace_back(std::move(future));
			}
		};

		distributer(m_pretreatment_render_tasks);
		distributer(m_render_tasks);

		auto transitionCommandAllocator = m_command_list_pool->GetCommandAllocator();
		transitionCommandAllocator->Reset();
		auto transitionCommandList = m_command_list_pool->GetCommandList(transitionCommandAllocator.Get());
		transitionCommandList->Reset(transitionCommandAllocator.Get(), nullptr);
		m_transitionBackBufferToPresent(transitionCommandList);

		auto emplaceAndReturnCommand = [this](Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& allocator, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& commandList) -> void {
			m_command_lists.emplace_back(commandList.Get());
			m_command_list_pool->ReturnCommandAllocator(allocator);
			m_command_list_pool->ReturnCommandList(commandList);
		};

		emplaceAndReturnCommand(clearCommandAllocator, clearCommandList);

		// 画面クリア用とトランジション用が+2の部分
		m_command_lists.reserve(m_command_futures.size() + 2);
		for (auto& future : m_command_futures) {
			auto command = future.get();
			emplaceAndReturnCommand(command.commandAllocator, command.commandList);
		}

		emplaceAndReturnCommand(transitionCommandAllocator, transitionCommandList);
	}
	catch (const std::exception& e) {
		m_logger->error("Failed to execute rendering tasks.\n" + std::string(e.what()));
		m_pretreatment_render_tasks = {};
		m_render_tasks = {};
		return false;
	}
	catch (...) {
		m_logger->error("Failed to execute rendering tasks.\nUnknown error");
		m_pretreatment_render_tasks = {};
		m_render_tasks = {};
		return false;
	}
	m_command_futures.clear();
	return true;
}

bool Renderer::Present() {
	assert(m_command_queue);
	assert(m_swap_chain);

	if (m_command_lists.size() > std::numeric_limits<UINT>::max()) {
		return false;
	}

	m_command_queue->ExecuteCommandLists(static_cast<UINT>(m_command_lists.size()), m_command_lists.data());

	m_waitForGPU();

	m_command_lists.clear();

	if (FAILED(m_swap_chain->Present(static_cast<uint32_t>(m_vertical_sync_enabled), 0))) {
		return false;
	}

	m_is_device_removed_on_previous_frame = false;

	return true;
}

void Renderer::Recovery() {
	HRESULT hr = m_device->GetDeviceRemovedReason();
	try {
		if (m_command_lists.size() > std::numeric_limits<UINT>::max()) {
			throw Exceptions::RendererError("Command lists overflow.");
		}

		switch (hr) {
		case DXGI_ERROR_DEVICE_REMOVED:
		case DXGI_ERROR_DEVICE_RESET:
			m_reset_flags = 0u;
			m_resetD3D12();
			break;
		case S_OK:
			if (m_reset) {
				m_reset = false;
				m_resetD3D12();
			}
			break;
		case DXGI_ERROR_DEVICE_HUNG:
			throw Exceptions::RendererError(std::format("{:08X}", static_cast<unsigned long>(hr)) + "(DXGI_ERROR_DEVICE_HUNG)");
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			throw Exceptions::RendererError(std::format("{:08X}", static_cast<unsigned long>(hr)) + "(DXGI_ERROR_DRIVER_INTERNAL_ERROR)");
		case DXGI_ERROR_INVALID_CALL:
			throw Exceptions::RendererError(std::format("{:08X}", static_cast<unsigned long>(hr)) + "(DXGI_ERROR_INVALID_CALL)");
		default:
			throw Exceptions::RendererError(std::format("{:08X}", static_cast<unsigned long>(hr)));
		}
	}
	catch (const Pame::Exceptions::ExceptionBase& e) {
		throw Exceptions::RendererError("Failed to recovery renderer.\n" + std::string(e.what()), e.GetTrace());
	}
	catch (const std::exception& e) {
		throw Exceptions::RendererError("Failed to recovery renderer.\n" + std::string(e.what()));
	}
	catch (...) {
		throw Exceptions::RendererError("Failed to recovery renderer.\nUnknown error.");
	}
}

void Renderer::m_clearAndPreparationBackBuffer(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList) {
	UINT backBufferIndex = m_swap_chain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_back_buffers[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &barrier);

	static constexpr std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };

	commandList->ClearRenderTargetView(m_rtv_handles[backBufferIndex], color.data(), 0, nullptr);

	commandList->OMSetRenderTargets(1, &m_rtv_handles[backBufferIndex], FALSE, nullptr);

	commandList->Close();
}

void Renderer::m_transitionBackBufferToPresent(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList) {
	UINT backBufferIndex = m_swap_chain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_back_buffers[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
	commandList->Close();
}

void Renderer::m_initDXGIFactory(bool useDebugLayer, bool useAdvancedDebugLayer) {
	bool debugDisallow = false;
#ifndef _DEBUG
	debugDisallow = true;
	m_logger->warn("Cannot to enable debugging renderer in release build");
#endif
	if (useDebugLayer && !debugDisallow) {
		m_handleError(m_enableDebugLayer(), "Failed to enable debug layer.");
		if (useAdvancedDebugLayer) {
			m_handleError(m_enableSynchronizedCommandQueueValidation(), "Failed to enable synchronized command queue validation.");
			m_handleError(m_enableGPUBasedValidation(), "Failed to enable GPU based validation");
		}

		m_handleError(m_createDebugDXGIFactory(), "Failed to create debugging DXGI factory");
	}
	else {
		m_handleError(m_createDXGIFactory(), "Failed to create DXGI factory");
	}
}

void Renderer::m_initD3D12(uint32_t flags) {
	using RendererFlags::ResetFlags; // リセット(実際の呼び出しはRecovery())で使いまわすために、ここでフラグを読み取る

	if (flags & ResetFlags::NoReset) return; // そもそもこのフラグが入ったままここに来るのは意味不明だけど念のため

	if (!(flags & ResetFlags::NoDeviceReset)) {
		m_handleError(m_createD3D12Device(), "Failed to create Direct3D12 device.");
		m_handleError(m_createFence(), "Failed to create fence.");
		m_handleError(m_createCommandQueue(), "Failed to create command queue.");

		m_command_list_pool = std::make_shared<CommandListPool>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	m_handleError(m_createSwapChain(), "Failed to create swap chain.");
	m_handleError(m_createRTVHeap(), "Failed to create RTV heap and RTV handles");
}

void Renderer::m_release(uint32_t flags) {
	m_waitForGPU();

	using RendererFlags::ResetFlags;

	if (flags & ResetFlags::NoReset) return;

	m_pretreatment_render_tasks = {};
	m_render_tasks = {};
	if (!(flags & ResetFlags::NoDeviceReset)) {
		m_device = nullptr;
		m_adapter = nullptr;
		m_fence = nullptr;
		m_command_queue = nullptr;

		m_command_list_pool = nullptr;
	}

	m_swap_chain = nullptr;
	m_back_buffers.clear();
	m_rtv_handles.clear();
	m_rtv_heap = nullptr;
}

void Renderer::m_waitForGPU() noexcept {
	if (!m_command_queue) return;
	if (!m_fence) return;
	if (!m_fence_event) return;

	const uint64_t currentFenceValue = m_fence_value++;
	m_handleError(m_command_queue->Signal(m_fence.Get(), currentFenceValue), "Cannot to wait for GPU.");

	if (m_fence->GetCompletedValue() < currentFenceValue) {
		m_handleError(m_fence->SetEventOnCompletion(currentFenceValue, m_fence_event), "Failed to set waiting event.");
		WaitForSingleObject(m_fence_event, INFINITE);
	}
}

HRESULT Renderer::m_createDXGIFactory() noexcept {
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory));
	return hr;
}

HRESULT Renderer::m_createDebugDXGIFactory() noexcept {
	if (!m_debug_interface) {
		return m_createDXGIFactory();
	}
	UINT dxgiFactoryFlags = 0;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_info_queue)))) {
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	else {
		return m_createDXGIFactory();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	if (FAILED(hr)) {
		return hr;
	}

	hr = factory.As(&m_dxgi_factory);
	if (FAILED(hr)) {
		return hr;
	}

	return hr;
}

HRESULT Renderer::m_createD3D12Device() noexcept {
	for (UINT i = 0; m_dxgi_factory->EnumAdapters1(i, &m_adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC1 adapterDesc;
		m_handleError(m_adapter->GetDesc1(&adapterDesc), "Failed to get display adapter description.");

		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		return D3D12CreateDevice(
			m_adapter.Get(),
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(&m_device)
		);
	}

	return E_FAIL;
}

HRESULT Renderer::m_createCommandQueue() noexcept {
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	return m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_command_queue));
}

HRESULT Renderer::m_createFence() noexcept {
	HRESULT hr = m_device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_fence)
	);

	return hr;
}

HRESULT Renderer::m_createSwapChain() noexcept {
	try {
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		auto windowProperties = m_window->GetProperties(Window::NoClassName | Window::NoWindowName | Window::NoWindowStyle);
		assert(windowProperties.width.has_value() && windowProperties.height.has_value());
		swapChainDesc.BufferCount = 2;
		swapChainDesc.Width = windowProperties.width.value();
		swapChainDesc.Height = windowProperties.height.value();
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT hr = m_dxgi_factory->CreateSwapChainForHwnd(
			m_command_queue.Get(),
			m_window->GetWindowHandle(),
			&swapChainDesc,
			nullptr,
			nullptr,
			reinterpret_cast<IDXGISwapChain1**>(m_swap_chain.GetAddressOf())
		);

		if (FAILED(hr)) {
			return hr;
		}

		hr = m_dxgi_factory->MakeWindowAssociation(m_window->GetWindowHandle(), DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

		m_viewport = { 0.0f, 0.0f, static_cast<float>(windowProperties.width.value()), static_cast<float>(windowProperties.height.value()), 0.0f, 1.0f };
		m_scissor_rect = { 0, 0, static_cast<long>(windowProperties.width.value()), static_cast<long>(windowProperties.height.value()) };

		return hr;
	}
	catch (...) {
		return E_FAIL;
	}
}

HRESULT Renderer::m_createRTVHeap() noexcept {
	m_back_buffers.clear();
	m_rtv_handles.clear();
	using Microsoft::WRL::ComPtr;

	const UINT backBufferCount = 2;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = backBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtv_heap));
	if (FAILED(hr)) {
		return hr;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleStart = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (UINT i = 0; i < backBufferCount; i++) {
		ComPtr<ID3D12Resource> backBuffer;
		hr = m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		if (FAILED(hr)) {
			return hr;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandleStart;
		rtvHandle.ptr += i * rtvDescriptorSize;

		m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_back_buffers.push_back(std::move(backBuffer));

		m_rtv_handles.emplace_back(rtvHandle);
	}

	return S_OK;
}

HRESULT Renderer::m_enableDebugLayer() noexcept {
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_interface));
	if (SUCCEEDED(hr)) {
		m_debug_interface->EnableDebugLayer();
		m_debug_interface->SetEnableSynchronizedCommandQueueValidation(FALSE);
	}

	return hr;
}

HRESULT Renderer::m_enableGPUBasedValidation() noexcept {
	if (!m_debug_interface) return E_POINTER;
	m_debug_interface->SetEnableGPUBasedValidation(TRUE);

	return S_OK;
}

HRESULT Renderer::m_enableSynchronizedCommandQueueValidation() noexcept {
	if (!m_debug_interface) return E_POINTER;
	m_debug_interface->SetEnableSynchronizedCommandQueueValidation(TRUE);

	return S_OK;
}
