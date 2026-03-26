#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class ScriptErrorTag {
	public:
		static constexpr const char* typeName = "ScriptError";
	};

	using ScriptError = Pame::Exceptions::ExceptionOf<ScriptErrorTag>;
}
