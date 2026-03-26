#pragma once
#include <boost/dll.hpp>
#include <spdlog/logger.h>
#include "../abi/check.hpp"
#include "../exceptions/script_error.hpp"
#include "../ecs/ecs_host.hpp"

namespace PameECS::Script {
	class ScriptObject {
	public:
		ScriptObject(const boost::dll::fs::path& libraryPath, std::shared_ptr<ECS::ECSHost>& ecsHost, std::shared_ptr<spdlog::logger>& logger) {
			try {
				m_logger = logger;
				m_library = boost::dll::shared_library(libraryPath);
				m_checkABI();
				m_init(ecsHost);
			}
			catch (const PameECS::Exceptions::ScriptError) {
				throw;
			}
			catch (const std::exception& e) {
				throw PameECS::Exceptions::ScriptError(std::string("Failed to load script object: ") + e.what());
			}
			catch (...) {
				throw PameECS::Exceptions::ScriptError(std::string("Failed to load script object: ") + "Unknown error.");
			}
		}
		~ScriptObject() = default;

		ScriptObject(const ScriptObject&) = delete;
		ScriptObject& operator=(const ScriptObject&) = delete;
	private:
		void m_checkABI();
		void m_init(std::shared_ptr<ECS::ECSHost>& ecsHost);
		boost::dll::shared_library m_library;
		std::shared_ptr<spdlog::logger> m_logger;
	};
}
