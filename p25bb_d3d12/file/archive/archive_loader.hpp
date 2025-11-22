#pragma once
#include <filesystem>
#include <memory>
#include <future>
#include <vector>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <spdlog/logger.h>

#include "types.hpp"
#include "../../helpers/path.hpp"
#include "../../helpers/binary.hpp"
#include "../../exceptions/file_error.hpp"

namespace PameECS::File::Archive {
	class ArchiveLoader {
	public:
		ArchiveLoader(const std::filesystem::path& path, std::shared_ptr<BS::thread_pool<0U>> threadPool, std::shared_ptr<spdlog::logger> logger);
		~ArchiveLoader() = default;
		ArchiveLoader(const ArchiveLoader&) = delete;
		ArchiveLoader& operator=(const ArchiveLoader&) = delete;
		ArchiveLoader(ArchiveLoader&&) = default;
		ArchiveLoader& operator=(ArchiveLoader&&) = default;

		Types::Entry GetEntry(const std::string& virtualPath) const {
			return m_getEntry(Helpers::Path::PathToVector(virtualPath));
		}

		std::vector<uint8_t> GetFileData(const std::string& virtualPath) const {
			GetFileData(GetEntry(virtualPath));
		}
		std::vector<uint8_t> GetFileData(const Types::Entry& entry) const {
			GetFileDataAsync(entry).get();
		}
		std::future<std::vector<uint8_t>> GetFileDataAsync(const std::string& virtualPath) const {
			return GetFileDataAsync(GetEntry(virtualPath));
		}
		std::future<std::vector<uint8_t>> GetFileDataAsync(const Types::Entry& entry) const;

		bool IsExist(const std::string& virtualPath) const {
			return m_isExist(Helpers::Path::PathToVector(virtualPath));
		}

		bool IsFile(const Types::Entry& entry) const {
			return entry.dataSize != 0;
		}
		bool IsDirectory(const Types::Entry& entry) const {
			return !IsFile(entry);
		}
	private:
		bool m_canRead(const size_t sourceSize, const size_t bytes, const size_t position) const noexcept {
			return position + bytes <= sourceSize;
		}

		void m_readData(void* dest, const void* source, const size_t bytes, const size_t position, const size_t sourceSize) const {
			if (!m_canRead(sourceSize, bytes, position)) {
				throw Exceptions::FileError("Attempted to read beyond the end of the mapped file.");
			}
			std::memcpy(dest, static_cast<const uint8_t*>(source) + position, bytes);
		}

		void m_readData(void* buffer, const size_t bytes, const size_t position) const {
			m_readData(buffer, m_file_view.get_address(), bytes, position, m_file_view.get_size());
		}

		void m_readData(void* buffer, const size_t bytes) {
			m_readData(buffer, bytes, m_last_read);
			m_last_read += bytes;
		}

		template<typename T>
		T m_toNativeEndian(T value) {
			return Helpers::Binary::ToNativeEndian<T, std::endian::little>(value);
		}
			
		struct EntryIndex {
			uint16_t index = 0; // エントリのインデックス
			std::unordered_map<std::string, EntryIndex> children; // 子エントリのインデックス
		};

		inline static constexpr size_t m_chunk_size = 2048;

		std::future<std::array<uint8_t, m_chunk_size>> m_getChunkDataAsync(size_t chunkIndex) const;

		void m_fileMap(const std::filesystem::path& path);
		void m_loadAndVerifyHeader();
		void m_loadSizeInformationFromLastRead();
		void m_constructEntries(const std::vector<uint8_t>& data, size_t& readPosition);
		void m_constructEntries(const std::vector<uint8_t>& data, std::vector<Types::Entry>& entries, std::unordered_map<std::string, EntryIndex>& entryIndexes, size_t& readPosition);
		void m_loadDataChunkRanges(const std::vector<uint8_t>& data, size_t& readPosition);
		void m_loadAndVerifyFooterFromLastRead(const std::vector<uint8_t>& checkTarget);

		std::vector<uint16_t> m_pathVectorToIndexVector(
			const std::vector<std::string>& path,
			const std::unordered_map<std::string, EntryIndex>& indexMap) const;
		bool m_isExist(const std::vector<std::string>& path) const;
		Types::Entry m_getEntry(const std::vector<std::string>& path) const;

		std::shared_ptr<BS::thread_pool<0U>> m_thread_pool;

		Types::SizeInformation m_size_info;
		std::unordered_map<std::string, EntryIndex> m_virtual_root_entry_indexes;
		std::vector<Types::Entry> m_entries;
		std::vector<std::pair<uint64_t, uint64_t>> m_data_chunk_ranges; // (offset, size)

		uint64_t m_data_start_position = 0;

		uint64_t m_last_read = 0;

		boost::interprocess::file_mapping m_file_map;
		boost::interprocess::mapped_region m_file_view;

		std::shared_ptr<spdlog::logger> m_logger;
	};
}
