#pragma once
#include "component_storage_interface.hpp"

namespace PameECS::ECS {
	template<typename T>
	class ComponentStorage : public IComponentStorage {
	public:
		ComponentStorage() : IComponentStorage() {
			m_binary_storage = ComponentBinaryStorage(sizeof(T));
		}
		virtual ~ComponentStorage() = default;

		T* AddComponent(const Types::Entity& entity) {
			return m_addComponentAs<T>(entity);
		}
		T* GetComponent(const Types::Entity& entity) {
			return m_getComponentAs<T>(entity);
		}
	};
}
