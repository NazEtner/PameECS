#pragma once
#include "../macros/assertion.hpp"

namespace PameECS::TemplateTypes {
	template <size_t N>
	struct StringLiteral {
		char data[N];
		// コンパイル時に文字列をコピー
		constexpr StringLiteral(const char(&str)[N]) {
			std::copy_n(str, N, data);
		}
	};
}
