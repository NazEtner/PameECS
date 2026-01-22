#pragma once
#include <string>
#include <string_view>

namespace PameECS::Hash {
	struct StringHash {
		using is_transparent = void;

		size_t operator()(std::string_view sv) const {
			return std::hash<std::string_view>{}(sv);
		}
		size_t operator()(const std::string& s) const {
			return std::hash<std::string>{}(s);
		}
	};
}
