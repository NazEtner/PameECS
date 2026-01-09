#include "audio_service.hpp"

using PameECS::Services::AudioService;

struct AudioService::Impl {
	Impl() {
		player = std::make_unique<PameECS::Audio::Player>();
	}

	std::unique_ptr<PameECS::Audio::Player> player;
};

AudioService::AudioService() {
	m_impl = std::make_unique<Impl>();
}

AudioService::~AudioService() {

}

void AudioService::ShowDebug() {
	m_impl->player->ShowDebug();
}

PameECS::Audio::Player* AudioService::GetPlayer() {
	return m_impl->player.get();
}

extern "C" {
	PameECS::Audio::Player* ServicesAudioGetPlayer(PameECS::Services::AudioService* service) {
		return service->GetPlayer();
	}
}
