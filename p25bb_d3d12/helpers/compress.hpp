#include <zstd/zstd.h>
#include <vector>

#include "../exceptions/compress_error.hpp"

namespace PameECS::Helpers::Compress {
	inline std::vector<uint8_t> ZStdCompress(const std::vector<uint8_t>& data, int compressionLevel = 3) {
		size_t compressedSize = ZSTD_compressBound(data.size());
		if (ZSTD_isError(compressedSize)) {
			throw Exceptions::CompressError("ZStd compression bound calculation failed");
		}
		std::vector<uint8_t> compressedData(compressedSize);
		compressedSize = ZSTD_compress(compressedData.data(), compressedData.size(), data.data(), data.size(), compressionLevel);
		if (ZSTD_isError(compressedSize)) {
			throw Exceptions::CompressError("ZStd compression failed");
		}
		compressedData.resize(compressedSize);
		return compressedData;
	}

	inline std::vector<uint8_t> ZStdDecompress(const std::vector<uint8_t>& data, size_t decompressedSize) {
		std::vector<uint8_t> decompressedData(decompressedSize);
		size_t result = ZSTD_decompress(decompressedData.data(), decompressedData.size(), data.data(), data.size());
		if (ZSTD_isError(result)) {
			throw Exceptions::CompressError("ZStd decompression failed.");
		}
		return decompressedData;
	}

	inline std::vector<uint8_t> ZStdDecompress(const std::vector<uint8_t>& data) {
		size_t decompressedSize = ZSTD_getFrameContentSize(data.data(), data.size());
		if (decompressedSize == ZSTD_CONTENTSIZE_ERROR) {
			throw Exceptions::CompressError("ZStd decompression size error");
		}
		else if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
			throw Exceptions::CompressError("ZStd decompression size unknown");
		}
		return ZStdDecompress(data, decompressedSize);
	}
}
