#include "system.hpp"

using PameECS::ECS::System::Dependencies;

struct Dependencies::Impl {
	Impl(const std::vector<size_t>& write, const std::vector<size_t>& read)
		: writeDependencies(write), readDependencies(read) {}
	const std::vector<size_t> writeDependencies;
	const std::vector<size_t> readDependencies;
};

// exportするもの
// これ以外はexportしない
Dependencies::Dependencies(const size_t* write, const size_t writeSize, const size_t* read, const size_t readSize)
	: Dependencies(
		write ? std::vector<size_t>(write, write + writeSize) : std::vector<size_t>(),
		read ? std::vector<size_t>(read, read + readSize) : std::vector<size_t>()) { }

Dependencies::Dependencies(const std::vector<size_t>& write, const std::vector<size_t>& read) {
	m_impl = std::make_unique<Impl>(write, read);
}

Dependencies::~Dependencies() {
}

const std::vector<size_t>& Dependencies::GetWriteDependencies() const {
	return m_impl->writeDependencies;
}

const std::vector<size_t>& Dependencies::GetReadDependencies() const {
	return m_impl->readDependencies;
}

extern "C" {
	PECS_DLL_SHARED PameECS::ECS::System::Dependencies* ECSSDependenciesConstruct(const size_t* write, const size_t writeSize, const size_t* read, const size_t readSize) {
		return new Dependencies(write, writeSize, read, readSize);
	}
	PECS_DLL_SHARED void ECSSDependenciesDestruct(PameECS::ECS::System::Dependencies* dependencies) {
		delete dependencies;
	}
}
