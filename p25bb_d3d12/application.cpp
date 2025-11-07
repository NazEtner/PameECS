#include "application.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "helpers/id_generator.hpp"
#include "template_types/string_literal.hpp"

using PameECS::Application;

void Application::Initialize() {
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

#ifdef _DEBUG
	m_logger->info("Debug build");
#else
	m_logger->info("Release build");
#endif

	m_logger->info("PameECS D3D12 - P25BB_D3D12 (c)2025 Nananami(NazEt)");

	PameECS::Helpers::IdGenerator idGen;

	m_logger->debug("apple : ID {}", idGen.GetId<"apple">());
	m_logger->debug("banana : ID {}", idGen.GetId<"banana">());
	m_logger->debug("apple : ID {}", idGen.GetId<"apple">());
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
