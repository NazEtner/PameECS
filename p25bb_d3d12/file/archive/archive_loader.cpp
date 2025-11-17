#include "archive_loader.hpp"
#include "../../helpers/compress.hpp"
#include "../../helpers/crc.hpp"
#include "../../exceptions/file_error.hpp"

using PameECS::File::Archive::ArchiveLoader;

ArchiveLoader::ArchiveLoader(const std::filesystem::path& path, std::shared_ptr<BS::thread_pool<0U>> threadPool, std::shared_ptr<spdlog::logger> logger)
	: m_thread_pool(threadPool), m_logger(logger) {
	assert(m_thread_pool);
	assert(m_logger);

	m_fileMap(path);
	m_loadAndVerifyHeader();
	m_loadSizeInformationFromLastRead();
	size_t sizeToRead =
		m_size_info.entryCompressedSize
		+ m_size_info.dataChunkIndexCompressedSize
		+ m_size_info.totalDataChunkCompressedSize;
	std::vector<uint8_t> data(sizeToRead);
	m_readData(data.data(), sizeToRead);
	m_loadAndVerifyFooterFromLastRead(data);

	size_t readPosition = 0;

	m_constructEntries(data, readPosition);
	m_loadDataChunkRanges(data, readPosition);

	m_data_start_position = sizeof(Types::Header)
		+ sizeof(Types::SizeInformation)
		+ m_size_info.entryCompressedSize
		+ m_size_info.dataChunkIndexCompressedSize;
}

void ArchiveLoader::m_fileMap(const std::filesystem::path& path) {
	m_file_map = boost::interprocess::file_mapping(path.c_str(), boost::interprocess::read_only);
	m_file_view = boost::interprocess::mapped_region(m_file_map, boost::interprocess::read_only);
}

void ArchiveLoader::m_loadAndVerifyHeader() {
	const std::array<std::array<uint8_t, 3>, 1> expectedVersions = {
		{1, 0, 0},
	};

	Types::Header header;
	m_readData(&header, sizeof(Types::Header));
	header.ToNativeEndian();
	m_logger->debug(header.GenerateDebugString());
	if (!header.IsValid()) {
		throw Exceptions::FileError("Invalid archive header.");
	}
	bool isVersionValid = false;
	for (const auto& version : expectedVersions) {
		if (header.versionMajor == version[0]
			&& header.versionMinor == version[1]
			&& header.versionPatch == version[2]) {
			isVersionValid = true;
			break;
		}
	}
	if (!isVersionValid) {
		throw Exceptions::FileError("Unsupported archive version.");
	}
}

void ArchiveLoader::m_loadSizeInformationFromLastRead() {
	m_readData(&m_size_info, sizeof(Types::SizeInformation));
	m_size_info.ToNativeEndian();
	m_logger->debug(m_size_info.GenerateDebugString());
}

void ArchiveLoader::m_constructEntries(std::vector<uint8_t>& data, size_t& readPosition) {
	auto entryData = std::vector<uint8_t>(data.begin() + readPosition, data.begin() + readPosition + m_size_info.entryCompressedSize);
	readPosition += m_size_info.entryCompressedSize;

	auto decompressed = Helpers::Compress::ZStdDecompress(entryData, m_size_info.entryUncompressedSize);
	m_constructEntries(decompressed, m_entries, m_virtual_root_entry_indexes);
}

void ArchiveLoader::m_constructEntries(std::vector<uint8_t>& data, std::vector<Types::Entry>& entries, std::unordered_map<std::string, EntryIndex>& entryIndexes) {
}

void ArchiveLoader::m_loadDataChunkRanges(std::vector<uint8_t>& data, size_t& readPosition) {
	auto compressedDataChunkIndexData = std::vector<uint8_t>(data.begin() + readPosition, data.begin() + readPosition + m_size_info.dataChunkIndexCompressedSize);
	readPosition += m_size_info.dataChunkIndexCompressedSize;

	auto dataChunkIndexData = Helpers::Compress::ZStdDecompress(compressedDataChunkIndexData, m_size_info.dataChunkIndexUncompressedSize);

	if (dataChunkIndexData.size() % sizeof(uint64_t) != 0) {
		throw Exceptions::FileError("Invalid data chunk index size.");

	}
	auto count = dataChunkIndexData.size() / sizeof(uint64_t);
	std::vector<uint64_t> indexes(count);
	memcpy(indexes.data(), dataChunkIndexData.data(), dataChunkIndexData.size());
	indexes.emplace_back(m_size_info.totalDataChunkCompressedSize);

	for (size_t i = 1; i < indexes.size(); ++i) {
		m_data_chunk_ranges.emplace_back(
			indexes[i - 1], indexes[i] - indexes[i - 1]
		);
	}

	if (m_data_chunk_ranges.empty()) {
		throw Exceptions::FileError("No data chunk ranges found.");
	}
}

void ArchiveLoader::m_loadAndVerifyFooterFromLastRead(const std::vector<uint8_t>& checkTarget) {
	uint64_t footer = 0;
	m_readData(&footer, sizeof(uint64_t));
	footer = m_toNativeEndian(footer);
	Helpers::CRC::CRC64ECMACalculator crcCalculator;
	auto calculated = crcCalculator.Calculate(checkTarget);
	if (footer != calculated) {
		throw Exceptions::FileError("Archive footer CRC64 verification failed.");
	}
}
