#pragma once
#include "exception_base.hpp"

namespace Pame::Exceptions {
	template<class T>
	class ExceptionOf : public ExceptionBase {
	public:
		explicit ExceptionOf(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionBase(message, stackTrace) { }

		explicit ExceptionOf(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionBase(message, stackTrace) { }

		virtual ~ExceptionOf() = default;

		const char* GetExceptionTypeName() const override {
			static_assert(std::is_convertible_v<decltype(T::typeName), const char*>,
				"T must define static constexpr const char* typeName");
			return T::typeName;
		}
	};
}
