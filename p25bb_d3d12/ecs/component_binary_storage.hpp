#pragma once
#include "types.hpp"
#include "../macros/debug.hpp"
#include <vector>
#include <cstddef>
#include <utility>
#include <cassert>
#include <cstring>
#include <optional>

namespace PameECS::ECS {
	class ComponentBinaryStorage {
	public:
		ComponentBinaryStorage(size_t componentSize, size_t align) : m_component_size(componentSize), m_align(align) {}

		std::byte* AddComponentData(const Types::Entity& entity) {
			if (auto denseIndex = m_getDenseIndex(entity.id); denseIndex.has_value()) {
				if (m_generations[*denseIndex] == entity.generation) {
					return m_getAddressByDenseIndex(*denseIndex);
				}
			}

			size_t pageIndex = entity.id / PAGE_SIZE;
			size_t offsetIndex = entity.id % PAGE_SIZE;

			if (pageIndex >= m_sparse_pages.size()) {
				m_sparse_pages.resize(pageIndex + 1);
			}
			if (m_sparse_pages[pageIndex].empty()) {
				m_sparse_pages[pageIndex].resize(PAGE_SIZE, INVALID_DENSE_INDEX);
			}

			uint32_t newDenseIndex = static_cast<uint32_t>(m_dense_indexes.size());
			m_sparse_pages[pageIndex][offsetIndex] = newDenseIndex;

			m_dense_indexes.push_back(entity.id);
			m_generations.push_back(entity.generation);

			m_ensureStorageCapacity(m_dense_indexes.size());

			return m_getAddressByDenseIndex(newDenseIndex);
		}

		std::byte* GetComponentData(Types::Entity entity) {
			auto index = m_getDenseIndex(entity.id);
			if (!index.has_value() || m_generations[index.value()] != entity.generation) {
				return nullptr;
			}
			return m_getAddressByDenseIndex(index.value());
		}

		void RemoveComponent(Types::Entity entity) {
			auto denseIndex = m_getDenseIndex(entity.id);
			if (!denseIndex.has_value() || m_generations[denseIndex.value()] != entity.generation) {
				return;
			}

			uint32_t target = denseIndex.value();
			uint32_t last = static_cast<uint32_t>(m_dense_indexes.size() - 1);
			uint64_t lastEntityId = m_dense_indexes.back();

			if (target != last) {
				std::memcpy(m_getAddressByDenseIndex(target), m_getAddressByDenseIndex(last), m_component_size);

				m_generations[target] = m_generations[last];
				m_dense_indexes[target] = lastEntityId;

				m_sparse_pages[lastEntityId / PAGE_SIZE][lastEntityId % PAGE_SIZE] = target;
			}

			m_sparse_pages[entity.id / PAGE_SIZE][entity.id % PAGE_SIZE] = INVALID_DENSE_INDEX;
			m_dense_indexes.pop_back();
			m_generations.pop_back();
		}

		std::byte* GetComponentDataByRawIndex(size_t index) {
			return m_getAddressByDenseIndex(index);
		}

		size_t GetCount() const {
			return m_dense_indexes.size();
		}

		Types::Entity GetEntityByRawIndex(size_t index) {
			assert(index < m_dense_indexes.size());

			return Types::Entity{
				.id = m_dense_indexes[index],
				.generation = m_generations[index]
			};
		}
	private:
		static constexpr size_t PAGE_SIZE = 1024; // 4KB
		static constexpr size_t INVALID_DENSE_INDEX = 0xFFFFFFFF;

		std::optional<uint32_t> m_getDenseIndex(uint64_t entity_id) const {
			size_t page_idx = entity_id / PAGE_SIZE;
			if (page_idx >= m_sparse_pages.size() || m_sparse_pages[page_idx].empty()) {
				return std::nullopt;
			}
			uint32_t idx = m_sparse_pages[page_idx][entity_id % PAGE_SIZE];
			return (idx == INVALID_DENSE_INDEX) ? std::nullopt : std::make_optional(idx);
		}

		std::byte* m_getAddressByDenseIndex(size_t dense_idx) {
			return &m_storage[dense_idx * m_component_size + m_offset];
		}

		void m_ensureStorageCapacity(size_t required_count) {
			size_t current_cap = (m_storage.size() > m_offset) ? (m_storage.size() - m_offset) / m_component_size : 0;
			if (required_count > current_cap) {
				m_reserve();
			}
		}

		void m_reserve() {
			size_t currentCapacity = m_storage.size() / m_component_size;
			size_t newCapacity = currentCapacity == 0 ? 1 : currentCapacity * 2;
			m_storage.resize((newCapacity + 1) * m_component_size);
			// m_generations.resize(newCapacity, 0xFFFFFFFF'FFFFFFFF);
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

		std::vector<std::vector<uint32_t>> m_sparse_pages;
		std::vector<std::byte> m_storage;
		std::vector<uint64_t> m_dense_indexes;
		std::vector<uint64_t> m_generations;
	};
}
