#pragma once
#include "../audio/player.hpp"

namespace PameECS::Services {
	class AudioService {
	public:
		AudioService();
		~AudioService();

		void ShowDebug();

		Audio::Player* GetPlayer();
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PameECS::Audio::Player* ServicesAudioGetPlayer(PameECS::Services::AudioService* service);
}
