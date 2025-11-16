#pragma once
#include <filesystem>
#include <memory>
#include <future>
#include <vector>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>

#include "types.hpp"
#include "../../helpers/path.hpp"

namespace PameECS::File::Archive {
	class ArchiveLoader {
	public:
		ArchiveLoader(const std::filesystem::path& path, std::shared_ptr<BS::thread_pool<0U>> threadPool);
		~ArchiveLoader() = default;
		ArchiveLoader(const ArchiveLoader&) = delete;
		ArchiveLoader& operator=(const ArchiveLoader&) = delete;
		ArchiveLoader(ArchiveLoader&&) = default;
		ArchiveLoader& operator=(ArchiveLoader&&) = default;

		Types::Entry GetEntry(const std::string& virtualPath) const;

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

		bool IsExist(const std::string& virtualPath) const;

		bool IsFile(const Types::Entry& entry) const;
		bool IsDirectory(const Types::Entry& entry) const;
	private:
		inline static constexpr size_t m_chunk_size = 2048;
	};
}
