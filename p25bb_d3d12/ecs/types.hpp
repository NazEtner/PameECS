#pragma once
#include <cstdint>
#include <string>

namespace PameECS::ECS::Types {
	struct Entity {
		uint64_t id;
		uint64_t generation;
	};

	struct ComponentLayoutElement {
		ComponentLayoutElement(
			const std::string& typeName,
			uint32_t typeSize,
			uint32_t typeOffset
		) {
			const size_t n = std::min(typeName.size(), sizeof(type));
			std::memcpy(type, typeName.data(), n);
			size = typeSize;
			offset = typeOffset;
		}
		// 型名(null終端なし)
		char type[24] = {};
		uint32_t size = 0;
		uint32_t offset = 0;
	};
}
