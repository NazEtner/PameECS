#pragma once
#include "types.hpp"
#include "component_binary_storage.hpp"

namespace PameECS::ECS {
	class IComponentStorage {
	public:
		IComponentStorage() = default;
		virtual ~IComponentStorage() = default;
		ComponentBinaryStorage& GetBinaryStorage() {
			return m_binary_storage;
		}
	protected:
		template<typename T>
		T* m_addComponentAs(const Types::Entity& entity) {
			static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
				"Component type must be trivially copyable and trivially destructiable.");
			std::byte* data = m_binary_storage.AddComponentData(entity);
			auto component = new (data) T();
			return component;
		}

		template<typename T>
		T* m_getComponentAs(const Types::Entity& entity) {
			static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
				"Component type must be trivially copyable and trivially destructiable.");
			std::byte* data = m_binary_storage.GetComponentData(entity);
			if (data == nullptr) {
				return nullptr;
			}
			return reinterpret_cast<T*>(data);
		}
		ComponentBinaryStorage m_binary_storage;
	};
}
