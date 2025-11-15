#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class InvalidArgumentTag {
	public:
		static constexpr const char* typeName = "InvalidArgument";
	};

	using InvalidArgument = Pame::Exceptions::ExceptionOf<InvalidArgumentTag>;
}
