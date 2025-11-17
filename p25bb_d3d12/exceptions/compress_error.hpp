#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class CompressErrorTag {
	public:
		static constexpr const char* typeName = "CompressError";
	};

	using CompressError = Pame::Exceptions::ExceptionOf<CompressErrorTag>;
}
