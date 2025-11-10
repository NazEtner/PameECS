#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class InvalidArgument : Pame::Exceptions::ExceptionOf<InvalidArgument> {
	public:
		static constexpr const char* typeName = "InvalidArgument";
		explicit InvalidArgument(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<InvalidArgument>(message, stackTrace) {}
		explicit InvalidArgument(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<InvalidArgument>(message, stackTrace) {}
		virtual ~InvalidArgument() = default;
	};
}
