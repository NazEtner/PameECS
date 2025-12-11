#pragma once
#include <cstdint>

namespace PameECS::ECS::Types {
	struct Entity {
		uint64_t id;
		uint64_t generation;
	};
}
