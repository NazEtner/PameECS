#pragma once
#include "exception_of.hpp"

namespace Pame::Exceptions {
	class InvalidState : public ExceptionOf<InvalidState> {
	public:
		static constexpr const char* typeName = "InvalidState";
		explicit InvalidState(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<InvalidState>(message, stackTrace) {}
		explicit InvalidState(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<InvalidState>(message, stackTrace) {}
		virtual ~InvalidState() = default;
	};
}
