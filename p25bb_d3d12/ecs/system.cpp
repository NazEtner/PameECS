#include "system.hpp"

using PameECS::ECS::System::Dependencies;

struct Dependencies::Impl {
	Impl(const std::vector<size_t>& write, const std::vector<size_t>& read)
		: writeDependencies(write), readDependencies(read) {}
	const std::vector<size_t> writeDependencies;
	const std::vector<size_t> readDependencies;
};

Dependencies::Dependencies(const std::vector<size_t>& write, const std::vector<size_t>& read) {
	m_impl = std::make_unique<Impl>(write, read);
}

const std::vector<size_t> Dependencies::GetWriteDependencies() const {
	return m_impl->writeDependencies;
}

const std::vector<size_t> Dependencies::GetReadDependencies() const {
	return m_impl->readDependencies;
}
