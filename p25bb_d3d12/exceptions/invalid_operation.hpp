#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class InvalidOperationTag {
	public:
		static constexpr const char* typeName = "InvalidOperation";
	};

	using InvalidOperation = Pame::Exceptions::ExceptionOf<InvalidOperationTag>;
}
