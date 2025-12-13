#pragma once
#include <bit>
#include <cstring>

namespace PameECS::Helpers::Binary {
	template<typename T, std::endian Source, std::endian Native = std::endian::native>
	T ToNativeEndian(T value) {
		if constexpr (Source == Native) {
			return value;
		} else {
			return std::byteswap(value);
		}
	}

	inline bool IsSameMemory(const void* ptr1, size_t size1, const void* ptr2, size_t size2) noexcept {
		if (!ptr1 || !ptr2) {
			return false;
		}
		if (ptr1 == ptr2) {
			return true;
		}
		if (size1 != size2) {
			return false;
		}
		if (size1 == 0) {
			return true;
		}
		return std::memcmp(ptr1, ptr2, size1) == 0;
	}
}
