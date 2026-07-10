#pragma once
#include "component_storage.hpp"
#include "../helpers/id_generator.hpp"
#include "../macros/dll.hpp"
#include "../debug_tools/debug_gui_host.hpp"
#include "system.hpp"
#include "sync_task.hpp"
#include "../services/services.hpp"
#include <string>
#include <typeindex>
#include <vector>
#include <limits>
#include <unordered_set>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <span>
#include <memory>

namespace PameECS::ECS {
	class ECSHost;
}

extern "C" {
	PECS_DLL_SHARED size_t ECSGetComponentStorageId(const PameECS::ECS::ECSHost* ecsHost, const char* component);
	PECS_DLL_SHARED bool ECSNewEntity(PameECS::ECS::ECSHost* ecsHost,
		PameECS::ECS::Types::Entity* entity, const char** components, const size_t elementCount,
		size_t idMin = 0,
		size_t idMax = std::numeric_limits<size_t>::max());
	PECS_DLL_SHARED bool ECSRemoveEntity(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity* entity);
	PECS_DLL_SHARED void ECSRemoveEntityRange(PameECS::ECS::ECSHost* ecsHost, size_t idMin, size_t idMax);
	PECS_DLL_SHARED bool ECSAddComponent(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity* entity, const char* component);
	//PECS_DLL_SHARED bool ECSRemoveComponent(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity* entity, const char* component);
	PECS_DLL_SHARED bool ECSRemoveComponent(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::Types::Entity* entity, size_t id);
	PECS_DLL_SHARED void ECSAddSyncTask(PameECS::ECS::ECSHost* ecsHost, const PameECS::ECS::SyncTask* task);
	PECS_DLL_SHARED bool ECSRegisterComponentStorage(PameECS::ECS::ECSHost* ecsHost, const char* id, PameECS::ECS::IComponentStorage* storage, void(*deleter)(PameECS::ECS::IComponentStorage*));
	PECS_DLL_SHARED PameECS::ECS::IComponentStorage* ECSGetComponentStorage(const PameECS::ECS::ECSHost* ecsHost, const size_t id);
	PECS_DLL_SHARED size_t ECSAddSystem(PameECS::ECS::ECSHost* ecsHost, PameECS::ECS::System::Base* system, void(*deleter)(PameECS::ECS::System::Base*));
	PECS_DLL_SHARED PameECS::Services::Services* ECSGetServices(const PameECS::ECS::ECSHost* ecsHost);
	PECS_DLL_SHARED void ECSStop(PameECS::ECS::ECSHost* ecsHost);
}

namespace PameECS::ECS {
	class ECSHost final {
	public:
		ECSHost(std::shared_ptr<BS::thread_pool<0U>> threadPool, std::shared_ptr<Services::Services> services);
		~ECSHost();
		ECSHost(const ECSHost&) = delete;
		ECSHost& operator=(const ECSHost&) = delete;
		ECSHost(ECSHost&&) = delete;
		ECSHost& operator=(ECSHost&&) = delete;

		void OpenDebugWindow(std::shared_ptr<DebugTools::DebugGUIHost> debugGUI);

		size_t GetComponentStorageId(const std::string& component) const {
			return ECSGetComponentStorageId(this, component.c_str());
		}
		size_t GetComponentStorageId(const char* component) const;

		bool NewEntity(Types::Entity& entity, const std::vector<std::string>& components,
			size_t idMin = 0, // 実際のEntity::idはサイズの都合上uint64_tだけど、実質的にはインデックスなのでsize_tにしている
			size_t idMax = std::numeric_limits<size_t>::max()) {
			std::vector<const char*> cComponents;
			cComponents.reserve(components.size());

			for (const auto& component : components) {
				cComponents.emplace_back(component.c_str());
			}

			return ECSNewEntity(this, &entity, cComponents.data(), cComponents.size(), idMin, idMax);
		}

		bool NewEntity(Types::Entity& entity, const char** components, const size_t elementCount,
			size_t idMin = 0,
			size_t idMax = std::numeric_limits<size_t>::max());

		bool RemoveEntity(const Types::Entity& entity);
		void RemoveEntityRange(size_t idMin, size_t idMax);

		bool AddComponent(const Types::Entity& entity, const std::string& component) {
			return ECSAddComponent(this, &entity, component.c_str());
		}

		bool AddComponent(const Types::Entity& entity, const char* component);

		bool RemoveComponent(const Types::Entity& entity, const std::string& component) {
			// return ECSRemoveComponent(this, &entity, component.c_str());
			return ECSRemoveComponent(this, &entity, GetComponentStorageId(component));
		}

		bool RemoveComponent(const Types::Entity& entity, const char* component) {
			return ECSRemoveComponent(this, &entity, ECSGetComponentStorageId(this, component));
		}

		bool RemoveComponent(const Types::Entity& entity, size_t id);

		// 内部用のAPI
		void LockAll();
		void UnlockAll();
		void Unlock(const std::unordered_set<size_t>& ids);

		void Update();

		template <Concepts::ComponentType T>
		bool NewComponentStorage(const std::string& id) {
			// shared_ptrがDLL境界をまたがないようにするため
			// 呼び出し側ABIでの作成と削除を強制する
			auto storage = new ComponentStorage<T>();
			auto deleter = [](IComponentStorage* ptr) -> void {
				delete ptr;
			};
			
			return ECSRegisterComponentStorage(this, id.c_str(), storage, deleter);
		}

		// 戻り値はインデックス
		// 一度追加したSystemを削除することはできない
		template<typename T, typename ...Args>
			requires std::derived_from<T, System::Base>
		size_t AddSystem(Args&&... args) {
			auto system = new (std::nothrow) T(std::forward<Args>(args)...);
			auto deleter = [](System::Base* system) -> void {
				delete system;
			};

			if (!system) return std::numeric_limits<size_t>::max();

			return ECSAddSystem(this, system, deleter);
		}

		// TのGetComponentLayoutElementsやGetNameTagを工夫すればハックできそう
		// やるとしても自己責任で
		template <Concepts::ComponentType T>
		ComponentStorage<T>* GetComponentStorageAs(const size_t index) const {
			auto storageBase = ECSGetComponentStorage(this, index);
			if (storageBase == nullptr) [[unlikely]] {
				return nullptr;
			}
			const Types::ComponentLayoutElement* layoutPtr = nullptr;
			size_t layoutCount = 0;
			T::GetComponentLayoutElements(&layoutPtr, &layoutCount);
			const char* typeName = nullptr;
			size_t typeNameSize = 0;
			T::GetNameTag(&typeName, &typeNameSize);
			if (!storageBase->MaybeSafeToCast(
				layoutPtr, layoutCount,
				typeName, typeNameSize,
				sizeof(T))) [[unlikely]] {
				return nullptr;
			}
			return static_cast<ComponentStorage<T>*>(storageBase);
		}

		template <Concepts::ComponentType T>
		ComponentStorage<T>* GetComponentStorageAs(const std::string& id) const {
			auto index = GetComponentStorageId(id);
			return GetComponentStorageAs<T>(index);
		}

		void AddSyncTask(const SyncTask& task);

		void GetEntityAliveFlags(const uint8_t*& flags, size_t& count);
		void GetEntityGenerations(const uint64_t*& generations, size_t& count);

		bool RegisterComponentStorage(const std::string& id, std::shared_ptr<IComponentStorage> storage);
		bool RegisterComponentStorage(const char* id, IComponentStorage* storage, void(*deleter)(IComponentStorage*));
		IComponentStorage* GetComponentStorage(const std::string_view& id) const;
		IComponentStorage* GetComponentStorage(const char* id) const;
		IComponentStorage* GetComponentStorage(const size_t id) const;
		size_t AddSystem(System::Base* system, void(*deleter)(System::Base*));
		Services::Services* GetServices() const;

		void Stop();

		bool Stopped() const;
	private:
		bool m_newEntity(Types::Entity& entity, const std::span<const char*>& components, size_t idMin, size_t idMax);
		struct Impl;
		std::unique_ptr<Impl> m_impl = nullptr;
	};
}
