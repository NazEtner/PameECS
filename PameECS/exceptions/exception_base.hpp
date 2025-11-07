#pragma once
#include <stdexcept>
#include <stacktrace>

namespace Pame::Exceptions {
	class ExceptionBase : public std::exception {
	public:
		explicit ExceptionBase(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: std::exception(message), m_stack_trace(stackTrace) {}

		explicit ExceptionBase(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: std::exception(message.c_str()), m_stack_trace(stackTrace) {}

		virtual ~ExceptionBase() = default;

		const std::stacktrace& GetTrace() const {
			return m_stack_trace;
		}

		virtual const char* GetExceptionTypeName() const = 0;
	private:
		std::stacktrace m_stack_trace;
	};
}
