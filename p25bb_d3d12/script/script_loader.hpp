#pragma once
#include "script_object.hpp"
#include "../configs/script_config.hpp"
#include <memory>
#include <vector>
#include <unordered_set>

namespace PameECS::Script {
	class ScriptLoader {
	public:
		ScriptLoader(
			const Configs::ScriptConfig& config,
			std::shared_ptr<spdlog::logger>& logger) {
			m_logger = logger;
			m_config = config;
		}
		~ScriptLoader() = default;
		void LoadAllScripts(std::shared_ptr<ECS::ECSHost>& ecsHost);
	private:
		Configs::ScriptConfig m_config;
		std::unordered_set<std::string> m_loaded_paths;
		std::vector<std::unique_ptr<ScriptObject>> m_script_objects;
		std::shared_ptr<spdlog::logger> m_logger;
	};
}
