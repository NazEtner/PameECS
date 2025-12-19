#include "application.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <imgui/imgui_impl_win32.h>

#include "macros/debug.hpp"
#include "helpers/id_generator.hpp"
#include "template_types/string_literal.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return 1L;
	}

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
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN) {
			if (window) {
				window->OnAltEnterPressed();
				return 0;
			}
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
	m_initializeFileSystem();
	m_initializeWindow();
	m_initializeRenderer();
	m_initializeDebugTools();
	m_initializeECS();

	m_initialized = true;
}

void Application::Update() {
	m_ecs_host->Update();
	m_debug_gui_host->Update();
}

void Application::SubmitRenderTask() {
	// ECSのレンダリングタスクはデバッグGUIより前
	// m_ecs_host->SubmitRenderTask();
	m_debug_gui_host->SubmitRenderTask();
}

void Application::Finalize() {
	if (!m_initialized) return;
	m_logger->info("Finalizing application...");

	File::FileGlobalState<static_cast<size_t>(Constants::FileGlobalStateIds::Common)>().Reset();
	m_thread_pool_table.reset();
	m_renderer.reset();
	m_window.reset();
	m_window_setting_adaptor.reset();
	m_debug_gui_host.reset();
	m_ecs_host.reset();

	m_logger->info("Application finalized.");
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
	m_logger->warn("DON'T CLOSE THE CONSOLE WINDOW, AS IT MAY CAUSE A RESOURCE LEAK!");

#else
	m_logger->info("Release build");
#endif
}

void Application::m_initializeThreadPoolTable() {
	// スレッドセーフではない
	m_thread_pool_table = std::make_shared<Thread::ThreadPoolTable<false, static_cast<size_t>(Constants::ThreadPoolTableIds::ApplicationMain)>>();
}

void Application::m_initializeFileSystem() {
	// ファイルシステムの初期化
	m_thread_pool_table->Allocate<Constants::StringLiterals::FileIOThreadPoolName>();
	File::FileGlobalState<static_cast<size_t>(Constants::FileGlobalStateIds::Common)>()
		.GetThreadPool() = m_thread_pool_table->GetThreadPool<Constants::StringLiterals::FileIOThreadPoolName>();

	// アーカイブファイルの初期化
	m_thread_pool_table->Allocate<Constants::StringLiterals::ArchiveDecompressThreadPoolName>();
	m_thread_pool_table->Allocate<Constants::StringLiterals::ArchiveMergeThreadPoolName>();
	// * PameECSのコンフィグ用のアーカイブファイル
	// (static_cast<size_t>(Constants::FileGlobalStateIds::Common) == 0(デフォルト値))なので、
	// static_cast<size_t>(Constants::FileGlobalStateIds::Common)は省略してもいい
	File::File<static_cast<size_t>(Constants::FileImplementIds::Internal), static_cast<size_t>(Constants::FileGlobalStateIds::Common)>()
		.SetArchiveLoader(
			std::make_shared<File::Archive::ArchiveLoader>(
				m_config_archive_name,
				m_thread_pool_table->GetThreadPool<Constants::StringLiterals::ArchiveDecompressThreadPoolName>(),
				m_logger,
				m_thread_pool_table->GetThreadPool<Constants::StringLiterals::ArchiveMergeThreadPoolName>()
			)
		);

	// * PameECSのアセット用のアーカイブファイル
	auto assetsConfig = m_loadAssetsConfig();
	for (const auto& archive : assetsConfig.archives) {
		File::File<static_cast<size_t>(Constants::FileImplementIds::Assets), static_cast<size_t>(Constants::FileGlobalStateIds::Common)>()
			.SetArchiveLoader(
				std::make_shared<File::Archive::ArchiveLoader>(
					archive,
					m_thread_pool_table->GetThreadPool<Constants::StringLiterals::ArchiveDecompressThreadPoolName>(),
					m_logger,
					m_thread_pool_table->GetThreadPool<Constants::StringLiterals::ArchiveMergeThreadPoolName>()
				)
			);
	}
}

void Application::m_initializeWindow() {
	m_window_setting_adaptor = std::make_shared<Graphics::WindowSettingAdaptor>();
	auto properties = m_window_setting_adaptor->CreateWindowProperty(m_loadWindowConfig());
	properties.windowProcedure = WndProc;

	m_window = std::make_shared<Graphics::Window>(properties, m_logger);
	m_window->Show();

	m_window_setting_adaptor->SetWindow(m_window);
}

void Application::m_initializeRenderer() {
	m_thread_pool_table->Allocate<Constants::StringLiterals::RendererThreadPoolName>();

	auto rendererConfig = m_loadRendererConfig();

	m_renderer = std::make_shared<Graphics::Renderer>(
		m_logger,
		m_window,
		m_thread_pool_table->GetThreadPool<Constants::StringLiterals::RendererThreadPoolName>(),
		rendererConfig.useDebugLayer, rendererConfig.useAdvancedDebug,
		rendererConfig.protectedContent, rendererConfig.vSync
	);
}

void Application::m_initializeDebugTools() {
	m_debug_gui_host = std::make_shared<DebugTools::DebugGUIHost>(m_window, m_renderer);
}

void Application::m_initializeECS() {
	m_thread_pool_table->Allocate<Constants::StringLiterals::ESCScheduleThreadPoolName>();

	m_ecs_host = std::make_shared<ECS::ECSHost>(
		m_thread_pool_table->GetThreadPool<Constants::StringLiterals::ESCScheduleThreadPoolName>()
	);
	if (m_debug_gui_host) {
		m_ecs_host->OpenDebugWindow(m_debug_gui_host);
	}
}

PameECS::Configs::AssetsConfig Application::m_loadAssetsConfig() {
	auto json = m_getConfigJson(m_asset_config_filename);

	Configs::AssetsConfig ret;
	ret.Deserialize(json);
	return ret;
}

PameECS::Configs::RendererConfig Application::m_loadRendererConfig() {
	auto json = m_getConfigJson(m_renderer_config_filename);

	Configs::RendererConfig ret;
	ret.Deserialize(json);
	return ret;
}

PameECS::Configs::WindowConfig Application::m_loadWindowConfig() {
	auto json = m_getConfigJson(m_window_config_filename);

	Configs::WindowConfig ret;
	ret.Deserialize(json);
	return ret;
}
