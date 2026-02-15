#pragma once
#include "component_storage_interface.hpp"
#include "concepts.hpp"
#include "../helpers/binary.hpp"

namespace PameECS::ECS {
	template<Concepts::ComponentType T>
	class ComponentStorage : public IComponentStorage {
	public:
		ComponentStorage() : IComponentStorage(sizeof(T), alignof(T)) {
			T::GetComponentLayoutElements(&m_layout_ptr, &m_layout_count);
			T::GetNameTag(&m_type_name, &m_type_name_size);
		}
		virtual ~ComponentStorage() = default;

		bool IsSameLayout(const Types::ComponentLayoutElement* layoutPtr, size_t count) const override {
			return Helpers::Binary::IsSameMemory(
				m_layout_ptr, sizeof(Types::ComponentLayoutElement) * m_layout_count,
				layoutPtr, sizeof(Types::ComponentLayoutElement) * count);
		}

		bool IsSameNameTag(const char* typeName, size_t typeNameSize) const override {
			return Helpers::Binary::IsSameMemory(
				m_type_name, m_type_name_size,
				typeName, typeNameSize);
		}

		bool MaybeSafeToCast(const Types::ComponentLayoutElement* layoutPtr, size_t count,
			const char* typeName, size_t typeNameSize, size_t componentSize) const override {
			return IsSameLayout(layoutPtr, count) && IsSameNameTag(typeName, typeNameSize) && sizeof(T) == componentSize;
		}

		T* GetComponent(const Types::Entity& entity) {
			return m_getComponentAs<T>(entity);
		}

		T* GetComponentByRawIndex(size_t index) {
			return m_getComponentByRawIndexAs<T>(index);
		}

		bool AddComponent(const Types::Entity& entity) override {
			return m_addComponentAs<T>(entity);
		}
	private:
		const Types::ComponentLayoutElement* m_layout_ptr = nullptr;
		size_t m_layout_count = 0;
		const char* m_type_name = nullptr;
		size_t m_type_name_size = 0;
	};
}
