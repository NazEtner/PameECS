#pragma once
#include "exception_of.hpp"

namespace Pame::Exceptions {
	class DllLoadFailedTag {
	public:
		static constexpr const char* typeName = "DllLoadFailed";
	};

	using DllLoadFailed = Pame::Exceptions::ExceptionOf<DllLoadFailedTag>;
}
