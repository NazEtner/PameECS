#include "services.hpp"

using PameECS::Services::Services;

extern "C" {
	PECS_DLL_SHARED PameECS::Services::InputService* ServicesGetInputService(const PameECS::Services::Services* services) {
		return services->GetInputService();
	}
}

struct Services::Impl {
	std::unique_ptr<InputService> inputService;
};

Services::Services() {
	m_impl = std::make_unique<Impl>();
}

Services::~Services() {

}

void Services::OpenDebugWindow(std::shared_ptr<DebugTools::DebugGUIHost> debugGUI) {
	debugGUI->AddWindow(
		"Services",
		[this]() -> void {
			if (ImGui::CollapsingHeader("Input")) {
				m_impl->inputService->ShowDebug();
			}
		},
		{ 480.f, 0.f },
		{ 480.f, 560.f },
		false
	);
}

void Services::Update() {
	m_impl->inputService->Update();
}

void Services::SetInputService(std::unique_ptr<InputService>&& inputService) {
	m_impl->inputService = std::move(inputService);
}

PameECS::Services::InputService* Services::GetInputService() const {
	return m_impl->inputService.get();
}
