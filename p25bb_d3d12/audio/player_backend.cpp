#include "player_backend.hpp"
#include "../exceptions/audio_error.hpp"
#include "../helpers/container.hpp"

using PameECS::Audio::PlayerBackend;

PlayerBackend::PlayerBackend() : m_voice_callback_handler(this) {
	if (FAILED(XAudio2Create(m_xaudio2.GetAddressOf()))) {
		throw Exceptions::AudioError("Failed to initialize XAudio2.");
	}
	if (FAILED(m_xaudio2->CreateMasteringVoice(&m_master_voice))) {
		throw Exceptions::AudioError("Failed to create mastering voice.");
	}
}

PlayerBackend::~PlayerBackend() {
	for (auto& slot : m_voice_slots) {
		if (!slot.sourceVoice) continue;
		m_voice_pool.Release(slot.callback->GetFormat(), slot.sourceVoice);
		slot.sourceVoice = nullptr;
	}
}

void PlayerBackend::Register(size_t slotIndex, Callback* callback, void(*deleter)(Callback*)) {
	if (slotIndex >= m_voice_slots.size()) {
		Helpers::Container::ResizePow2(m_voice_slots, slotIndex + 1);
	}

	auto& slot = m_voice_slots[slotIndex];

	if (slot.sourceVoice) {
		m_voice_pool.Release(slot.callback->GetFormat(), slot.sourceVoice);
		slot.sourceVoice = nullptr;
		slot.callback.reset();
	}

	slot.callback = std::unique_ptr<Callback, void(*)(Callback*)>(callback, deleter);
}

bool PlayerBackend::Start(size_t slotIndex) {
	if (slotIndex >= m_voice_slots.size()) return false;
	auto& slot = m_voice_slots[slotIndex];
	if (!slot.callback || !slot.callback->IsReady()) return false;
	if (!slot.sourceVoice) {
		if (slot.callback) {
			slot.sourceVoice = m_voice_pool.Acquire(
				m_xaudio2.Get(),
				slot.callback->GetFormat(),
				&m_voice_callback_handler
			);
		}
	}
	if (!slot.sourceVoice) return false;

	slot.sourceVoice->Stop();

	slot.stopped = false;

	XAUDIO2_VOICE_STATE state = {};
	slot.sourceVoice->GetState(&state);
	if (state.BuffersQueued == 0) {
		for (size_t i = 0; i < slot.soundBuffer.size(); ++i) m_fillAndSubmit(slotIndex);
	}
	slot.sourceVoice->Start();

	return true;
}

void PlayerBackend::Stop(size_t slotIndex) {
	if (slotIndex >= m_voice_slots.size()) return;
	auto& slot = m_voice_slots[slotIndex];
	if (!slot.sourceVoice) return;
	slot.sourceVoice->Stop();
	slot.stopped = true;
	slot.sourceVoice->FlushSourceBuffers();
}

void PlayerBackend::m_fillAndSubmit(size_t slotIndex) {
	auto& slot = m_voice_slots[slotIndex];
	if (slot.stopped) return;
	if (!slot.sourceVoice || !slot.callback) return;
	if (slot.callback->IsExpired()) {
		m_voice_pool.Release(slot.callback->GetFormat(), slot.sourceVoice);
		slot.sourceVoice = nullptr;
		return;
	}

	size_t bufferIndex = slot.currentBuffer++;
	slot.currentBuffer %= slot.soundBuffer.size();
	size_t samples = 4096;
	slot.soundBuffer[bufferIndex].resize(samples * slot.callback->GetFormat().Format.nBlockAlign);

	size_t read = slot.callback->GetCallback()(slot.callback.get(), slot.soundBuffer[bufferIndex].data(), samples);

	if (read > 0) {
		XAUDIO2_BUFFER buffer = { 0 };
		buffer.AudioBytes = static_cast<UINT32>(read * slot.callback->GetFormat().Format.nBlockAlign);
		buffer.pAudioData = slot.soundBuffer[bufferIndex].data();
		buffer.pContext = reinterpret_cast<void*>(slotIndex + 1);

		slot.sourceVoice->SubmitSourceBuffer(&buffer);
	}
}
