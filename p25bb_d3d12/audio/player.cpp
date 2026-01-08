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

namespace {
	namespace Commands {
		enum class CommandType {
			AcquireVoice,
			ReleaseVoice,
			SubmitPCM,
			SubmitFile,
			Play,
			Pause,
			Stop,
			SetVolume,
			SetOutputMatrix
		};

		struct Command {
			CommandType type;
			size_t voice;
		};

		struct AcquireVoice : public Command {};
		struct ReleaseVoice : public Command {};

		struct SubmitPCM : public Command {
			PameECS::Audio::PCMEntry pcm;
		};

		struct SubmitFile : public Command {
			PameECS::Audio::FileEntry file;
		};

		struct SubmitCallback : public Command {
			size_t(*callback)(void* userData, uint8_t* dest, size_t samples);
		};

		struct Play : public Command {};
		struct Pause : public Command {};
		struct Stop : public Command {};

		struct SetVolume : public Command {
			float volume;
		};

		struct SetOutputMatrix : public Command {
			uint32_t sourceChannel, destChannel;
			float matrix[64] = {};
		};
	}

	WAVEFORMATEXTENSIBLE FillFormat(uint16_t channels, uint16_t bits, uint32_t rate, bool isFloat) {
		WAVEFORMATEXTENSIBLE format;
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.nChannels = channels;
		format.Format.nSamplesPerSec = rate;
		format.Format.wBitsPerSample = bits;
		format.Format.nBlockAlign = (channels * bits) / 8;
		format.Format.nAvgBytesPerSec = rate * format.Format.nBlockAlign;
		format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		format.SubFormat = isFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
	}
}

using PameECS::Audio::Player;

struct Player::Impl {
	class AudioStream {
	public:
		~AudioStream() = default;
		// 引数はサンプル数、戻り値はバイト数
		// ビット深度が8のときのみ要求した数に一致するサンプルを取得できたときにsamples == 戻り値になることが期待できる
		virtual std::vector<uint8_t> GetBuffer(size_t samples) = 0;
		// バッファ取得前にフォーマットを取得し、ボイスで使われているものと一致しない場合、バッファを取得しない
		virtual bool IsSameFormat(const WAVEFORMATEXTENSIBLE& format) const = 0;
	};

	class FlacFileAudioStream : public AudioStream {
	public:
		FlacFileAudioStream(const std::string& filePath) {
			m_file_path = filePath;
		}

		~FlacFileAudioStream() {
			if (m_flac) drflac_close(m_flac);
		}

		std::vector<uint8_t> GetBuffer(size_t samples) override {
			if (!m_is_open) {
				PameECS::File::File<0, 0> file;
				auto ss = file.LoadSync(m_file_path);
				m_file_binary = std::vector<uint8_t>((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());
				m_is_open = true;

				m_flac = drflac_open_memory(m_file_binary.data(), m_file_binary.size(), nullptr);
				m_format = FillFormat(m_flac->channels, 32, m_flac->sampleRate, true);
			}
			if (!m_flac) return {};
			std::vector<float> pcmFrames(samples * m_flac->channels);
			size_t size = drflac_read_pcm_frames_f32(m_flac, samples, pcmFrames.data()) * (m_format.Format.wBitsPerSample / 8) * m_flac->channels;
			std::vector<uint8_t> ret(size);
			memcpy(ret.data(), pcmFrames.data(), size);
			return ret;
		}

		bool IsSameFormat(const WAVEFORMATEXTENSIBLE& other) const override {
			return memcmp(&m_format, &other, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
		}
	private:
		bool m_is_open = false;
		std::string m_file_path = "";
		std::vector<uint8_t> m_file_binary;
		drflac* m_flac;
		WAVEFORMATEXTENSIBLE m_format;
	};

	std::mutex commandMutex;
	std::queue<Commands::Command> commandQueue;

	std::thread worker;
	std::atomic<bool> running;

	Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice* masterVoice;

	struct VoiceSlot {
		IXAudio2SourceVoice* voice = nullptr;
		WAVEFORMATEXTENSIBLE format;
		std::queue<std::variant<std::vector<uint8_t>, AudioStream>> pending;

		bool loop = false;
		size_t loopStartSamples = 0;
		size_t audioEndSamples = 0;
		size_t samplesPlayed = 0;

		static constexpr size_t NumBuffers = 3;
		std::array<std::vector<uint8_t>, NumBuffers> playBuffers;
		size_t next = 0;
	};
};
