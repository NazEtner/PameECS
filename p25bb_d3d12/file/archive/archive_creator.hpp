#pragma once
#include "types.hpp"
#include <filesystem>
#include <limits>

#include "../../exceptions/file_error.hpp"

namespace PameECS::File::Archive {
	class ArchiveCreator {
	public:
		ArchiveCreator() = default;
		void AddFileToVirtualRoot(const std::filesystem::path& path) {
			m_addFile(path, m_virtual_root);
		}
		void AddDirectoryToVirtualRoot(const std::filesystem::path& path) {
			m_collectEntriesFromDirectory(path, m_virtual_root);
		}

		void Write(const std::filesystem::path& path);
	private:
		inline static constexpr size_t m_chunk_size = Types::ChunkSize;
		inline static constexpr std::array<uint8_t, 3> m_version = { 1, 0, 0 };

		template<typename T>
		T m_toExpectedEndian(T value) {
			return Helpers::Binary::ToNativeEndian<T, std::endian::native, std::endian::little>(value);
		}

		void m_collectEntriesFromDirectory(const std::filesystem::path& path, std::vector<Types::Entry>& target);
		void m_addFile(const std::filesystem::path& path, std::vector<Types::Entry>& target);

		std::vector<std::array<uint8_t, m_chunk_size>> m_createChunks() const;
		std::vector<std::vector<uint8_t>> m_compressChunks(const std::vector<std::array<uint8_t, m_chunk_size>>& chunks) const;
		std::vector<uint64_t> m_createChunkIndexes(const std::vector<std::vector<uint8_t>>& chunks) const;

		Types::Entry m_createEntry(const std::filesystem::path& path, uint64_t offset, uint64_t size) {
			Types::Entry entry;
			auto fileName = path.filename().string();
			if (fileName.size() >= std::numeric_limits<uint16_t>::max())
				throw Exceptions::FileError("File name is too long");
			entry.dataSize = size;
			entry.dataOffset = offset;
			entry.nameLength = static_cast<uint16_t>(fileName.size());
			entry.name = fileName;
			return entry;
		}

		Types::Entry m_createDirectoryEntry(const std::filesystem::path& path) {
			return m_createEntry(path, 0, 0);
		}

		template<typename T>
		std::vector<uint8_t> m_serialize(const T& value) const {
			static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");
			std::vector<uint8_t> result(sizeof(T));
			memcpy(result.data(), &value, result.size());
			return result;
		}

		template<typename T>
		std::vector<uint8_t> m_serialize(const std::vector<T>& values) const {
			std::vector<uint8_t> result;
			for (const auto& value : values) {
				auto valueBin = m_serialize(value);
				result.insert(result.end(), valueBin.begin(), valueBin.end());
			}

			return result;
		}

		std::vector<uint8_t> m_raw_data;
		std::vector<Types::Entry> m_virtual_root;
	};
}
