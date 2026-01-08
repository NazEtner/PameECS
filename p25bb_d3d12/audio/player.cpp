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
			WAVEFORMATEXTENSIBLE format;
			void* userData;
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
		virtual ~AudioStream() = default;
		// 戻り値のサンプル数がsamplesを満たしていない場合、このストリームは破棄対象
		virtual std::vector<uint8_t> GetBuffer(size_t samples) = 0;
		// バッファ取得前にフォーマットを取得し、ボイスで使われているものと一致しない場合、バッファを取得しない
		virtual bool IsSameFormat(const WAVEFORMATEXTENSIBLE& format) const = 0;
		virtual WAVEFORMATEXTENSIBLE GetFormat() const = 0;
	};

	// FLACファイル用のオーディオストリーム
	class FlacFileAudioStream : public AudioStream {
	public:
		FlacFileAudioStream(const std::string& filePath) {
			m_flac = nullptr;
			m_format = {};
			m_file_path = filePath;
		}

		virtual ~FlacFileAudioStream() {
			if (m_flac) drflac_close(m_flac);
		}

		std::vector<uint8_t> GetBuffer(size_t samples) override {
			if (!m_is_open) {
				// アーカイブファイルの仕様上、ファイルロード自体をストリーミングにすることはできないので、
				// こういう実装。小さいFLACファイルを複数個用意して複数個のストリーム自体キューに入れる想定
				PameECS::File::File<0, 0> file;
				auto ss = file.LoadSync(m_file_path);
				m_file_binary = std::vector<uint8_t>((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());
				m_is_open = true;

				m_flac = drflac_open_memory(m_file_binary.data(), m_file_binary.size(), nullptr);
				if(m_flac) m_format = FillFormat(m_flac->channels, 32, m_flac->sampleRate, true);
			}
			if (!m_flac) return {};
			std::vector<float> pcmFrames(samples * m_flac->channels);
			size_t size = drflac_read_pcm_frames_f32(m_flac, samples, pcmFrames.data()) * m_format.Format.nBlockAlign;
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
		bool m_is_open = false;
		std::string m_file_path = "";
		std::vector<uint8_t> m_file_binary;
		drflac* m_flac;
		WAVEFORMATEXTENSIBLE m_format;
	};

	// ここにWAVファイル用のオーディオストリームを書く

	// PCM用のオーディオストリーム
	class PCMAudioStream : public AudioStream {
	public:
		PCMAudioStream(std::vector<uint8_t> buffer, const WAVEFORMATEXTENSIBLE& format)
			: m_pcm_buffer(buffer), m_format(format) {}
		virtual ~PCMAudioStream() = default;
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

	static constexpr size_t audioChunkSize = 262'144; // (Bytes)
	std::mutex commandMutex;
	std::queue<Commands::Command> commandQueue;

	std::thread worker;
	std::atomic<bool> running;

	Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice* masterVoice;

	struct VoiceSlot {
		IXAudio2SourceVoice* voice = nullptr;
		WAVEFORMATEXTENSIBLE format;
		std::vector<std::unique_ptr<AudioStream>> pending;
		std::vector<std::vector<uint8_t>> buffersHeld;
		size_t pendingIndex = 0;

		bool holdBuffer = false;
		bool loop = false;
		size_t loopStartSamples = 0;
		size_t audioEndSamples = 0;
		size_t samplesPlayed = 0;

		static constexpr size_t NumBuffers = 3;
		std::array<std::vector<uint8_t>, NumBuffers> playBuffers;
		size_t next = 0;
	};
};
