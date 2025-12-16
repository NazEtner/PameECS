#include "ecs_host.hpp"
#include "../helpers/id_generator.hpp"
#include "../helpers/container.hpp"

using PameECS::ECS::ECSHost;
using PameECS::ECS::IComponentStorage;

struct ECSHost::Impl {
	size_t refCount = 0;
	struct Tag {};
	Helpers::IdGenerator<true, true, Tag, 0> idGenerator;

	std::vector<std::shared_ptr<IComponentStorage>> componentStorages;
	std::vector<uint64_t> entityGenerations;
	std::vector<bool> entityAliveFlags;
	size_t lastEntityId = 0;
	bool locked = false;
	std::unordered_set<size_t> unlocked;

	void ResizeComponents(size_t minSize) {
		// 少なくともminSize以上の容量を確保する
		m_resize(minSize, componentStorages);
	}

	void ResizeEntities(size_t minSize) {
		m_resize(minSize, entityGenerations);
		m_resize(minSize, entityAliveFlags);
	}
private:
	template<typename T>
	void m_resize(size_t id, std::vector<T>& vec) {
		Helpers::Container::ResizePow2(vec, id, T());
	}
};

ECSHost::ECSHost() {
	if (m_impl == nullptr) {
		m_impl = new Impl();
	}
	m_impl->refCount++;
}

ECSHost::~ECSHost() {
	if (m_impl->refCount > 0) {
		m_impl->refCount--;
	}
	if (m_impl->refCount == 0) {
		// 本当はunique_ptrの方が良いけど、Pimplだと生ポインタの方が自然だというのと、
		// __declspec(dllexport)での警告が出るのでその回避のため
		delete m_impl;
		m_impl = nullptr;
	}
}

void ECSHost::LockAll() {
	m_impl->unlocked.clear();
	m_impl->locked = true;
}

void ECSHost::UnlockAll() {
	m_impl->unlocked.clear();
	m_impl->locked = false;
}

void ECSHost::Unlock(const std::unordered_set<size_t>& ids) {
	for (const auto& id : ids) {
		m_impl->unlocked.insert(id);
	}
}

size_t ECSHost::GetComponentStorageId(const std::string& name) {
	return m_impl->idGenerator.GetId(name);
}

bool ECSHost::NewEntity(Types::Entity& entity, const std::vector<std::string>& components,
	size_t idMin, size_t idMax) {
	auto id = idMin;
	while (id <= idMax) {
		m_impl->ResizeEntities(id + 1);
		if (!m_impl->entityAliveFlags[id]) {
			entity.id = id;
			entity.generation = ++m_impl->entityGenerations[id];
			m_impl->entityAliveFlags[id] = true;
			m_impl->lastEntityId = std::max(m_impl->lastEntityId, id);
			// コンポーネントの追加
			for (const auto& compId : components) {
				auto storage = m_getComponentStorage(compId);
				[[maybe_unused]]
				bool added = storage && storage->AddComponent(entity);
				// 失敗しても無視する
			}
			return true;
		}
		id++;
	}
	return false;
}

bool ECSHost::RemoveEntity(const Types::Entity& entity) {
	auto id = entity.id;
	if (id >= m_impl->entityAliveFlags.size() || !m_impl->entityAliveFlags[id]) {
		return false;
	}
	m_impl->entityAliveFlags[id] = false;
	return true;
}

bool ECSHost::AddComponent(const Types::Entity& entity, const std::string& component) {
	auto storage = m_getComponentStorage(component);
	if (storage == nullptr) {
		return false;
	}
	return storage->AddComponent(entity);
}

bool ECSHost::RemoveComponent(const Types::Entity& entity, const std::string& component) {
	auto storage = m_getComponentStorage(component);
	if (storage == nullptr) {
		return false;
	}
	Types::Entity dummyEntity = { entity.id, 0xFFFFFFFF'FFFFFFFF };
	return storage->AddComponent(dummyEntity);
}

bool ECSHost::m_registerComponentStorage(const std::string& id, std::shared_ptr<IComponentStorage> storage) {
	auto index = m_impl->idGenerator.GetId(id);
	m_impl->ResizeComponents(index + 1);
	if (m_impl->componentStorages[index] != nullptr) {
		return false;
	}
	m_impl->componentStorages[index] = storage;
	return true;
}

bool ECSHost::m_registerComponentStorage(const std::string& id, IComponentStorage* storage, void(*deleter)(IComponentStorage*)) {
	auto sharedStorage = std::shared_ptr<IComponentStorage>(storage, deleter);
	return m_registerComponentStorage(id, sharedStorage);
}

IComponentStorage* ECSHost::m_getComponentStorage(const std::string& id) {
	auto index = m_impl->idGenerator.GetId(id);
	if (m_impl->locked && !m_impl->unlocked.contains(index)) return nullptr;
	if (index >= m_impl->componentStorages.size()) {
		return nullptr;
	}
	return m_impl->componentStorages[index].get();
}
