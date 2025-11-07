#pragma once
#include <core/application_interface.hpp>
#include <boost/dll.hpp>
#include <spdlog/spdlog.h>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>

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
			return nullptr; // とりあえず
		}

		void Finalize() override;

		bool IsStopped() override {
			return false; // とりあえず
		}

		bool IsResetRequired() override {
			return false; // とりあえず
		}
	private:
		std::shared_ptr<spdlog::logger> m_logger;
	};

	Application application;
}

BOOST_DLL_ALIAS(
	PameECS::application,
	application
)
