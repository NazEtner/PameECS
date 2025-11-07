#pragma once
#include "../graphics/renderer_interface.hpp"
#include "../graphics/window_interface.hpp"
#include <memory>

namespace Pame::Core {
	class IApplication {
	public:
		virtual ~IApplication() = default;
		virtual void Initialize() = 0;
		virtual void Update() = 0;
		virtual void SubmitRenderTask() = 0;
		virtual std::shared_ptr<Graphics::IRenderer> GetRenderer() const = 0;
		virtual std::shared_ptr<Graphics::IWindow> GetWindow() const = 0;
		virtual void Finalize() = 0;
		virtual bool IsStopped() { return false; }
		virtual bool IsResetRequired() { return false; }
	};
}
