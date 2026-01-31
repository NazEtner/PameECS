#include "ecs_host.hpp"
#include "scheduler.hpp"
#include "../helpers/id_generator.hpp"
#include "../helpers/container.hpp"
#include <mutex>

using PameECS::ECS::ECSHost;
using PameECS::ECS::IComponentStorage;

extern "C" {
	PECS_DLL_SHARED size_t ECSGetComponentStorageId(const PameECS::ECS::ECSHost* ecsHost, const char* component) {
		return ecsHost->GetComponentStorageId(component);
	}
	PECS_DLL_SHARED bool ECSNewEntity(PameECS::ECS::ECSHost* ecsHost,
		PameECS::ECS::Types::Entity& entity, const char** components, const size_t elementCount,
		size_t idMin,
		size_t idMax) {
		return ecsHost->NewEntity(
			entity, components, elementCount, idMin, idMax
		);
	}
	PECS_DLL_SHARED bool ECSRemoveEntity(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity& entity) {
		return ecsHost->RemoveEntity(
			entity
		);
	}
	PECS_DLL_SHARED bool ECSAddComponent(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity& entity, const char* component) {
		return ecsHost->AddComponent(entity, component);
	}
	PECS_DLL_SHARED bool ECSRemoveComponent(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity& entity, const char* component) {
		return ecsHost->RemoveComponent(entity, component);
	}
	PECS_DLL_SHARED void ECSAddSyncTask(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::SyncTask& task) {
		ecsHost->AddSyncTask(task);
	}
	PECS_DLL_SHARED bool ECSRegisterComponentStorage(PameECS::ECS::ECSHost* ecsHost, const char* id, PameECS::ECS::IComponentStorage* storage, void(*deleter)(PameECS::ECS::IComponentStorage*)) {
		return ecsHost->RegisterComponentStorage(id, storage, deleter);
	}
	PECS_DLL_SHARED PameECS::ECS::IComponentStorage* ECSGetComponentStorage(const PameECS::ECS::ECSHost* ecsHost, const size_t id) {
		return ecsHost->GetComponentStorage(id);
	}
	PECS_DLL_SHARED size_t ECSAddSystem(PameECS::ECS::ECSHost* ecsHost, PameECS::ECS::System::Base* system, void(*deleter)(PameECS::ECS::System::Base*)) {
		return ecsHost->AddSystem(system, deleter);
	}
}

struct ECSHost::Impl {
	Impl(std::shared_ptr<BS::thread_pool<0U>> threadPool) : scheduler(threadPool) {

	}
	struct Tag {};
	Helpers::IdGenerator<true, true, Tag, 0> idGenerator;

	std::vector<std::shared_ptr<IComponentStorage>> componentStorages;
	std::vector<uint64_t> entityGenerations;
	// Copyはできるだけ安全にポインタを扱えるようにするためのもの
	std::vector<uint64_t> entityGenerationsCopy;
	bool isEntityGenerationsDirty = true;
	// boolに(から)キャストすべき
	// .data()が安全に使えないのでその回避のためにuint8_t
	std::vector<uint8_t> entityAliveFlags;
	std::vector<uint8_t> entityAliveFlagsCopy;
	bool isAliveFlagsDirty = true;
	// entityAliveFlags.size() >= lastEntityId
	// && entityGenerations.size() == entityAliveFlags.size()なはず
	size_t lastEntityId = 0;
	bool locked = false;
	Scheduler scheduler;
	std::unordered_set<size_t> unlocked;
	std::vector<std::shared_ptr<System::Base>> systems;
	std::vector<SyncTask> syncTasks;
	std::mutex mutex;
	std::shared_ptr<Services::Services> services;

	void ResizeComponents(size_t minSize) {
		// 少なくともminSize以上の容量を確保する
		m_resize(minSize, componentStorages);
	}

	void ResizeEntities(size_t minSize) {
		isAliveFlagsDirty = true;
		isEntityGenerationsDirty = true;
		m_resize(minSize, entityGenerations, std::numeric_limits<uint64_t>::max());
		m_resize(minSize, entityAliveFlags);
	}
private:
	template<typename T>
	void m_resize(size_t id, std::vector<T>& vec, T init = T()) {
		Helpers::Container::ResizePow2(vec, id, init);
	}
};

ECSHost::ECSHost(std::shared_ptr<BS::thread_pool<0U>> threadPool) {
	m_impl = std::make_unique<Impl>(threadPool);
}

ECSHost::~ECSHost() {}

void ECSHost::OpenDebugWindow(std::shared_ptr<DebugTools::DebugGUIHost> debugGUI) {
	debugGUI->AddWindow(
		"ECSHost",
		[this]() -> void {
			if (ImGui::CollapsingHeader("ECS Information")) {
				ImGui::Text("Component storages: %llu", m_impl->componentStorages.size());
				ImGui::Text("Entity generations: (%llu, %p)", m_impl->entityGenerations.size(), m_impl->entityGenerations.data());
				ImGui::Text("Entity alive flags: (%llu, %p)", m_impl->entityAliveFlags.size(), m_impl->entityAliveFlags.data());
				if (ImGui::TreeNode("Entities")) {
					static bool hideInactive = true;
					ImGui::Checkbox("Hide inactive", &hideInactive);
					size_t maxIndex = std::min(m_impl->entityGenerations.size(), m_impl->entityAliveFlags.size());
					if (hideInactive) {
						maxIndex = std::min(m_impl->lastEntityId + 1, maxIndex);
					}
					for (size_t i = 0; i < maxIndex; ++i) {
						if (hideInactive && !m_impl->entityAliveFlags[i]) continue;
						if (ImGui::TreeNode(std::format("{} (Generation: {:x})", i, m_impl->entityGenerations[i]).c_str())) {
							if (m_impl->entityAliveFlags[i]) {
								if (ImGui::Button(std::format("Remove##{}", i).c_str())) {
									Types::Entity entity = { i, m_impl->entityGenerations[i] };
									RemoveEntity(entity);
								}
							}

							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}
				ImGui::Text("Last entity id    : %llu", m_impl->lastEntityId);
				ImGui::Text("Locked            : %s", m_impl->locked ? "true" : "false");
				ImGui::Text("Systems           : %llu", m_impl->systems.size());
			}
			if (ImGui::CollapsingHeader("ECS Control")) {
				if (ImGui::Button("Add test entity")) {
					Types::Entity entity = {};
					NewEntity(entity, {});
				}
			}
		},
		{ 0.f, 0.f },
		{ 480.f, 560.f },
		false
	);
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

void ECSHost::Update() {
	for (const auto& system : m_impl->systems) {
		if (!system) continue;
		// Registerは「次に実行するSchedule()」で使うSystemを登録するもの
		m_impl->scheduler.Register(system.get());
	}

	System::Context context = {};
	context.ecsHost = this;
	context.services = m_impl->services.get();
	GetEntityAliveFlags(context.entityAliveFlags, context.entityAliveFlagsCount);
	GetEntityGenerations(context.entityGenerations, context.entityGenerationsCount);
	m_impl->scheduler.Schedule(&context);

	assert(!m_impl->locked);
	for (auto& tasks : m_impl->syncTasks) {
		tasks.Execute(&context);
	}
}

size_t ECSHost::GetComponentStorageId(const char* name) const {
	return m_impl->idGenerator.GetId(name);
}

bool ECSHost::NewEntity(Types::Entity& entity, const char** components, const size_t elementCount,
	size_t idMin,
	size_t idMax) {
	return m_newEntity(entity, std::span<const char*>{components, elementCount}, idMin, idMax);
}

bool ECSHost::RemoveEntity(const Types::Entity& entity) {
	if (m_impl->locked) return false;
	auto id = entity.id;
	if (id >= m_impl->entityAliveFlags.size() || !m_impl->entityAliveFlags[id]) {
		return false;
	}
	if (entity.generation != m_impl->entityGenerations[id]) {
		return false;
	}
	m_impl->isAliveFlagsDirty = true;
	m_impl->entityAliveFlags[id] = false;
	return true;
}

bool ECSHost::AddComponent(const Types::Entity& entity, const char* component) {
	if (m_impl->locked) return false;
	auto storage = GetComponentStorage(component);
	if (storage == nullptr) {
		return false;
	}
	return storage->AddComponent(entity);
}

bool ECSHost::RemoveComponent(const Types::Entity& entity, const char* component) {
	if (m_impl->locked) return false;
	auto storage = GetComponentStorage(component);
	if (storage == nullptr) {
		return false;
	}
	Types::Entity dummyEntity = { entity.id, 0xFFFFFFFF'FFFFFFFF };
	return storage->AddComponent(dummyEntity);
}

bool ECSHost::RegisterComponentStorage(const std::string& id, std::shared_ptr<IComponentStorage> storage) {
	if (m_impl->locked) return false;
	auto index = m_impl->idGenerator.GetId(id);
	m_impl->ResizeComponents(index + 1);
	if (m_impl->componentStorages[index] != nullptr) {
		return false;
	}
	m_impl->componentStorages[index] = storage;
	return true;
}

bool ECSHost::RegisterComponentStorage(const char* id, IComponentStorage* storage, void(*deleter)(IComponentStorage*)) {
	auto sharedStorage = std::shared_ptr<IComponentStorage>(storage, deleter);
	return RegisterComponentStorage(id, sharedStorage);
}

IComponentStorage* ECSHost::GetComponentStorage(const std::string_view& id) const {
	auto index = m_impl->idGenerator.GetId(id.data());
	return GetComponentStorage(index);
}

IComponentStorage* ECSHost::GetComponentStorage(const char* id) const {
	return GetComponentStorage(std::string_view(id));
}

IComponentStorage* ECSHost::GetComponentStorage(const size_t id) const {
	if (m_impl->locked && !m_impl->unlocked.contains(id)) return nullptr;
	if (id >= m_impl->componentStorages.size()) {
		return nullptr;
	}
	return m_impl->componentStorages[id].get();
}

size_t ECSHost::AddSystem(System::Base* system, void(*deleter)(System::Base*)) {
	if (m_impl->locked) return false;
	if (!system) return std::numeric_limits<size_t>::max();
	auto ret = m_impl->systems.size();
	auto uniqueSystem = std::shared_ptr<System::Base>(system, deleter);
	m_impl->systems.emplace_back(std::move(uniqueSystem));

	return ret;
}

bool ECSHost::m_newEntity(Types::Entity& entity, const std::span<const char*>& components,
	size_t idMin, size_t idMax) {
	if (m_impl->locked) return false;
	auto id = idMin;
	while (id <= idMax) {
		m_impl->ResizeEntities(id + 1);
		if (!m_impl->entityAliveFlags[id]) {
			entity.id = id;
			m_impl->isEntityGenerationsDirty = true;
			entity.generation = ++m_impl->entityGenerations[id];
			m_impl->isAliveFlagsDirty = true;
			m_impl->entityAliveFlags[id] = true;
			m_impl->lastEntityId = std::max(m_impl->lastEntityId, id);
			// コンポーネントの追加
			for (const auto& compId : components) {
				auto storage = GetComponentStorage(compId);
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

void ECSHost::AddSyncTask(const SyncTask& task) {
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->syncTasks.push_back(task);
}

void ECSHost::GetEntityAliveFlags(const uint8_t*& flags, size_t& count) {
	if (m_impl->isAliveFlagsDirty) {
		// ほぼゼロコストは多分無理がある
		m_impl->entityAliveFlagsCopy = m_impl->entityAliveFlags;
		m_impl->isAliveFlagsDirty = false;
	}
	flags = m_impl->entityAliveFlagsCopy.data();
	count = std::min(m_impl->lastEntityId + 1, m_impl->entityAliveFlagsCopy.size());
}

void ECSHost::GetEntityGenerations(const uint64_t*& generations, size_t& count) {
	if (m_impl->isEntityGenerationsDirty) {
		m_impl->entityGenerationsCopy = m_impl->entityGenerations;
		m_impl->isEntityGenerationsDirty = false;
	}

	generations = m_impl->entityGenerationsCopy.data();
	count = std::min(m_impl->lastEntityId + 1, m_impl->entityGenerationsCopy.size());
}
