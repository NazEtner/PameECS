#include "script_object.hpp"
#include <format>
#include <cassert>

using PameECS::Script::ScriptObject;

void ScriptObject::m_checkABI() {
	assert(m_library);
	m_logger->info("Checking ABI compatibility for script object...");
	auto getPluginCompilerInfo = boost::dll::import_symbol<void(CompilerInfo*)>(m_library, "GetCompilerInfo");
	CompilerInfo pluginCompilerInfo = {};
	getPluginCompilerInfo(&pluginCompilerInfo);
	CompilerInfo hostCompilerInfo = {};
	GetCompilerInfo(&hostCompilerInfo);
	if (hostCompilerInfo.size == pluginCompilerInfo.size) {
		const uint8_t* hostBytes = reinterpret_cast<const uint8_t*>(&hostCompilerInfo);
		const uint8_t* pluginBytes = reinterpret_cast<const uint8_t*>(&pluginCompilerInfo);
		std::string hostHex;
		std::string pluginHex;
		for (size_t i = 0; i < sizeof(CompilerInfo); ++i) {
			hostHex += std::format("{:02X}", hostBytes[i]);
			pluginHex += std::format("{:02X}", pluginBytes[i]);
		}

		m_logger->info("Host: {}", hostHex);
		m_logger->info("Script: {}", pluginHex);
	}
	if (!IsSameCompilerABI(&hostCompilerInfo, &pluginCompilerInfo)) {
		throw PameECS::Exceptions::ScriptError("Incompatible compiler or ABI between host and script object.");
	}
}

void ScriptObject::m_init(std::shared_ptr<ECS::ECSHost>& ecsHost) {
	assert(m_library);
	m_logger->info("Initializing script object...");
	auto initFunc = boost::dll::import_symbol<void(ECS::ECSHost*)>(m_library, "Init");
	initFunc(ecsHost.get());
	m_logger->info("Script object initialized.");
}
