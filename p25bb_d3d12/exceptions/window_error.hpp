#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class WindowErrorTag {
	public:
		static constexpr const char* typeName = "WindowError";
	};

	using WindowError = Pame::Exceptions::ExceptionOf<WindowErrorTag>;
}
