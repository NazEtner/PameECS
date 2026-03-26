#pragma once
#include "types.hpp"
#include <type_traits>
#include <concepts>
#include <cstddef>

namespace PameECS::ECS::Concepts {
	// const Types::ComponentLayoutElement** elementsは必ずstaticなポインタを返すようにすること
	// countも同様にstaticなサイズを返すようにすること
	// const char** nameも同様
	template <typename T>
	concept ComponentType =
		std::is_trivially_copyable_v<T> &&
		std::is_trivially_destructible_v<T> &&
		std::is_default_constructible_v<T> &&
		std::is_class_v<T> &&
		requires(const Types::ComponentLayoutElement** elements, size_t* count) {
			{ T::GetComponentLayoutElements(elements, count) } -> std::same_as<void>;
		} &&
		requires(const char** name, size_t* count) {
			{ T::GetNameTag(name, count) } -> std::same_as<void>;
		};
}
