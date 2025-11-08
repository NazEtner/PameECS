#pragma once
#include <core/application_interface.hpp>
#include <boost/dll.hpp>
#include <spdlog/spdlog.h>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include "graphics/window.hpp"
#include "graphics/renderer.hpp"
#include "thread/thread_pool_table.hpp"

namespace PameECS {
	class Application : public Pame::Core::IApplication {
	public:
		virtual ~Application() = default;
		void Initialize() override;
		void Update() override;
		void SubmitRenderTask() override;
		std::shared_ptr<Pame::Graphics::IRenderer> GetRenderer() const override {
			return nullptr; // とりあえず
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
		void m_initializeLogger();
		void m_logInfo();
		void m_initializeThreadPoolTable();
		void m_initializeWindow();

		std::shared_ptr<spdlog::logger> m_logger;
		std::shared_ptr<Graphics::Window> m_window;
		std::shared_ptr<Thread::ThreadPoolTable<true>> m_thread_pool_table;
	};

	Application application;
}

BOOST_DLL_ALIAS(
	PameECS::application,
	application
)
