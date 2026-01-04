#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class ComErrorTag {
	public:
		static constexpr const char* typeName = "ComError";
	};

	using ComError = Pame::Exceptions::ExceptionOf<ComErrorTag>;
}
