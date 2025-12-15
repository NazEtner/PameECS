#include "system.hpp"

using PameECS::ECS::System::Dependencies;

struct Dependencies::Impl {
	Impl(const std::vector<std::string>& write, const std::vector<std::string>& read)
		: writeDependencies(write), readDependencies(read) {}
	const std::vector<std::string> writeDependencies;
	const std::vector<std::string> readDependencies;
};

Dependencies::Dependencies(const std::vector<std::string>& write, const std::vector<std::string>& read) {
	m_impl = std::make_unique<Impl>(write, read);
}

const std::vector<std::string> Dependencies::GetWriteDependencies() const {
	return m_impl->writeDependencies;
}

const std::vector<std::string> Dependencies::GetReadDependencies() const {
	return m_impl->readDependencies;
}
