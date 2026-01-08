#pragma once
#include "input_service.hpp"
#include "audio_service.hpp"
#include "../debug_tools/debug_gui_host.hpp"

namespace PameECS::Services {
	// サービスを一つのクラスから取得するためのインターフェイス
	class Services {
	public:
		Services();
		~Services();
		Services(const Services&) = delete;
		Services& operator=(const Services&) = delete;
		Services(Services&&) = delete;
		Services& operator=(Services&&) = delete;

		void OpenDebugWindow(std::shared_ptr<DebugTools::DebugGUIHost> debugGUI);

		void Update();
		void SetInputService(std::unique_ptr<InputService>&& inputService);
		void SetAudioService(std::unique_ptr<AudioService>&& audioService);
		InputService* GetInputService() const;
		AudioService* GetAudioService() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PECS_DLL_SHARED PameECS::Services::InputService* ServicesGetInputService(const PameECS::Services::Services* services);
}
