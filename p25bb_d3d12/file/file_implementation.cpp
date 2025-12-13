#include "file_implementation.hpp"
#include "../exceptions/invalid_operation.hpp"
#include <condition_variable>

using PameECS::File::FileImplementation;

struct FileImplementation::Impl {
	std::vector<std::shared_ptr<Archive::ArchiveLoader>> archives;
	std::shared_ptr<BS::thread_pool<0U>> threadPool;
	std::mutex mutex;
	std::condition_variable conditionVariable;
	size_t taskCount = 0;
	bool stopRequested = false;
};

namespace {
	class TaskCounter {
	public:
		TaskCounter(size_t& counter, std::condition_variable& cv): m_counter(counter), m_cv(cv) {
			m_counter++;
		}
		~TaskCounter() {
			m_counter--;
			m_cv.notify_all();
		}
	private:
		size_t& m_counter;
		std::condition_variable& m_cv;
	};
}

FileImplementation::FileImplementation(std::shared_ptr<BS::thread_pool<0U>> threadPool) {
	assert(threadPool);
	m_impl = new Impl;
	m_impl->threadPool = threadPool;
}

FileImplementation::~FileImplementation() {
	{
		std::unique_lock lock(m_impl->mutex);
		m_impl->stopRequested = true;
		m_impl->conditionVariable.wait(lock, [impl = m_impl]() { return impl->taskCount == 0; });
	}

	delete m_impl;
}

void FileImplementation::SetArchiveLoader(std::shared_ptr<Archive::ArchiveLoader> archiveLoader) {
	std::unique_lock lock(m_impl->mutex);
	m_impl->archives.emplace_back(archiveLoader);
}

std::future<std::stringstream> FileImplementation::Load(const std::string& path) {
	return m_impl->threadPool->submit_task(
		[path, impl = m_impl]() -> std::stringstream {
			std::vector<std::shared_ptr<Archive::ArchiveLoader>> archivesCopy;
			{
				std::unique_lock lock(impl->mutex);
				if (impl->stopRequested) {
					throw PameECS::Exceptions::InvalidOperation("FileImplementation is being destroyed, cannot load files.");
				}
				TaskCounter counter(impl->taskCount, impl->conditionVariable);
				archivesCopy = impl->archives;
			}
			for (const auto& archive : archivesCopy) {
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
