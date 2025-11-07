#pragma once
#include "exception_of.hpp"

namespace Pame::Exceptions {
	class DllLoadFailed : public ExceptionOf<DllLoadFailed> {
	public:
		static constexpr const char* typeName = "DllLoadFailed";
		explicit DllLoadFailed(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<DllLoadFailed>(message, stackTrace) {}
		explicit DllLoadFailed(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<DllLoadFailed>(message, stackTrace) {}
		virtual ~DllLoadFailed() = default;
	};
}
