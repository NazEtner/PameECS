#include "debug_gui_host.hpp"
#include <unordered_set>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>

using PameECS::DebugTools::DebugGUIHost;

#ifndef PAMEECS_NO_DEBUG_GUI
DebugGUIHost::DebugGUIHost(
	std::shared_ptr<PameECS::Graphics::Window> window,
	std::shared_ptr<PameECS::Graphics::Renderer> renderer)
	: m_window(window), m_renderer(renderer) {
	assert(window);
	assert(renderer);
	m_initialize();
}

DebugGUIHost::~DebugGUIHost() {
	m_finalize();
}

void DebugGUIHost::Update() {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	m_applyChanges();

	std::unordered_set<std::string> closedWindows;
	for (auto func : m_window_functions) {
		auto windowSize = m_window_size.find(func.first);
		if (windowSize != m_window_size.end()) {
			ImGui::SetNextWindowSize(ImVec2(windowSize->second.first, windowSize->second.second), ImGuiCond_FirstUseEver);
		}
		auto windowPosition = m_window_positions.find(func.first);
		if (windowPosition != m_window_positions.end()) {
			ImGui::SetNextWindowPos(ImVec2(windowPosition->second.first, windowPosition->second.second), ImGuiCond_FirstUseEver);
		}
		bool open = true;
		bool* pOpen = nullptr;
		auto windowCanBeClosed = m_windows_can_be_closed.find(func.first);
		if (windowCanBeClosed != m_windows_can_be_closed.end()) {
			pOpen = windowCanBeClosed->second ? &open : nullptr;
		}
		if (!ImGui::Begin(func.first.c_str(), pOpen, ImGuiWindowFlags_HorizontalScrollbar)) {
			ImGui::End();
			continue;
		}
		try {
			func.second();
		}
		catch (const std::exception& e) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Exception: %s", e.what());
		}
		catch (...) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown exception");
		}
		ImGui::End();

		if (!open) {
			closedWindows.insert(func.first);
		}
	}

	for (auto windowName : closedWindows) {
		m_window_functions.erase(windowName);
		m_window_positions.erase(windowName);
		m_window_size.erase(windowName);
		m_windows_can_be_closed.erase(windowName);
	}

	closedWindows.clear();

	ImGui::Render();
}

void DebugGUIHost::SubmitRenderTask() {
	if (m_renderer->IsDeviceRemovedOnPreviousFrame()) {
		m_finalize();
		m_initialize();
		return;
	}

	auto renderTargetHandle = m_renderer->GetCurrentRenderTargetHandle();

	Graphics::RendererTypes::RenderTask renderTask =
		[renderTargetHandle, srvHeap = this->m_srv_heap](Graphics::RendererTypes::RenderCommand command) -> Graphics::RendererTypes::RenderCommand {
		command.commandList->Reset(command.commandAllocator.Get(), nullptr);
		command.commandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, nullptr);

		ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeap.Get() };
		command.commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command.commandList.Get());

		command.commandList->Close();

		return command;
	};

	m_renderer->EnqueueRenderTask(std::move(renderTask));
}

void DebugGUIHost::AddWindow(
	std::string windowName,
	std::function<void()>&& windowFunc,
	std::pair<float, float> position,
	std::pair<float, float> size,
	bool canBeClosed) {
	m_pending_window_functions[windowName] = std::move(windowFunc);
	m_pending_window_positions[windowName] = position;
	m_pending_window_size[windowName] = size;
	m_pending_windows_can_be_closed[windowName] = canBeClosed;
}

void DebugGUIHost::m_initialize() {
	IMGUI_CHECKVERSION();
	context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 8;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_renderer->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_srv_heap.GetAddressOf()));

	ImGui_ImplWin32_Init(m_window->GetWindowHandle());
	ImGui_ImplDX12_Init(static_cast<ID3D12Device*>(m_renderer->GetDevice().Get()), static_cast<int>(m_renderer->GetNumFrameBuffers()),
		DXGI_FORMAT_R8G8B8A8_UNORM, m_srv_heap.Get(),
		m_srv_heap->GetCPUDescriptorHandleForHeapStart(),
		m_srv_heap->GetGPUDescriptorHandleForHeapStart());
}

void DebugGUIHost::m_finalize() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	if (context) {
		ImGui::DestroyContext(context);
		context = nullptr;
	}
	m_srv_heap.Reset();
}

void DebugGUIHost::m_applyChanges() {
	m_window_functions.insert(m_pending_window_functions.begin(), m_pending_window_functions.end());
	m_window_positions.insert(m_pending_window_positions.begin(), m_pending_window_positions.end());
	m_window_size.insert(m_pending_window_size.begin(), m_pending_window_size.end());
	m_windows_can_be_closed.insert(m_pending_windows_can_be_closed.begin(), m_pending_windows_can_be_closed.end());

	m_pending_window_functions.clear();
	m_pending_window_positions.clear();
	m_pending_window_size.clear();
	m_pending_windows_can_be_closed.clear();
}

#else
DebugGUIHost::DebugGUIHost(std::shared_ptr<PameECS::Graphics::Window>, std::shared_ptr<PameECS::Graphics::Renderer>) {}
DebugGUIHost::~DebugGUIHost() {}
void DebugGUIHost::Update() {}
void DebugGUIHost::SubmitRenderTask() {}
void DebugGUIHost::AddWindow(
	std::string,
	std::function<void()>&&,
	std::pair<float, float>,
	std::pair<float, float>,
	bool) {}
[[maybe_unused]] void DebugGUIHost::m_initialize() {}
[[maybe_unused]] void DebugGUIHost::m_finalize() {}
[[maybe_unused]] void DebugGUIHost::m_applyChanges() {}
#endif
