#include "application.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "helpers/id_generator.hpp"
#include "template_types/string_literal.hpp"
#include "constants/string_literals.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto* window = reinterpret_cast<PameECS::Graphics::Window*>(
		GetWindowLongPtr(hWnd, GWLP_USERDATA));

	auto renderer = PameECS::application.GetRenderer();

	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		if (window) {
			window->Destroy();
		}
		else {
			DestroyWindow(hWnd);
		}
		return 0;
	case WM_SIZE:
		if (renderer) {
			renderer->Reset(PameECS::Graphics::RendererFlags::NoDeviceReset);
			return 0;
		}
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

using PameECS::Application;

void Application::Initialize() {
	m_initializeLogger();
	m_logInfo();
	m_initializeThreadPoolTable();
	m_initializeWindow();
	m_initializeRenderer();
}

void Application::Update() {
	// とりあえず何もしない
}

void Application::SubmitRenderTask() {
	// とりあえず何もしない
}

void Application::Finalize() {
	m_logger->info("Finalizing application...");
}

void Application::m_initializeLogger() {
	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("application.log", true);
	std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
	m_logger = std::make_shared<spdlog::logger>("PameECS", sinks.begin(), sinks.end());
#ifdef _DEBUG
	consoleSink->set_level(spdlog::level::trace);
	fileSink->set_level(spdlog::level::trace);
	m_logger->set_level(spdlog::level::trace);
#else
	consoleSink->set_level(spdlog::level::info);
	fileSink->set_level(spdlog::level::info);
	m_logger->set_level(spdlog::level::info);
#endif

	m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^level %L%$] [thread %t] %v");
}

void Application::m_logInfo() {
	m_logger->info("PameECS D3D12 - P25BB_D3D12 (c)2025 Nananami(NazEt)");

#ifdef _DEBUG
	m_logger->info("Debug build");
#else
	m_logger->info("Release build");
#endif
}

void Application::m_initializeThreadPoolTable() {
	m_thread_pool_table = std::make_shared<Thread::ThreadPoolTable<true>>();
}

void Application::m_initializeWindow() {
	Graphics::Window::Properties properties;
	properties.windowProcedure = WndProc;

	m_window = std::make_shared<Graphics::Window>(properties, m_logger);
	m_window->Show();
}

void Application::m_initializeRenderer() {
	m_thread_pool_table->Allocate<Constants::StringLiterals::RendererThreadPoolName>();

	m_renderer = std::make_shared<Graphics::Renderer>(
		m_logger,
		m_window,
		m_thread_pool_table->GetThreadPool<Constants::StringLiterals::RendererThreadPoolName>(),
		true, false
	);
}
