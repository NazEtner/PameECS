#pragma once
#include <bit>

namespace PameECS::Helpers::Binary {
	template<typename T, std::endian Source, std::endian Native = std::endian::native>
	T ToNativeEndian(T value) {
		if constexpr (Source == Native) {
			return value;
		} else {
			return std::byteswap(value);
		}
	}
}
