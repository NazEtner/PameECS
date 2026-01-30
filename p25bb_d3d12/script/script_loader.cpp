#include "script_loader.hpp"

using PameECS::Script::ScriptLoader;

void ScriptLoader::LoadAllScripts(std::shared_ptr<ECS::ECSHost>& ecsHost) {
	for (const auto& dllPath : m_config.loader.dllPath) {
		try {
			if (m_loaded_paths.find(dllPath) != m_loaded_paths.end()) {
				m_logger->warn("Script DLL {} is already loaded. Skipping.", dllPath);
				continue;
			}
			m_logger->info("Loading script DLL: {}", dllPath);
			auto scriptObject = std::make_unique<ScriptObject>(dllPath, ecsHost, m_logger);
			m_script_objects.push_back(std::move(scriptObject));
			m_logger->info("Successfully loaded script DLL: {}", dllPath);
			m_loaded_paths.insert(dllPath);
		}
		catch (const PameECS::Exceptions::ScriptError& e) {
			m_logger->error("Failed to load script DLL {}: {}", dllPath, e.what());
		}
		catch (const std::exception& e) {
			m_logger->error("Unexpected error while loading script DLL {}: {}", dllPath, e.what());
		}
	}
}
