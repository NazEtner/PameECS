#pragma once
#include "archive/archive_loader.hpp"
#include <vector>
#include <fstream>
#include <future>
#include <sstream>
#include <string>
#include <memory>
#include "../macros/dll.hpp"
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>

namespace PameECS::File {
	class FileImplementation final {
	public:
		FileImplementation(std::shared_ptr<BS::thread_pool<0U>> threadPool);
		~FileImplementation();

		FileImplementation(const FileImplementation&) = delete;
		FileImplementation& operator=(const FileImplementation&) = delete;
		FileImplementation(FileImplementation&&) = delete;
		FileImplementation& operator=(FileImplementation&&) = delete;

		std::future<std::stringstream> Load(const std::string& path);
		void SetArchiveLoader(std::shared_ptr<Archive::ArchiveLoader> archiveLoader);
	private:
		struct Impl;
		Impl* m_impl;
	};
}
