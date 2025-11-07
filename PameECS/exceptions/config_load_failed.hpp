#pragma once
#include "exception_of.hpp"

namespace Pame::Exceptions {
	class ConfigLoadFailed : public ExceptionOf<ConfigLoadFailed> {
	public:
		static constexpr const char* typeName = "ConfigLoadFailed";
		explicit ConfigLoadFailed(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<ConfigLoadFailed>(message, stackTrace) {}
		explicit ConfigLoadFailed(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<ConfigLoadFailed>(message, stackTrace) {}
		virtual ~ConfigLoadFailed() = default;
	};
}