#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class InvalidComponentTypeTag {
	public:
		static constexpr const char* typeName = "InvalidComponentType";
	};

	using InvalidComponentType = Pame::Exceptions::ExceptionOf<InvalidComponentTypeTag>;
}
