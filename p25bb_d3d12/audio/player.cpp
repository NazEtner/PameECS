#include "player.hpp"
#include "../exceptions/audio_error.hpp"
#include "../helpers/container.hpp"
#include "../file/file.hpp"
#include <mutex>
#include <queue>
#include <vector>
#include <stack>
#include <format>
#include <thread>
#include <chrono>
#include <xaudio2.h>
#include <wrl/client.h>
#include <dr_libs/dr_wav.h>
#include <dr_libs/dr_flac.h>
#include <ks.h>
#include <ksmedia.h>
#include <imgui/imgui.h>

namespace {
	namespace Commands {
		enum class CommandType {
			AcquireVoice,
			ReleaseVoice,
			SubmitPCM,
			SubmitFile,
			SubmitCallback,
			Play,
			Pause,
			Stop,
			SetVolume,
			SetOutputMatrix
		};

		struct Command {
			virtual ~Command() = default;
			CommandType type;
			size_t voice;
		};

		struct AcquireVoice : public Command {};
		struct ReleaseVoice : public Command {};

		struct SubmitPCM : public Command {
			PameECS::Audio::PCMEntry pcm;
			std::vector<uint8_t> buffer;
		};

		struct SubmitFile : public Command {
			PameECS::Audio::FileEntry file;
			std::string fileName; // nullptr参照防止のためにコピーする
		};

		struct SubmitCallback : public Command {
			size_t(*callback)(void* userData, uint8_t* dest, size_t samples) = nullptr;
			WAVEFORMATEXTENSIBLE format = {};
			void* userData = nullptr;
		};

		struct Play : public Command {};
		struct Pause : public Command {};
		struct Stop : public Command {};

		struct SetVolume : public Command {
			float volume = 1.0f;
		};

		struct SetOutputMatrix : public Command {
			uint32_t sourceChannel = 0, destChannel = 0;
			float matrix[64] = {};
		};
	}

	WAVEFORMATEXTENSIBLE FillFormat(uint16_t channels, uint16_t bits, uint32_t rate, bool isFloat) {
		WAVEFORMATEXTENSIBLE format;
		ZeroMemory(&format, sizeof(format));
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.nChannels = channels;
		format.Format.nSamplesPerSec = rate;
		format.Format.wBitsPerSample = bits;
		format.Format.nBlockAlign = (channels * bits) / 8;
		format.Format.nAvgBytesPerSec = rate * format.Format.nBlockAlign;
		format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		format.Samples.wValidBitsPerSample = bits;
		format.SubFormat = isFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;

		return format;
	}
}

using PameECS::Audio::Player;

struct Player::Impl {
	// 内部型定義 --------------------------------------------------------------
	class AudioStream {
	public:
		virtual ~AudioStream() = default;
		// 戻り値のサンプル数がsamplesを満たしていない場合、このストリームは破棄対象
		virtual std::vector<uint8_t> GetBuffer(size_t samples) = 0;
		// バッファ取得前にフォーマットを取得し、ボイスで使われているものと一致しない場合、バッファを取得しない
		virtual bool IsSameFormat(const WAVEFORMATEXTENSIBLE& format) const = 0;
		virtual WAVEFORMATEXTENSIBLE GetFormat() const = 0;
		virtual bool Ready() const { return true; }
	};

	// FLACファイル用のオーディオストリーム
	class FlacFileAudioStream : public AudioStream {
	public:
		FlacFileAudioStream(const std::string& filePath) : m_file_path(filePath) {
			PameECS::File::File<0, 0> file;

			m_future_ss = file.LoadAsync(m_file_path);
		}

		virtual ~FlacFileAudioStream() {
			if (m_flac)
				drflac_close(m_flac);
		}

		bool Ready() const override {
			if (m_is_open) return true;

			return m_future_ss.valid() &&
				m_future_ss.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
		}

		std::vector<uint8_t> GetBuffer(size_t samples) override {
			// GetBufferが呼ばれる時点でReady()がtrueである前提
			if (!m_is_open) {
				if (!OpenFromFuture()) return {}; // ロード失敗時は空を返して破棄対象へ
			}

			if (!m_flac) return {};

			std::vector<float> pcmFrames(samples * m_flac->channels);
			size_t framesRead = drflac_read_pcm_frames_f32(m_flac, samples, pcmFrames.data());

			// 実際に読み込めたバイト数にリサイズ
			size_t size = framesRead * m_format.Format.nBlockAlign;
			std::vector<uint8_t> ret(size);
			memcpy(ret.data(), pcmFrames.data(), size);

			return ret;
		}

		bool IsSameFormat(const WAVEFORMATEXTENSIBLE& other) const override {
			return memcmp(&m_format, &other, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
		}

		WAVEFORMATEXTENSIBLE GetFormat() const override { return m_format; }

	private:
		bool OpenFromFuture() {
			try {
				auto ss = m_future_ss.get();
				m_file_binary = std::vector<uint8_t>((std::istreambuf_iterator<char>(ss)),
					std::istreambuf_iterator<char>());

				m_flac = drflac_open_memory(m_file_binary.data(), m_file_binary.size(), nullptr);
				if (!m_flac) return false;

				m_format = FillFormat(m_flac->channels, 32, m_flac->sampleRate, true);
				m_is_open = true;
				return true;
			}
			catch (...) {
				return false;
			}
		}

		bool m_is_open = false;
		std::string m_file_path;
		std::vector<uint8_t> m_file_binary;
		drflac* m_flac = nullptr;
		WAVEFORMATEXTENSIBLE m_format = {};
		std::future<std::stringstream> m_future_ss;
	};

	// WAVファイル用のオーディオストリーム
	class WavFileAudioStream : public AudioStream {
	public:
		WavFileAudioStream(const std::string& filePath) : m_file_path(filePath) {
			m_format = {};
			PameECS::File::File<0, 0> file;

			m_future_ss = file.LoadAsync(m_file_path);
		}

		virtual ~WavFileAudioStream() {
			if (m_is_open)
				drwav_uninit(&m_wav);
		}

		bool Ready() const override {
			if (m_is_open) return true;
			if (m_has_error) return true;

			return m_future_ss.valid() &&
				m_future_ss.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
		}

		std::vector<uint8_t> GetBuffer(size_t samples) override {
			if (m_has_error) return {};

			if (!m_is_open) {
				if (!OpenFromFuture()) {
					m_has_error = true;
					return {};
				}
			}

			std::vector<float> pcmFrames(samples * m_wav.channels);
			size_t framesRead = drwav_read_pcm_frames_f32(&m_wav, samples, pcmFrames.data());

			size_t size = framesRead * m_format.Format.nBlockAlign;
			std::vector<uint8_t> ret(size);
			memcpy(ret.data(), pcmFrames.data(), size);

			return ret;
		}

		bool IsSameFormat(const WAVEFORMATEXTENSIBLE& other) const override {
			return memcmp(&m_format, &other, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
		}

		WAVEFORMATEXTENSIBLE GetFormat() const override {
			return m_format;
		}

	private:
		bool OpenFromFuture() {
			try {
				auto ss = m_future_ss.get();
				m_file_binary = std::vector<uint8_t>((std::istreambuf_iterator<char>(ss)),
					std::istreambuf_iterator<char>());

				if (!drwav_init_memory(&m_wav, m_file_binary.data(), m_file_binary.size(), nullptr)) {
					return false;
				}

				m_format = FillFormat(m_wav.channels, 32, m_wav.sampleRate, true);
				m_is_open = true;
				return true;
			}
			catch (...) {
				return false;
			}
		}

		bool m_is_open = false;
		bool m_has_error = false;
		std::string m_file_path = "";
		std::vector<uint8_t> m_file_binary;
		drwav m_wav = {};
		WAVEFORMATEXTENSIBLE m_format;
		std::future<std::stringstream> m_future_ss;
	};

	// PCM用のオーディオストリーム
	class PCMAudioStream : public AudioStream {
	public:
		PCMAudioStream(std::vector<uint8_t> buffer, const WAVEFORMATEXTENSIBLE& format)
			: m_pcm_buffer(buffer), m_format(format) {}
		virtual ~PCMAudioStream() = default;

		std::vector<uint8_t> GetBuffer(size_t samples) override {
			auto bytesToRead = samples * m_format.Format.nBlockAlign;
			auto start = m_bytes_read;
			auto end = m_bytes_read + bytesToRead;
			end = std::min(m_pcm_buffer.size(), end);
			std::vector<uint8_t> ret(m_pcm_buffer.data() + start, m_pcm_buffer.data() + end);
			m_bytes_read += ret.size();
			return ret;
		}

		bool IsSameFormat(const WAVEFORMATEXTENSIBLE& other) const override {
			return memcmp(&m_format, &other, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
		}

		WAVEFORMATEXTENSIBLE GetFormat() const override {
			return m_format;
		}
	private:
		std::vector<uint8_t> m_pcm_buffer;
		size_t m_bytes_read = 0;
		WAVEFORMATEXTENSIBLE m_format;
	};

	// コールバック用のオーディオストリーム
	class CallbackAudioStream : public AudioStream {
	public:
		using AudioCallback = size_t(*)(void* userData, uint8_t* dest, size_t samples);
		CallbackAudioStream(AudioCallback callback, const WAVEFORMATEXTENSIBLE& format, void* userData)
			: m_callback(callback), m_format(format), m_user_data(userData) {}

		virtual ~CallbackAudioStream() = default;

		std::vector<uint8_t> GetBuffer(size_t samples) override {
			if (!m_callback) return {};
			std::vector<uint8_t> ret(samples * m_format.Format.nBlockAlign);
			auto sizeRead = m_callback(m_user_data, ret.data(), samples) * m_format.Format.nBlockAlign;
			ret.resize(sizeRead);
			return ret;
		}

		bool IsSameFormat(const WAVEFORMATEXTENSIBLE& other) const override {
			return memcmp(&m_format, &other, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
		}

		WAVEFORMATEXTENSIBLE GetFormat() const override {
			return m_format;
		}
	private:
		WAVEFORMATEXTENSIBLE m_format = {};
		AudioCallback m_callback = nullptr;
		void* m_user_data = nullptr;
	};

	struct VoiceSlot {
		~VoiceSlot() {
			Clear();
		}
		void Clear() {
			if (voice) {
				voice->Stop();
				voice->FlushSourceBuffers();
				voice->DestroyVoice();
				voice = nullptr;
			}
			matrixInfo.active = false;
			pending = {};
			loopBuffer = {};
			samplesPlayed = 0;
		}
		// ボイスが初期化済みかどうかの確認はソースボイスが有効なポインタであるかで行う
		IXAudio2SourceVoice* voice = nullptr;
		WAVEFORMATEXTENSIBLE format = {};
		std::queue<std::unique_ptr<AudioStream>> pending;
		// holdBuffer == trueのときに、デコードしたバッファをここに保存しておく
		// 次のPlay時に、PCMAudioStreamに変換してpendingの先頭に入れる
		std::vector<std::unique_ptr<PCMAudioStream>> buffersHeld;
		std::queue<std::unique_ptr<AudioStream>> loopBuffer;

		bool holdBuffer = false;
		bool lockHold = false; // 一度以上ループされていたらホールド用バッファをロックする
		bool loop = false;
		size_t loopStartSamples = 0;
		size_t samplesPlayed = 0;
		float volume = 1.0f;
		bool active = false;
		bool isPlayRequested = false;

		struct MatrixInfo {
			bool active = false;
			uint32_t sourceChannel = 0, destChannel = 0;
			float matrix[64] = {};
		} matrixInfo;

		static constexpr size_t NumBuffers = 3;
		std::array<std::vector<uint8_t>, NumBuffers> playBuffers;
		size_t next = 0;
	};
	// 内部型定義 END ----------------------------------------------------------

	static constexpr size_t audioChunkSamples = 32768; // 単位はサンプル 
	std::mutex commandMutex;
	std::queue<std::unique_ptr<Commands::Command>> commandQueue;

	std::thread worker;
	std::atomic<bool> running;

	std::vector<std::unique_ptr<VoiceSlot>> slots;
	std::queue<size_t> emptySlots;
	size_t nextSlot = 0;

	Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice* masterVoice;

	// worker用関数定義 --------------------------------------------------------
	void WorkerMain() {
		// どうせコマンドの処理があるのでコールバックを使うより普通にポーリングした方がシンプル
		while (running) {
			std::queue<std::unique_ptr<Commands::Command>> commandSnapshot = {};
			{
				std::lock_guard<std::mutex> lock(commandMutex);
				std::swap(commandQueue, commandSnapshot);
			}
			while (!commandSnapshot.empty()) {
				ProcessCommand(commandSnapshot.front().get());
				commandSnapshot.pop();
			}
			for (auto& slot : slots) {
				ProcessSlot(slot.get());
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void ProcessSlot(VoiceSlot* slot) {
		if (!slot || !slot->active) return;
		XAUDIO2_VOICE_STATE state = {};
		if (slot->voice) {
			slot->voice->GetState(&state);
		}
		if (slot->isPlayRequested && slot->voice) {
			slot->voice->Stop();
			slot->voice->FlushSourceBuffers();
		}
		if (state.BuffersQueued < VoiceSlot::NumBuffers && !slot->pending.empty()) {
			auto& stream = slot->pending.front();
			if (!stream->Ready()) return;
			auto buffer = stream->GetBuffer(audioChunkSamples);

			if (!slot->voice) {
				slot->format = stream->GetFormat();
				if (FAILED(xaudio2->CreateSourceVoice(&slot->voice, &slot->format.Format))) {
					slot->pending.pop();
					slot->voice = nullptr;
					return;
				}
			}

			if (!stream->IsSameFormat(slot->format)) {
				slot->pending.pop();
				return;
			}

			auto format = slot->format;
			auto bufferSamples = buffer.size() / format.Format.nBlockAlign;

			if (audioChunkSamples > bufferSamples) {
				slot->pending.pop();
				if (slot->pending.empty() && slot->loop) {
					slot->lockHold = true;
					std::swap(slot->pending, slot->loopBuffer);
					slot->samplesPlayed = slot->loopStartSamples;
				}
				if (bufferSamples == 0) return;
			}

			if (slot->holdBuffer && !slot->lockHold) {
				slot->buffersHeld.push_back(
					std::make_unique<PCMAudioStream>(buffer, format)
				);
			}

			if (slot->loop && slot->loopStartSamples < slot->samplesPlayed + bufferSamples) {
				size_t eraseBytes = 0;
				if (slot->samplesPlayed <= slot->loopStartSamples) {
					eraseBytes = (slot->loopStartSamples - slot->samplesPlayed) * format.Format.nBlockAlign;
				}
				slot->loopBuffer.push(
					std::make_unique<PCMAudioStream>(std::vector<uint8_t>(buffer.data() + eraseBytes, buffer.data() + buffer.size()), format)
				);
			}

			// slot->playBuffers[slot->next] = buffer;
			std::swap(slot->playBuffers[slot->next], buffer);

			XAUDIO2_BUFFER sourceBuffer = { 0 };
			sourceBuffer.AudioBytes = static_cast<UINT32>(slot->playBuffers[slot->next].size());
			sourceBuffer.pAudioData = slot->playBuffers[slot->next].data();
			sourceBuffer.Flags = 0;

			slot->voice->SubmitSourceBuffer(&sourceBuffer);

			slot->samplesPlayed += bufferSamples;

			slot->next++;
			slot->next %= VoiceSlot::NumBuffers;
		}
		if (slot->isPlayRequested && slot->voice) {
			slot->voice->Start();
			slot->isPlayRequested = false;
		}
		if (slot->voice) {
			slot->voice->SetVolume(slot->volume);
			if (slot->matrixInfo.active) {
				slot->voice->SetOutputMatrix(
					nullptr,
					slot->matrixInfo.sourceChannel, slot->matrixInfo.destChannel,
					slot->matrixInfo.matrix
				);
				slot->matrixInfo.active = false;
			}
		}
	}

	std::unique_ptr<VoiceSlot>& GetSlotRef(size_t handle) {
		if (slots.size() <= handle) {
			Helpers::Container::ResizePow2(slots, handle + 1);
		}
		return slots[handle]; // nullptrであっても関係なく取得
	}

	void SetSlotProperty(VoiceSlot* slot, bool loop, size_t loopStart, bool holdBuffer) {
		if (!slot->voice && slot->pending.empty()) {
			slot->loop = loop;
			slot->loopStartSamples = loopStart;
			slot->holdBuffer = holdBuffer;
		}
	}

	void ProcessCommand(Commands::Command* command) {
		if (!command) return;

		std::lock_guard<std::mutex> lock(commandMutex);

		auto& slot = GetSlotRef(command->voice);

		switch (command->type) {
		case Commands::CommandType::AcquireVoice:
			if (slot) {
				slot->Clear();
			}
			slot = std::make_unique<VoiceSlot>(); // ソースボイスはまだ作らない
			break;
		case Commands::CommandType::ReleaseVoice:
			if (slot) {
				slot->Clear();
			}
			slot = nullptr;
			break;
		case Commands::CommandType::SubmitPCM: {
			if (!slot) break;
			auto pcmCommand = static_cast<Commands::SubmitPCM*>(command);
			SetSlotProperty(slot.get(), pcmCommand->pcm.loopEnable, pcmCommand->pcm.loopStart, pcmCommand->pcm.holdBuffer);
			slot->pending.push(
				std::make_unique<PCMAudioStream>(pcmCommand->buffer, pcmCommand->pcm.waveFormat)
			);
			slot->buffersHeld.clear();
			slot->lockHold = false;
			break;
		}
		case Commands::CommandType::SubmitFile: {
			if (!slot) break;
			auto fileCommand = static_cast<Commands::SubmitFile*>(command);
			SetSlotProperty(slot.get(), fileCommand->file.loopEnable, fileCommand->file.loopStart, fileCommand->file.holdBuffer);

			switch (fileCommand->file.codec) {
			case FileEntry::Codec::WAV:
				slot->pending.push(
					std::make_unique<WavFileAudioStream>(fileCommand->fileName)
				);
				break;
			case FileEntry::Codec::FLAC:
				slot->pending.push(
					std::make_unique<FlacFileAudioStream>(fileCommand->fileName)
				);
				break;
			default:
				break;
			}
			slot->buffersHeld.clear();
			slot->lockHold = false;
			break;
		}
		case Commands::CommandType::SubmitCallback: {
			if (!slot) break;
			auto callbackCommand = static_cast<Commands::SubmitCallback*>(command);
			SetSlotProperty(slot.get(), false, 0, false);

			slot->pending.push(
				std::make_unique<CallbackAudioStream>(
					callbackCommand->callback,
					callbackCommand->format,
					callbackCommand->userData
				)
			);

			slot->buffersHeld.clear();
			break;
		}
		case Commands::CommandType::Play:
			if (!slot || slot->active) break;
			slot->active = true;
			slot->isPlayRequested = true;
			if (!slot->buffersHeld.empty() && slot->pending.empty()) {
				for (const auto& buffer : slot->buffersHeld) {
					slot->pending.push(std::make_unique<PCMAudioStream>(*buffer));
				}

				slot->lockHold = true;
				slot->samplesPlayed = 0;
			}
			break;
		case Commands::CommandType::Pause:
			if (!slot || !slot->voice) break;
			slot->active = false;
			slot->voice->Stop();
			break;
		case Commands::CommandType::Stop:
			if (!slot) break;
			slot->active = false;
			slot->Clear();
			break;
		case Commands::CommandType::SetVolume: {
			auto volumeCommand = static_cast<Commands::SetVolume*>(command);
			slot->volume = volumeCommand->volume;
			break;
		}
		case Commands::CommandType::SetOutputMatrix: {
			auto matrixCommand = static_cast<Commands::SetOutputMatrix*>(command);
			slot->matrixInfo.active = true;
			memcpy(slot->matrixInfo.matrix, matrixCommand->matrix, sizeof(slot->matrixInfo.matrix));
			slot->matrixInfo.destChannel = matrixCommand->destChannel;
			slot->matrixInfo.sourceChannel = matrixCommand->sourceChannel;
			break;
		}
		default:
			break;
		}
	}
	// worker用関数定義 END ----------------------------------------------------

	Impl() {
		if (FAILED(XAudio2Create(xaudio2.GetAddressOf()))) {
			throw Exceptions::AudioError("Failed to initialize XAudio2.");
		}
		if (FAILED(xaudio2->CreateMasteringVoice(&masterVoice))) {
			throw Exceptions::AudioError("Failed to create mastering voice.");
		}
		running = true;
		worker = std::thread([this]() -> void {WorkerMain(); });
	}

	~Impl() {
		running = false;
		if (worker.joinable()) worker.join();
		slots.clear();
		if (masterVoice) {
			masterVoice->DestroyVoice();
			masterVoice = nullptr;
		}
	}
};

Player::Player() : m_impl(std::make_unique<Impl>()) {}

Player::~Player() {

}

size_t Player::GetVoiceHandle() {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	size_t handle;
	if (!m_impl->emptySlots.empty()) {
		// 再利用可能なスロットがある場合
		handle = m_impl->emptySlots.front();
		m_impl->emptySlots.pop();
	}
	else {
		// 新規スロットを発行
		handle = m_impl->nextSlot++;
	}

	auto cmd = std::make_unique<Commands::AcquireVoice>();
	cmd->type = Commands::CommandType::AcquireVoice;
	cmd->voice = handle;

	m_impl->commandQueue.push(std::move(cmd));
	return handle;
}

void Player::ReleaseVoiceHandle(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	// コマンドを発行
	auto cmd = std::make_unique<Commands::ReleaseVoice>();
	cmd->type = Commands::CommandType::ReleaseVoice;
	cmd->voice = voiceHandle;
	m_impl->commandQueue.push(std::move(cmd));

	// 再利用リストに追加
	m_impl->emptySlots.push(voiceHandle);
}

void Player::Submit(size_t voiceHandle, const PCMEntry* entry) {
	if (!entry) return;
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	auto cmd = std::make_unique<Commands::SubmitPCM>();
	cmd->type = Commands::CommandType::SubmitPCM;
	cmd->voice = voiceHandle;
	cmd->pcm = *entry;
	// 生データからバッファへコピー
	cmd->buffer = std::vector<uint8_t>(
		entry->data, entry->data + entry->dataSize
	);

	m_impl->commandQueue.push(std::move(cmd));
}

void Player::Submit(size_t voiceHandle, const FileEntry* entry) {
	if (!entry) return;
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	auto cmd = std::make_unique<Commands::SubmitFile>();
	cmd->type = Commands::CommandType::SubmitFile;
	cmd->voice = voiceHandle;
	cmd->file = *entry;
	cmd->fileName = std::string(entry->fileName, entry->nameSize);

	m_impl->commandQueue.push(std::move(cmd));
}

void Player::Submit(size_t voiceHandle, size_t(*callback)(void*, uint8_t*, size_t), const WAVEFORMATEXTENSIBLE& format, void* userData) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	auto cmd = std::make_unique<Commands::SubmitCallback>();
	cmd->type = Commands::CommandType::SubmitCallback;
	cmd->voice = voiceHandle;
	cmd->callback = callback;
	cmd->format = format;
	cmd->userData = userData;

	m_impl->commandQueue.push(std::move(cmd));
}

void Player::Play(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);
	auto cmd = std::make_unique<Commands::Play>();
	cmd->type = Commands::CommandType::Play;
	cmd->voice = voiceHandle;
	m_impl->commandQueue.push(std::move(cmd));
}

void Player::Pause(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);
	auto cmd = std::make_unique<Commands::Pause>();
	cmd->type = Commands::CommandType::Pause;
	cmd->voice = voiceHandle;
	m_impl->commandQueue.push(std::move(cmd));
}

void Player::Stop(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);
	auto cmd = std::make_unique<Commands::Stop>();
	cmd->type = Commands::CommandType::Stop;
	cmd->voice = voiceHandle;
	m_impl->commandQueue.push(std::move(cmd));
}

void Player::SetVolume(size_t voiceHandle, float volume) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);
	auto cmd = std::make_unique<Commands::SetVolume>();
	cmd->type = Commands::CommandType::SetVolume;
	cmd->voice = voiceHandle;
	cmd->volume = volume;
	m_impl->commandQueue.push(std::move(cmd));
}

size_t Player::GetQueuedVoiceCount(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);
	if (voiceHandle >= m_impl->slots.size() || !m_impl->slots[voiceHandle]) return 0;
	return m_impl->slots[voiceHandle]->pending.size();
}

uint32_t Player::GetOutputChannels() {
	if (!m_impl->masterVoice) return 0;
	XAUDIO2_VOICE_DETAILS details;
	m_impl->masterVoice->GetVoiceDetails(&details);
	return details.InputChannels;
}

uint32_t Player::GetVoiceChannels(size_t voiceHandle) {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);
	if (voiceHandle >= m_impl->slots.size() || !m_impl->slots[voiceHandle]) return 0;
	return m_impl->slots[voiceHandle]->format.Format.nChannels;
}

void Player::SetOutputMatrix(size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix) {
	if (!matrix) return;
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	auto cmd = std::make_unique<Commands::SetOutputMatrix>();
	cmd->type = Commands::CommandType::SetOutputMatrix;
	cmd->voice = voiceHandle;
	cmd->sourceChannel = sourceChannels;
	cmd->destChannel = destChannels;

	size_t count = static_cast<size_t>(sourceChannels) * destChannels;
	if (count > 64) count = 64;
	memcpy(cmd->matrix, matrix, count * sizeof(float));

	m_impl->commandQueue.push(std::move(cmd));
}

void Player::ShowDebug() {
	std::lock_guard<std::mutex> lock(m_impl->commandMutex);

	ImGui::Text("Audio Player Debugger");

	if (ImGui::Button("Acquire New Voice Handle")) {
		size_t index;
		if (!m_impl->emptySlots.empty()) {
			index = m_impl->emptySlots.front();
			m_impl->emptySlots.pop();
		}
		else {
			index = m_impl->nextSlot++;
		}

		auto cmd = std::make_unique<Commands::AcquireVoice>();
		cmd->type = Commands::CommandType::AcquireVoice;
		cmd->voice = index;
		m_impl->commandQueue.push(std::move(cmd));
	}

	ImGui::Separator();

	// --- ボイス一覧 ---
	if (ImGui::BeginTable("SlotsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
		ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f);
		ImGui::TableSetupColumn("Status");
		ImGui::TableSetupColumn("Queue");
		ImGui::TableSetupColumn("Flags"); // Loop/Hold状態
		ImGui::TableSetupColumn("Volume");
		ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableHeadersRow();

		for (size_t i = 0; i < m_impl->slots.size(); ++i) {
			auto& slot = m_impl->slots[i];
			if (!slot) continue;

			ImGui::TableNextRow();
			ImGui::PushID(static_cast<int>(i));

			// ID & Status & Queue
			ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", i);
			ImGui::TableSetColumnIndex(1); ImGui::Text(slot->active ? "PLAY" : "STOP");
			ImGui::TableSetColumnIndex(2); ImGui::Text("Q:%zu H:%zu", slot->pending.size(), slot->buffersHeld.size());

			// Flags (Loop / Hold Buffer)
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%s %s", slot->loop ? "[L]" : "[-]", slot->holdBuffer ? "[H]" : "[-]");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("L: Loop enabled\nH: Buffer holding enabled");
			}

			// Volume
			ImGui::TableSetColumnIndex(4);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##v", &slot->volume, 0.0f, 1.0f)) {
				auto cmd = std::make_unique<Commands::SetVolume>();
				cmd->type = Commands::CommandType::SetVolume;
				cmd->voice = i;
				cmd->volume = slot->volume;
				m_impl->commandQueue.push(std::move(cmd));
			}

			// Actions (Play/Stop/Release)
			ImGui::TableSetColumnIndex(5);
			if (ImGui::Button("Play")) {
				auto cmd = std::make_unique<Commands::Play>();
				cmd->type = Commands::CommandType::Play;
				cmd->voice = i;
				m_impl->commandQueue.push(std::move(cmd));
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				auto cmd = std::make_unique<Commands::Stop>();
				cmd->type = Commands::CommandType::Stop;
				cmd->voice = i;
				m_impl->commandQueue.push(std::move(cmd));
			}
			ImGui::SameLine();
			if (ImGui::Button("Rel")) {
				auto cmd = std::make_unique<Commands::ReleaseVoice>();
				cmd->type = Commands::CommandType::ReleaseVoice;
				cmd->voice = i;
				m_impl->commandQueue.push(std::move(cmd));
				m_impl->emptySlots.push(i);
			}

			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	// --- Submit詳細設定 ---
	ImGui::Separator();
	ImGui::Text("Submit Configuration");

	static char path[128] = "test.wav";
	static int targetId = 0;
	static bool loopEnable = false;
	static int loopStart = 0;
	static bool holdBuffer = false;

	ImGui::InputInt("Target Voice ID", &targetId);
	ImGui::InputText("File Path", path, 128);
	ImGui::Checkbox("Loop Enable", &loopEnable);
	ImGui::SameLine();
	ImGui::Checkbox("Hold Buffer", &holdBuffer);
	if (loopEnable) {
		ImGui::InputInt("Loop Start (samples)", &loopStart);
	}

	if (ImGui::Button("Submit WAV")) {
		auto cmd = std::make_unique<Commands::SubmitFile>();
		cmd->type = Commands::CommandType::SubmitFile;
		cmd->voice = static_cast<size_t>(targetId);
		cmd->file.codec = FileEntry::Codec::WAV;
		cmd->file.loopEnable = loopEnable;
		cmd->file.loopStart = static_cast<size_t>(loopStart);
		cmd->file.holdBuffer = holdBuffer;
		cmd->fileName = path;
		m_impl->commandQueue.push(std::move(cmd));
	}
	ImGui::SameLine();
	if (ImGui::Button("Submit FLAC")) {
		auto cmd = std::make_unique<Commands::SubmitFile>();
		cmd->type = Commands::CommandType::SubmitFile;
		cmd->voice = static_cast<size_t>(targetId);
		cmd->file.codec = FileEntry::Codec::FLAC;
		cmd->file.loopEnable = loopEnable;
		cmd->file.loopStart = static_cast<size_t>(loopStart);
		cmd->file.holdBuffer = holdBuffer;
		cmd->fileName = path;
		m_impl->commandQueue.push(std::move(cmd));
	}

	// --- コールバック再生テスト ---
	if (ImGui::Button("Submit Sine Wave (Callback)")) {
		auto cmd = std::make_unique<Commands::SubmitCallback>();
		cmd->type = Commands::CommandType::SubmitCallback;
		cmd->voice = static_cast<size_t>(targetId);

		// 44.1kHz Stereo Float
		cmd->format = FillFormat(2, 32, 44100, true);

		// デバッグ用の簡易正弦波生成コールバック
		cmd->callback = [](void* userData, uint8_t* dest, size_t samples) -> size_t {
			static float phase = 0.0f;
			float* fDest = reinterpret_cast<float*>(dest);
			for (size_t i = 0; i < samples; ++i) {
				float val = sinf(phase) * 0.2f; // 音量控えめ
				fDest[i * 2 + 0] = val; // L
				fDest[i * 2 + 1] = val; // R
				phase += 2.0f * 3.14159f * 440.0f / 44100.0f;
				if (phase > 2.0f * 3.14159f) phase -= 2.0f * 3.14159f;
			}
			return samples;
			};
		cmd->userData = nullptr;
		m_impl->commandQueue.push(std::move(cmd));
	}
	if (ImGui::Button("Submit FM Synthesis (Callback)")) {
		auto cmd = std::make_unique<Commands::SubmitCallback>();
		cmd->type = Commands::CommandType::SubmitCallback;
		cmd->voice = static_cast<size_t>(targetId);

		cmd->format = FillFormat(2, 32, 44100, true);

		cmd->callback = [](void* userData, uint8_t* dest, size_t samples) -> size_t {
			static float time = 0.0f;
			float* fDest = reinterpret_cast<float*>(dest);
			const float sampleRate = 44100.0f;

			// FMパラメータ
			const float carrierFreq = 440.0f;    // 基軸となる音
			const float modRatio = 3.5f;       // モジュレータの比率（非整数だと金属音っぽくなる）

			for (size_t i = 0; i < samples; ++i) {
				// 時間経過で変調の深さ（Feedback/Index）を変化させて「ミョン」という音にする
				float modIndex = (sinf(time * 0.5f) + 1.0f) * 5.0f;

				// モジュレータ (Operator 2)
				float modulator = sinf(time * 2.0f * 3.14159f * carrierFreq * modRatio);

				// キャリア (Operator 1) - モジュレータによって周波数が揺らされる
				float carrier = sinf(time * 2.0f * 3.14159f * carrierFreq + (modulator * modIndex));

				float output = carrier * 0.2f;

				fDest[i * 2 + 0] = output; // L
				fDest[i * 2 + 1] = output; // R

				time += 1.0f / sampleRate;
			}
			return samples;
			};

		m_impl->commandQueue.push(std::move(cmd));
	}
	ImGui::Text("Slot debug");
	static uint32_t targetHandle = 0;
	ImGui::InputInt("Target Voice ID##1", reinterpret_cast<int*>(&targetHandle));
	if (targetHandle >= m_impl->slots.size() || !m_impl->slots[targetHandle]) {
		ImGui::Text("nullptr");
	}
	else {
		auto& slot = m_impl->slots[targetHandle];
		ImGui::Text("Queued: %d", slot->pending.size());
		ImGui::Text("Held  : %d", slot->buffersHeld.size());
		ImGui::Text("Loop  : %d", slot->loopBuffer.size());
		ImGui::Text("Next  : %d", slot->next);
		ImGui::Text("Play buffers: {\n\t%p(%dBytes), \n\t%p(%dBytes), \n\t%p(%dBytes)}",
			slot->playBuffers[0].data(), slot->playBuffers[0].size(),
			slot->playBuffers[1].data(), slot->playBuffers[1].size(),
			slot->playBuffers[2].data(), slot->playBuffers[2].size());
	}
}

extern "C" {
	PECS_DLL_SHARED size_t AudioPlayerGetVoiceHandle(PameECS::Audio::Player* player) {
		return player->GetVoiceHandle();
	}
	PECS_DLL_SHARED void AudioPlayerReleaseVoiceHandle(PameECS::Audio::Player* player, size_t voice) {
		player->ReleaseVoiceHandle(voice);
	}
	PECS_DLL_SHARED void AudioPlayerSubmitPCM(PameECS::Audio::Player* player, size_t voice, const PameECS::Audio::PCMEntry* entry) {
		player->Submit(voice, entry);
	}
	PECS_DLL_SHARED void AudioPlayerSubmitFile(PameECS::Audio::Player* player, size_t voice, const PameECS::Audio::FileEntry* entry) {
		player->Submit(voice, entry);
	}
	PECS_DLL_SHARED void AudioPlayerSubmitCallback(PameECS::Audio::Player* player, size_t voice, size_t(*callback)(void* userData, uint8_t* dest, size_t samples), const WAVEFORMATEXTENSIBLE& format, void* userData) {
		player->Submit(voice, callback, format, userData);
	}
	PECS_DLL_SHARED void AudioPlayerPlay(PameECS::Audio::Player* player, size_t voiceHandle) {
		player->Play(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerPause(PameECS::Audio::Player* player, size_t voiceHandle) {
		player->Pause(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerStop(PameECS::Audio::Player* player, size_t voiceHandle) {
		player->Stop(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerSetVolume(PameECS::Audio::Player* player, size_t voiceHandle, float volume) {
		player->SetVolume(voiceHandle, volume);
	}
	PECS_DLL_SHARED size_t AudioPlayerGetQueuedVoiceCount(PameECS::Audio::Player* player, size_t voiceHandle) {
		return player->GetQueuedVoiceCount(voiceHandle);
	}
	PECS_DLL_SHARED uint32_t AudioPlayerGetOutputChannels(PameECS::Audio::Player* player) {
		return player->GetOutputChannels();
	}
	PECS_DLL_SHARED uint32_t AudioPlayerGetVoiceChannels(PameECS::Audio::Player* player, size_t voiceHandle) {
		return player->GetVoiceChannels(voiceHandle);
	}
	PECS_DLL_SHARED void AudioPlayerSetOutputMatrix(PameECS::Audio::Player* player, size_t voiceHandle, uint32_t sourceChannels, uint32_t destChannels, float* matrix) {
		player->SetOutputMatrix(voiceHandle, sourceChannels, destChannels, matrix);
	}
}
