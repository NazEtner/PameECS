#include "component_storage.hpp"
#include "../helpers/id_generator.hpp"
#include "../macros/dll.hpp"
#include <string>
#include <typeindex>
#include <vector>
#include <limits>

namespace PameECS::ECS {
	class PECS_DLL_SHARED ECSHost final {
	public:
		ECSHost();
		~ECSHost();
		ECSHost(const ECSHost&) = delete;
		ECSHost& operator=(const ECSHost&) = delete;

		bool NewEntity(Types::Entity& entity, const std::vector<std::string>& components,
			size_t idMin = std::numeric_limits<size_t>::min(),
			size_t idMax = std::numeric_limits<size_t>::max());

		bool RemoveEntity(const Types::Entity& entity);

		bool AddComponent(const Types::Entity& entity, const std::string& component);
		bool RemoveComponent(const Types::Entity& entity, const std::string& component);

		template <Concepts::ComponentType T>
		bool NewComponentStorage(const std::string& id) {
			auto storage = std::make_shared<ComponentStorage<T>>();
			
			return m_registerComponentStorage(id, storage);
		}

		// TのGetComponentLayoutElementsやGetNameTagを工夫すればハックできそう
		// やるとしても自己責任で
		template <Concepts::ComponentType T>
		std::shared_ptr<ComponentStorage<T>> GetComponentStorageAs(const std::string& id) {
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
			return std::static_pointer_cast<ComponentStorage<T>>(storageBase);
		}
	private:
		bool m_registerComponentStorage(const std::string& id, std::shared_ptr<IComponentStorage> storage);
		std::shared_ptr<IComponentStorage> m_getComponentStorage(const std::string& id);
		struct Impl;
		inline static Impl* m_impl = nullptr;
	};
}
