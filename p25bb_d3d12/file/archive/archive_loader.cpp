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

std::future<std::vector<uint8_t>> ArchiveLoader::GetFileDataAsync(const Types::Entry& entry) const {
	auto start = entry.dataOffset / m_chunk_size;
	auto end = (entry.dataOffset + entry.dataSize - 1) / m_chunk_size;

	std::vector<std::future<std::array<uint8_t, m_chunk_size>>> chunkDataFutures;

	for (size_t i = start; i <= end; ++i) {
		chunkDataFutures.emplace_back(m_getChunkDataAsync(i));
	}

	auto func = [this, &entry](std::vector<std::future<std::array<uint8_t, m_chunk_size>>>&& futures) -> std::vector<uint8_t> {
		std::pair<size_t, size_t> clip = {
			entry.dataOffset % m_chunk_size,
			((entry.dataOffset + entry.dataSize) % m_chunk_size) == 0
				? 0 : (m_chunk_size - (entry.dataOffset + entry.dataSize) % m_chunk_size)
		};

		std::vector<uint8_t> fileData;
		for (auto& future : futures) {
			auto chunkData = future.get();
			fileData.insert(fileData.end(), chunkData.begin(), chunkData.end());
		}

		if (clip.first > 0) {
			fileData.erase(fileData.begin(), fileData.begin() + clip.first);
		}
		if (clip.second > 0) {
			fileData.erase(fileData.end() - clip.second, fileData.end());
		}

		return fileData;
	};

	return m_thread_pool->submit_task(
		[&]() {
			return func(std::move(chunkDataFutures));
		}
	);
}

std::future<std::array<uint8_t, ArchiveLoader::m_chunk_size>> ArchiveLoader::m_getChunkDataAsync(size_t chunkIndex) const {
	return m_thread_pool->submit_task(
		[this, chunkIndex]() -> std::array<uint8_t, m_chunk_size> {
			const auto& [offset, size] = m_data_chunk_ranges.at(chunkIndex);
			std::vector<uint8_t> compressed = std::vector<uint8_t>(size);
			m_readData(compressed.data(), size, m_data_start_position + offset);
			// ZStdDecompressは厳密にm_chunk_sizeバイトのデータを返すはずなので、サイズの確認は必要ない
			std::vector<uint8_t> decompressed = Helpers::Compress::ZStdDecompress(compressed, m_chunk_size);
			std::array<uint8_t, m_chunk_size> result;
			std::copy(decompressed.begin(), decompressed.end(), result.begin());
			return result;
		}
	);
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

void ArchiveLoader::m_constructEntries(const std::vector<uint8_t>& data, size_t& readPosition) {
	auto entryData
		= std::vector<uint8_t>(
			data.begin() + readPosition,
			data.begin() + readPosition + m_size_info.entryCompressedSize);

	readPosition += m_size_info.entryCompressedSize;

	auto decompressed = Helpers::Compress::ZStdDecompress(entryData, m_size_info.entryUncompressedSize);

	size_t decompressedReadPosition = 0;

	m_constructEntries(decompressed, m_entries, m_virtual_root_entry_indexes, decompressedReadPosition);
}

void ArchiveLoader::m_constructEntries(
	const std::vector<uint8_t>& data,
	std::vector<Types::Entry>& entries,
	std::unordered_map<std::string, EntryIndex>& entryIndexes,
	size_t& readPosition) {
	auto read = [this, &data, &readPosition]<typename T>(T* dest, size_t size = sizeof(T)) {
		m_readData(dest, data.data(), size, readPosition, data.size());
		readPosition += size;
	};

	// 実際は各エントリの最後の要素だけど、全体的に見ると先頭にはルート直下エントリの要素があるからこれでいいはず
	uint16_t entryCount = 0;
	read(&entryCount);

	for (size_t i = 0; i < entryCount; ++i) {
		Types::Entry entry = {};
		read(&entry.dataSize);
		read(&entry.dataOffset);
		read(&entry.nameLength);
		entry.name.resize(entry.nameLength);

		entry.ToNativeEndian();

		read(entry.name.data(), entry.name.size());

		entryIndexes[entry.name].index = static_cast<uint16_t>(i);

		m_constructEntries(data, entry.children, entryIndexes[entry.name].children, readPosition);
	}
}

void ArchiveLoader::m_loadDataChunkRanges(const std::vector<uint8_t>& data, size_t& readPosition) {
	auto compressedDataChunkIndexData
		= std::vector<uint8_t>(
			data.begin() + readPosition,
			data.begin() + readPosition + m_size_info.dataChunkIndexCompressedSize);
	readPosition += m_size_info.dataChunkIndexCompressedSize;

	auto dataChunkIndexData
		= Helpers::Compress::ZStdDecompress(
			compressedDataChunkIndexData,
			m_size_info.dataChunkIndexUncompressedSize);

	if (dataChunkIndexData.size() % sizeof(uint64_t) != 0) {
		throw Exceptions::FileError("Invalid data chunk index size.");

	}
	auto count = dataChunkIndexData.size() / sizeof(uint64_t);
	std::vector<uint64_t> indexes(count);
	memcpy(indexes.data(), dataChunkIndexData.data(), dataChunkIndexData.size());

	if (!indexes.empty() && indexes[0] != 0) {
		throw Exceptions::FileError("First chunk offset must be 0.");
	}

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

std::vector<uint16_t> ArchiveLoader::m_pathVectorToIndexVector(
	const std::vector<std::string>& path,
	const std::unordered_map<std::string, EntryIndex>& indexMap) const {
	const auto* currentIndexMap = &indexMap;
	std::vector<uint16_t> result;
	for (size_t i = 0; i < path.size(); ++i) {
		auto it = currentIndexMap->find(path[i]);
		if (it == currentIndexMap->end()) {
			return {};
		}

		result.emplace_back(it->second.index);
		currentIndexMap = &it->second.children;
	}

	return result;
}

bool ArchiveLoader::m_isExist(const std::vector<std::string>& path) const {
	return !m_pathVectorToIndexVector(path, m_virtual_root_entry_indexes).empty();
}


PameECS::File::Archive::Types::Entry ArchiveLoader::m_getEntry(const std::vector<std::string>& path) const {
	auto indexPath = m_pathVectorToIndexVector(path, m_virtual_root_entry_indexes);
	if (indexPath.empty()) {
		throw Exceptions::FileError("No entry found for the given path.");
	}

	try {
		const Types::Entry* currentEntry = &m_entries.at(indexPath[0]);
		for (size_t i = 1; i < indexPath.size(); ++i) {
			currentEntry = &currentEntry->children.at(indexPath[i]);
		}

		// currentEntryがnullptrになるわけがないから、nullチェックは不要
		return *currentEntry;
	}
	catch (const std::out_of_range) {
		// indexPathの元になるデータを生成するのはアーカイブのロード時なので、このエラーメッセージ
		throw Exceptions::FileError("Invalid child index is generated during archive file is loaded.");
	}
}
