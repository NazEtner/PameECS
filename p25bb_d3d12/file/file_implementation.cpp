#include "file_implementation.hpp"

using PameECS::File::FileImplementation;

struct FileImplementation::Impl {
	std::vector<std::shared_ptr<Archive::ArchiveLoader>> archives;
	std::shared_ptr<BS::thread_pool<0U>> threadPool;
};

FileImplementation::FileImplementation(std::shared_ptr<BS::thread_pool<0U>> threadPool) {
	assert(threadPool);
	m_impl = std::make_shared<Impl>();
	m_impl->threadPool = threadPool;
}

FileImplementation::~FileImplementation() {
	m_impl->threadPool->wait();
	m_impl->threadPool.reset();
}

void FileImplementation::SetArchiveLoader(std::shared_ptr<Archive::ArchiveLoader> archiveLoader) {
	m_impl->archives.emplace_back(archiveLoader);
}

std::future<std::stringstream> FileImplementation::Load(const std::string& path) {
	return m_impl->threadPool->submit_task(
		[path, impl = m_impl]() -> std::stringstream {
			for (const auto& archive : impl->archives) {
				if (!archive) continue;
				if (archive->IsExist(path)) {
					auto entry = archive->GetEntry(path);
					if (archive->IsFile(entry)) {
						auto fileData = archive->GetFileData(entry);
						std::stringstream ss;
						ss.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
						return ss;
					}
				}
			}

			std::ifstream ifs(path, std::ios::binary);
			std::stringstream ss;
			ss << ifs.rdbuf();
			return ss;
		}
	);
}
