#pragma once
#include <imgui/imgui.h>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>

#include "../graphics/renderer.hpp"
#include "../graphics/window.hpp"

namespace PameECS::DebugTools {
	// スレッドセーフではない
	class DebugGUIHost {
	public:
		DebugGUIHost(std::shared_ptr<PameECS::Graphics::Window> window, std::shared_ptr<PameECS::Graphics::Renderer> renderer);
		~DebugGUIHost();

		void Update();
		void SubmitRenderTask();
		void AddWindow(
			std::string windowName,
			std::function<void()>&& windowFunc,
			std::pair<float, float> position,
			std::pair<float, float> size,
			bool canBeClosed = true);
	private:
		void m_initialize();
		void m_finalize();
		void m_applyChanges();

		std::unordered_map<std::string, std::function<void()>> m_window_functions;
		std::unordered_map<std::string, std::pair<float, float>> m_window_positions;
		std::unordered_map<std::string, std::pair<float, float>> m_window_size;
		std::unordered_map<std::string, bool> m_windows_can_be_closed;

		std::unordered_map<std::string, std::function<void()>> m_pending_window_functions;
		std::unordered_map<std::string, std::pair<float, float>> m_pending_window_positions;
		std::unordered_map<std::string, std::pair<float, float>> m_pending_window_size;
		std::unordered_map<std::string, bool> m_pending_windows_can_be_closed;

		std::shared_ptr<PameECS::Graphics::Window> m_window;
		std::shared_ptr<PameECS::Graphics::Renderer> m_renderer;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srv_heap;

		ImGuiContext* context = nullptr;
	};
}
