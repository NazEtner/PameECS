#pragma once
#include <cstdint>
#include <array>
#include <string>

namespace PameECS::File::Archive::Types {
	struct Header {
		char magic[4];
		uint8_t versionMajor;
		uint8_t versionMinor;
		uint8_t versionPatch;
		uint8_t reserved;

		bool IsValid() const {
			bool isMagicValid =
				magic[0] == 'P' &&
				magic[1] == 'E' &&
				magic[2] == 'A' &&
				magic[3] == 'C';

			return isMagicValid;
		}
	};

	struct SizeInformation {
		uint32_t entryCompressedSize;
		uint32_t entryUncompressedSize;
		uint64_t dataChunkIndexCompressedSize;
		uint64_t dataChunkIndexUncompressedSize;
		uint64_t totalDataChunkCompressedSize;
	};

	struct Entry {
		uint64_t dataSize;
		uint64_t dataOffset;
		uint16_t nameLength;
		std::string name;
	};
}
