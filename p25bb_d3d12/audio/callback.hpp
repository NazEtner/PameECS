#pragma once
#include <cstdint>
#include <cstring>
#include <windows.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

namespace PameECS::Audio {
	class Callback {
	public:
		virtual ~Callback() = default;
		using CallbackFunction = size_t(*)(void* userData, uint8_t* dest, size_t samples);
		virtual CallbackFunction GetCallback() = 0;
		virtual WAVEFORMATEXTENSIBLE GetFormat() = 0;
		virtual void* GetUserData() = 0;
		virtual bool IsValid() = 0;
		virtual bool IsExpired() = 0;
	protected:
		WAVEFORMATEXTENSIBLE m_fillFormat(uint16_t channels, uint16_t bits, uint32_t rate, bool isFloat) {
			WAVEFORMATEXTENSIBLE format;
			memset(&format, 0, sizeof(format));
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
	};
}
