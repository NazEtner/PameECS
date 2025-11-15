#pragma once
#include "exception_of.hpp"

namespace Pame::Exceptions {
	class InvalidStateTag {
	public:
		static constexpr const char* typeName = "InvalidState";
	};

	using InvalidState = Pame::Exceptions::ExceptionOf<InvalidStateTag>;
}
