#pragma once
#include <core/application_interface.hpp>
#include <boost/dll.hpp>
#include <spdlog/spdlog.h>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <nlohmann/json.hpp>
#include "graphics/window.hpp"
#include "graphics/renderer.hpp"
#include "thread/thread_pool_table.hpp"
#include "debug_tools/debug_gui_host.hpp"
#include "constants/thread_pool_table_ids.hpp"

#include "constants/string_literals.hpp"
#include "constants/file_implement_ids.hpp"
#include "file/file.hpp"
#include "configs/assets_config.hpp"
#include "configs/renderer_config.hpp"
#include "configs/window_config.hpp"
#include "configs/script_config.hpp"
#include "graphics/window_setting_adapter.hpp"
#include "ecs/ecs_host.hpp"
#include "services/services.hpp"
#include "script/script_loader.hpp"

namespace PameECS {
	class Application : public Pame::Core::IApplication {
	public:
		virtual ~Application() = default;
		void Initialize() override;
		void Update() override;
		void SubmitRenderTask() override;
		std::shared_ptr<Pame::Graphics::IRenderer> GetRenderer() const override {
			return m_renderer;
		}

		std::shared_ptr<Pame::Graphics::IWindow> GetWindow() const override {
			return m_window;
		}

		void Finalize() override;

		bool IsStopped() override {
			return false; // とりあえず
		}

		bool IsResetRequired() override {
			return false; // とりあえず
		}
	private:
		// 初期化用の関数
		// 実行順に並べる
		void m_initializeCom();
		void m_initializeProcessDpi();
		void m_initializeLogger();
		void m_logInfo();
		void m_initializeThreadPoolTable();
		void m_initializeFileSystem();
		void m_initializeWindow();
		void m_initializeRenderer();
		void m_initializeDebugTools();
		void m_initializeGraphicsAdapter();
		void m_initializeServices();
		void m_initializeECS();
		void m_initializeScript();

		void m_finalizeCom();

		std::string m_configDirectoryPath() {
			return m_config_root + "/" + m_platform + "/";
		}

		nlohmann::json m_getConfigJson(std::string filename) {
			auto file = File::File<static_cast<size_t>(Constants::FileImplementIds::Internal), static_cast<size_t>(Constants::FileGlobalStateIds::Common)>();
			auto filestream = file.LoadSync(m_configDirectoryPath() + filename);
			nlohmann::json json;
			filestream >> json;

			return json;
		}

		Configs::AssetsConfig m_loadAssetsConfig();
		Configs::RendererConfig m_loadRendererConfig();
		Configs::WindowConfig m_loadWindowConfig();
		Configs::ScriptConfig m_loadScriptConfig();

		std::shared_ptr<spdlog::logger> m_logger;
		std::shared_ptr<Graphics::Window> m_window;
		std::shared_ptr<Graphics::Renderer> m_renderer;
		std::shared_ptr
			<Thread::ThreadPoolTable<false, static_cast<size_t>(Constants::ThreadPoolTableIds::ApplicationMain)>>
			m_thread_pool_table;
		std::shared_ptr<DebugTools::DebugGUIHost> m_debug_gui_host;
		std::shared_ptr<Graphics::WindowSettingAdapter> m_window_setting_adaptor;
		std::shared_ptr<Services::Services> m_services;
		std::shared_ptr<Graphics::Adapter> m_graphics_adapter;
		std::shared_ptr<ECS::ECSHost> m_ecs_host;
		std::shared_ptr<Script::ScriptLoader> m_script_loader;

		bool m_initialized = false;
		bool m_is_com_initialized = false;

		const std::string m_config_archive_name = "_config.peac";
		const std::string m_config_root = "__pameecs_configs";
		const std::string m_platform = "windows";
		const std::string m_asset_config_filename = "assets.json";
		const std::string m_renderer_config_filename = "renderer.json";
		const std::string m_window_config_filename = "window.json";
		const std::string m_script_config_filename = "script.json";
	};

	Application application;
}

BOOST_DLL_ALIAS(
	PameECS::application,
	application
)
