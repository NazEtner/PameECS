#include "core_loop.hpp"
#include "../exceptions/dll_load_failed.hpp"
#include "../exceptions/invalid_state.hpp"
#include "../exceptions/exception_base.hpp"
#include "../exceptions/config_load_failed.hpp"
#include <boost/dll.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <format>
#include <nlohmann/json.hpp>

using Pame::Core::CoreLoop;

CoreLoop::CoreLoop() {
	try {
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("core.log", true);

		std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
		m_logger = std::make_shared<spdlog::logger>("PameECSExecutable", sinks.begin(), sinks.end());

#ifdef _DEBUG
		consoleSink->set_level(spdlog::level::trace);
		fileSink->set_level(spdlog::level::trace);
		m_logger->set_level(spdlog::level::trace);
#else
		consoleSink->set_level(spdlog::level::info);
		fileSink->set_level(spdlog::level::info);
		m_logger->set_level(spdlog::level::info);
#endif
		spdlog::flush_every(std::chrono::seconds(3));
		m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^level %L%$] [thread %t] %v");
		m_logger->info("PameECS (c)2025 Nananami(NazEt)");
#ifdef _DEBUG
		m_logger->info("Debug build");
#else
		m_logger->info("Release build");
#endif
		m_logger->info("PameECS {:d}.{:d}.{:d} initializing...", m_major_version, m_minor_version, m_patch_version);

		m_loadConfig();
	}
	catch (const Exceptions::ExceptionBase& e) {
		auto message = std::format("{0} : {1}\n{2}", e.GetExceptionTypeName(), e.what(), e.GetTrace());
		m_logger->critical(message);
		MessageBox(NULL, message.c_str(), "PameECS initialize error", MB_ICONERROR | MB_OK);
		if (m_application) {
			m_application->Finalize();
		}
		throw;
	}
}

CoreLoop::~CoreLoop() {
	m_window = nullptr;
	m_renderer = nullptr;
	if (m_application) {
		m_application->Finalize();
	}
}

void CoreLoop::Execute() {
	try {
		if (!m_application)
			throw Exceptions::InvalidState("Application is not loaded.");
		if (!m_window)
			throw Exceptions::InvalidState("Window is not initialized.");
		if (!m_renderer)
			throw Exceptions::InvalidState("Renderer is not initialized.");
		while (m_window->Update() && !m_application->IsStopped()) {
			m_application->Update();
			m_application->SubmitRenderTask();
			if (!m_renderer->Render() || !m_renderer->Present()) {
				m_logger->error("Rendering error or Presentation error occurred!");
				m_logger->info("Trying to recovery renderer...");
				m_renderer->Recovery();
			}
		}
	}
	catch (const Exceptions::ExceptionBase& e) {
		auto message = std::format("{0} : {1}\n{2}", e.GetExceptionTypeName(), e.what(), e.GetTrace());
		m_logger->critical(message);
		MessageBox(NULL, message.c_str(), "PameECS runtime error", MB_ICONERROR | MB_OK);
		throw;
	}
}

bool CoreLoop::IsResetRequired() const {
	if (m_application) {
		return m_application->IsResetRequired();
	}
	return false;
}

void CoreLoop::m_loadApplication(std::string applicationPath) {
	try {
		m_logger->info("Loading {}...", applicationPath);
		m_application_library = boost::dll::shared_library(applicationPath.c_str());

		m_application = boost::dll::import_alias<Pame::Core::IApplication>(
			m_application_library,
			"application"
		);

		m_logger->info("Application Dll {} loaded.", applicationPath);

		m_application->Initialize();
		m_renderer = m_application->GetRenderer();
		m_window = m_application->GetWindow();
	}
	catch (const Pame::Exceptions::ExceptionBase& e) {
		throw Pame::Exceptions::DllLoadFailed(std::string(e.GetExceptionTypeName()) + " : " + e.what(), e.GetTrace());
	}
	catch (const std::exception& e) {
		throw Pame::Exceptions::DllLoadFailed(e.what());
	}
}

void CoreLoop::m_loadConfig() {
	std::ifstream configFile(m_config_file_name);
	nlohmann::json configJson;
	if (configFile.is_open()) {
		configFile >> configJson;
		configFile.close();
	}
	else {
		throw Pame::Exceptions::ConfigLoadFailed(("Failed to open " + m_config_file_name).c_str());
	}

	if (configJson.contains("system")) {
		auto systemConfig = configJson["system"];
		if (!systemConfig.contains("applicationDll")) throw Pame::Exceptions::ConfigLoadFailed("\"applicationDll\" is required.");
		m_loadApplication(systemConfig["applicationDll"]);
	}
	else {
		throw Pame::Exceptions::ConfigLoadFailed("There is no \"system\" config.");
	}
}
