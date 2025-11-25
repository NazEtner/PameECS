#include "archive_creator.hpp"
#include "../../helpers/compress.hpp"
#include "../../helpers/crc.hpp"

#include <fstream>

using PameECS::File::Archive::ArchiveCreator;

void ArchiveCreator::Write(const std::filesystem::path& path) {
	Types::Header header = {};
	static_assert(sizeof(header.magic) == 4);
	memcpy(header.magic, "PEAC", sizeof(header.magic));
	header.versionMajor = m_version[0];
	header.versionMinor = m_version[1];
	header.versionPatch = m_version[2];

	header.ToExpectedEndian();

	auto headerBinary = m_serialize(header);

	if (m_virtual_root.size() >= std::numeric_limits<uint16_t>::max())
		throw Exceptions::FileError("Too many entries in virtual root.");
	auto rawEntryBinary = m_serialize(m_virtual_root);
	auto entryCount = m_serialize(static_cast<uint16_t>(m_virtual_root.size()));
	rawEntryBinary.insert(rawEntryBinary.begin(), entryCount.begin(), entryCount.end());
	auto entryBinary = Helpers::Compress::ZStdCompress(rawEntryBinary);

	auto dataChunks = m_compressChunks(m_createChunks());
	auto rawIndexesBianry = m_serialize(m_createChunkIndexes(dataChunks));
	auto indexesBinary = Helpers::Compress::ZStdCompress(rawIndexesBianry);

	std::vector<uint8_t> dataChunkBinary;
	for (const auto& chunk : dataChunks) {
		auto serialized = m_serialize(chunk);
		dataChunkBinary.insert(dataChunkBinary.end(), serialized.begin(), serialized.end());
	}

	Types::SizeInformation sizeInfo;
	sizeInfo.entryCompressedSize = static_cast<uint32_t>(entryBinary.size());
	sizeInfo.entryUncompressedSize = static_cast<uint32_t>(rawEntryBinary.size());
	sizeInfo.dataChunkIndexCompressedSize = indexesBinary.size();
	sizeInfo.dataChunkIndexUncompressedSize = rawIndexesBianry.size();
	sizeInfo.totalDataChunkCompressedSize = dataChunkBinary.size();

	sizeInfo.ToExpectedEndian();

	auto sizeInfoBinary = m_serialize(sizeInfo);

	auto compressedPart = entryBinary;
	compressedPart.insert(compressedPart.end(), indexesBinary.begin(), indexesBinary.end());
	compressedPart.insert(compressedPart.end(), dataChunkBinary.begin(), dataChunkBinary.end());

	Helpers::CRC::CRC64ECMACalculator crcCalculator;
	uint64_t check = crcCalculator.Calculate(compressedPart);

	std::vector<uint8_t> dataToWrite;
	auto add = [&](const std::vector<uint8_t>& data) {
		dataToWrite.insert(dataToWrite.end(), data.begin(), data.end());
	};

	add(headerBinary);
	add(sizeInfoBinary);
	add(compressedPart);
	add(m_serialize(m_toExpectedEndian(check)));

	std::ofstream file(path, std::ios::binary);
	if (!file) {
		throw Exceptions::FileError("Failed to open file for writing: " + path.string());
	}
	if (!file.write(reinterpret_cast<const char*>(dataToWrite.data()), dataToWrite.size())) {
		throw Exceptions::FileError("Failed to write archive data to file: " + path.string());
	}
	file.close();
}

template<>
std::vector<uint8_t> ArchiveCreator::m_serialize(const PameECS::File::Archive::Types::Entry& value) const {
	std::vector<uint8_t> result;

	auto addData = [&]<typename T>(const T * data, const size_t size = sizeof(T)) {
		result.insert(result.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
	};

	addData(&value.dataSize);
	addData(&value.dataOffset);

	if (value.nameLength != value.name.size()) throw Exceptions::FileError("Invalid file name length.");

	addData(&value.nameLength);
	addData(value.name.data(), value.nameLength);
	uint16_t childCount = static_cast<uint16_t>(value.children.size());
	if (value.children.size() != childCount) throw Exceptions::FileError("Child count is too large.");
	addData(&childCount);
	for (uint16_t i = 0; i < childCount; ++i) {
		auto childrenBinary = m_serialize(value.children[i]);
		result.insert(result.end(), childrenBinary.begin(), childrenBinary.end());
	}

	return result;
}

void ArchiveCreator::m_addFile(const std::filesystem::path& path, std::vector<Types::Entry>& target) {
	if (!std::filesystem::is_regular_file(path)) {
		throw Exceptions::FileError("Path is not a regular file: " + path.string());
	}
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		throw Exceptions::FileError("Failed to open file: " + path.string());
	}
	size_t size = std::filesystem::file_size(path);
	Types::Entry entry = m_createEntry(path, m_raw_data.size(), size);
	entry.ToExpectedEndian();
	target.emplace_back(entry);

	auto fileData = std::vector<uint8_t>(size);
	if (!file.read(reinterpret_cast<char*>(fileData.data()), size)) {
		throw Exceptions::FileError("Failed to read file: " + path.string());
	}
	file.close();

	m_raw_data.insert(m_raw_data.end(), fileData.begin(), fileData.end());
}

void ArchiveCreator::m_collectEntriesFromDirectory(const std::filesystem::path& path, std::vector<Types::Entry>& target) {
	if (!std::filesystem::is_directory(path)) {
		throw Exceptions::FileError("Path is not a directory: " + path.string());
	}

	auto entry = m_createDirectoryEntry(path);
	for (auto& p : std::filesystem::directory_iterator(path)) {
		if (std::filesystem::is_directory(p)) {
			m_collectEntriesFromDirectory(p, entry.children);
		}
		else if (std::filesystem::is_regular_file(p)) {
			m_addFile(p, entry.children);
		}
	}

	entry.ToExpectedEndian();
	target.emplace_back(entry);
}

std::vector<std::array<uint8_t, ArchiveCreator::m_chunk_size>> ArchiveCreator::m_createChunks() const {
	std::vector<std::array<uint8_t, m_chunk_size>> chunks;
	size_t totalSize = m_raw_data.size();
	size_t offset = 0;
	static_assert(m_chunk_size > 0);
	while (offset < totalSize) {
		std::array<uint8_t, m_chunk_size> chunk = {};
		size_t chunkSize = std::min(m_chunk_size, totalSize - offset);
		std::copy(m_raw_data.begin() + offset, m_raw_data.begin() + offset + chunkSize, chunk.data());
		chunks.emplace_back(chunk);
		offset += chunkSize;
	}

	return chunks;
}

std::vector<std::vector<uint8_t>> ArchiveCreator::m_compressChunks(const std::vector<std::array<uint8_t, m_chunk_size>>& chunks) const {
	std::vector<std::vector<uint8_t>> compressedChunks;

	for (const auto& chunk : chunks) {
		auto compressed = Helpers::Compress::ZStdCompress(
			std::vector<uint8_t>(chunk.begin(), chunk.end())
		);
		compressedChunks.emplace_back(compressed);
	}

	return compressedChunks;
}

std::vector<uint64_t> ArchiveCreator::m_createChunkIndexes(const std::vector<std::vector<uint8_t>>& chunks) const {
	size_t start = 0;
	std::vector<uint64_t> indexes;
	for (const auto& chunk : chunks) {
		indexes.emplace_back(start);
		start += chunk.size();
	}

	return indexes;
}
