#include "ecs_host.hpp"
#include "../helpers/id_generator.hpp"

using PameECS::ECS::ECSHost;
using PameECS::ECS::IComponentStorage;

struct ECSHost::Impl {
	
};

ECSHost::ECSHost() {
	if (m_impl == nullptr) {
		m_impl = new Impl();
	}
	m_ref_count++;
}

ECSHost::~ECSHost() {
	if (m_ref_count > 0) {
		m_ref_count--;
	}
	if (m_ref_count == 0) {
		delete m_impl;
		m_impl = nullptr;
	}
}

bool ECSHost::m_registerComponentStorage(const std::string& id, std::shared_ptr<IComponentStorage> storage) {
	// 仮実装
	return true;
}

std::shared_ptr<IComponentStorage> ECSHost::m_getComponentStorage(const std::string& id) {
	// 仮実装
	return nullptr;
}
