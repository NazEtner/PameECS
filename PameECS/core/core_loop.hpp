#pragma once
#include "../graphics/renderer_interface.hpp"
#include "../graphics/window_interface.hpp"
#include "application_interface.hpp"
#include <memory>
#include <boost/shared_ptr.hpp>
#include <spdlog/spdlog.h>
#include <boost/dll.hpp>

namespace Pame::Core {
	class CoreLoop {
	public:
		CoreLoop();
		virtual ~CoreLoop();
		void Execute();
		bool IsResetRequired() const;
	private:
		void m_loadApplication(std::string applicationPath);
		void m_loadConfig();

		const std::string m_config_file_name = "engine_config.json";
		const unsigned int m_major_version = 1;
		const unsigned int m_minor_version = 0;
		const unsigned int m_patch_version = 0;

		boost::dll::shared_library m_application_library;
		boost::shared_ptr<IApplication> m_application;
		std::shared_ptr<Graphics::IRenderer> m_renderer;
		std::shared_ptr<Graphics::IWindow> m_window;

		std::shared_ptr<spdlog::logger> m_logger;
	};
}
