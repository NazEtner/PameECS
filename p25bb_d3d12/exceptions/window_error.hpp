#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class WindowError : Pame::Exceptions::ExceptionOf<WindowError> {
	public:
		static constexpr const char* typeName = "WindowError";
		explicit WindowError(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<WindowError>(message, stackTrace) {}
		explicit WindowError(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<WindowError>(message, stackTrace) {}
		virtual ~WindowError() = default;
	};
}
