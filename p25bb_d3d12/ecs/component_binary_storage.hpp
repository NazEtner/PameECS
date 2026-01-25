#pragma once
#include "types.hpp"
#include "../macros/debug.hpp"
#include <vector>
#include <cstddef>
#include <utility>
#include <cassert>
#include <cstring>

namespace PameECS::ECS {
	class ComponentBinaryStorage {
	public:
		ComponentBinaryStorage(size_t componentSize, size_t align) : m_component_size(componentSize), m_align(align) {}

		std::byte* AddComponentData(const Types::Entity& entity) {
			size_t index = entity.id;
			if (index >= m_generations.size()) {
				while (index >= m_generations.size()) {
					m_reserve();
				}
			}
			m_generations[index] = entity.generation;
			return &m_storage[index * m_component_size + m_offset];
		}

		std::byte* GetComponentData(Types::Entity entity) {
			size_t index = entity.id;
			if (index >= m_generations.size() || m_generations[index] != entity.generation) {
				return nullptr;
			}
			return &m_storage[index * m_component_size + m_offset];
		}
	private:
		void m_reserve() {
			size_t currentCapacity = m_storage.size() / m_component_size;
			size_t newCapacity = currentCapacity == 0 ? 1 : currentCapacity * 2;
			m_storage.resize((newCapacity + 1) * m_component_size);
			m_generations.resize(newCapacity, 0xFFFFFFFF'FFFFFFFF);
			auto oldOffset = m_offset;
			void* ptr = m_storage.data();
			size_t space = m_storage.size();
			if (std::align(m_align, m_component_size, ptr, space)) {
				std::byte* alignedPtr = static_cast<std::byte*>(ptr);

				m_offset = alignedPtr - m_storage.data();

				size_t dataSize = currentCapacity * m_component_size;
				if (dataSize > 0) {
					std::byte* src = m_storage.data() + oldOffset;
					std::byte* dst = alignedPtr;
					std::memmove(dst, src, dataSize);
				}
			}
			else {
#ifdef _DEBUG
				assert(false && "Alignment failed in ComponentBinaryStorage::m_reserve");
#else
				std::unreachable();
#endif
			}
		}
		const size_t m_align = 1;
		size_t m_offset = 0;
		size_t m_component_size;
		std::vector<std::byte> m_storage;
		std::vector<uint64_t> m_generations;
	};
}
