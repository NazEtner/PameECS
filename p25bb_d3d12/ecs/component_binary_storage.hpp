#pragma once
#include "types.hpp"
#include <vector>

namespace PameECS::ECS {
	class ComponentBinaryStorage {
	public:
		ComponentBinaryStorage(size_t componentSize) : m_component_size(componentSize) {}

		std::byte* AddComponentData(const Types::Entity& entity) {
			size_t index = entity.id;
			if (index >= m_generations.size()) {
				while (index >= m_generations.size()) {
					m_reserve();
				}
			}
			m_generations[index] = entity.generation;
			return &m_storage[index * m_component_size];
		}

		std::byte* GetComponentData(Types::Entity entity) {
			size_t index = entity.id;
			if (index >= m_generations.size() || m_generations[index] != entity.generation) {
				return nullptr;
			}
			return &m_storage[index * m_component_size];
		}
	private:
		void m_reserve() {
			size_t currentCapacity = m_storage.size() / m_component_size;
			size_t newCapacity = currentCapacity == 0 ? 1 : currentCapacity * 2;
			m_storage.resize(newCapacity * m_component_size);
			m_generations.resize(newCapacity, 0xFFFFFFFF'FFFFFFFF);
		}
		size_t m_component_size;
		std::vector<std::byte> m_storage;
		std::vector<uint64_t> m_generations;
	};
}
