#include "component_storage.hpp"
#include "../helpers/id_generator.hpp"
#include "../macros/dll.hpp"
#include "system.hpp"
#include <string>
#include <typeindex>
#include <vector>
#include <limits>
#include <unordered_set>
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <span>

namespace PameECS::ECS {
	class ECSHost final {
	public:
		ECSHost(std::shared_ptr<BS::thread_pool<0U>> threadPool);
		~ECSHost();
		ECSHost(const ECSHost&) = delete;
		ECSHost& operator=(const ECSHost&) = delete;
		ECSHost(ECSHost&&) = delete;
		ECSHost& operator=(ECSHost&&) = delete;

		size_t GetComponentStorageId(const std::string& component) {
			return GetComponentStorageId(component.c_str());
		}
		PECS_DLL_SHARED size_t GetComponentStorageId(const char* component);

		bool NewEntity(Types::Entity& entity, const std::vector<std::string>& components,
			size_t idMin = 0, // 実際のEntity::idはサイズの都合上uint64_tだけど、実質的にはインデックスなのでsize_tにしている
			size_t idMax = std::numeric_limits<size_t>::max()) {
			std::vector<const char*> cComponents;
			cComponents.reserve(components.size());

			for (const auto& component : components) {
				cComponents.emplace_back(component.c_str());
			}

			return NewEntity(entity, cComponents.data(), cComponents.size(), idMin, idMax);
		}

		PECS_DLL_SHARED bool NewEntity(Types::Entity& entity, const char** components, const size_t elementCount,
			size_t idMin = 0,
			size_t idMax = std::numeric_limits<size_t>::max());

		PECS_DLL_SHARED bool RemoveEntity(const Types::Entity& entity);

		bool AddComponent(const Types::Entity& entity, const std::string& component) {
			return AddComponent(entity, component.c_str());
		}

		PECS_DLL_SHARED bool AddComponent(const Types::Entity& entity, const char* component);

		bool RemoveComponent(const Types::Entity& entity, const std::string& component) {
			return RemoveComponent(entity, component.c_str());
		}

		PECS_DLL_SHARED bool RemoveComponent(const Types::Entity& entity, const char* component);

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
			
			return m_registerComponentStorage(id, storage, deleter);
		}

		// 戻り値はインデックス
		// 一度追加したSystemを削除することはできない
		template<typename T>
			requires std::derived_from<T, System::Base>
		size_t AddSystem() {
			auto system = new T();
			auto deleter = [](System::Base* system) -> void {
				delete system;
			};

			if (!system) return std::numeric_limits<size_t>::max();

			return m_addSystem(system, deleter);
		}

		// TのGetComponentLayoutElementsやGetNameTagを工夫すればハックできそう
		// やるとしても自己責任で
		template <Concepts::ComponentType T>
		ComponentStorage<T>* GetComponentStorageAs(const std::string& id) {
			auto storageBase = m_getComponentStorage(id);
			if (storageBase == nullptr) {
				return nullptr;
			}
			Types::ComponentLayoutElement* layoutPtr = nullptr;
			size_t layoutCount = 0;
			T::GetComponentLayoutElements(&layoutPtr, &layoutCount);
			const char* typeName = nullptr;
			size_t typeNameSize = 0;
			T::GetNameTag(&typeName, &typeNameSize);
			if (!storageBase->MaybeSafeToCast(
				layoutPtr, layoutCount,
				typeName, typeNameSize,
				sizeof(T))) {
				return nullptr;
			}
			return static_cast<ComponentStorage<T>>(storageBase);
		}
	private:
		bool m_registerComponentStorage(const std::string& id, std::shared_ptr<IComponentStorage> storage);
		PECS_DLL_SHARED bool m_registerComponentStorage(const std::string& id, IComponentStorage* storage, void(*deleter)(IComponentStorage*));
		IComponentStorage* m_getComponentStorage(const std::string_view& id);
		IComponentStorage* m_getComponentStorage(const char* id);
		PECS_DLL_SHARED size_t m_addSystem(System::Base* system, void(*deleter)(System::Base*));
		bool m_newEntity(Types::Entity& entity, const std::span<const char*>& components, size_t idMin, size_t idMax);
		struct Impl;
		Impl* m_impl = nullptr;
	};
}
