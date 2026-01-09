#include "services.hpp"

using PameECS::Services::Services;

extern "C" {
	PECS_DLL_SHARED PameECS::Services::InputService* ServicesGetInputService(const PameECS::Services::Services* services) {
		return services->GetInputService();
	}

	PECS_DLL_SHARED PameECS::Services::AudioService* ServicesGetAudioService(const PameECS::Services::Services* services) {
		return services->GetAudioService();
	}
}

struct Services::Impl {
	std::unique_ptr<InputService> inputService;
	std::unique_ptr<AudioService> audioService;
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
			if (m_impl->inputService && ImGui::CollapsingHeader("Input")) {
				m_impl->inputService->ShowDebug();
			}
			if (m_impl->audioService && ImGui::CollapsingHeader("Audio")) {
				m_impl->audioService->ShowDebug();
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

void Services::SetAudioService(std::unique_ptr<AudioService>&& audioService) {
	m_impl->audioService = std::move(audioService);
}

PameECS::Services::InputService* Services::GetInputService() const {
	return m_impl->inputService.get();
}

PameECS::Services::AudioService* Services::GetAudioService() const {
	return m_impl->audioService.get();
}
