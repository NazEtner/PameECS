#pragma once
#include "exception_of.hpp"

namespace Pame::Exceptions {
	class ConfigLoadFailedTag {
	public:
		static constexpr const char* typeName = "ConfigLoadFailed";
	};

	using ConfigLoadFailed = Pame::Exceptions::ExceptionOf<ConfigLoadFailedTag>;
}
