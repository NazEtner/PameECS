#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class RendererError : Pame::Exceptions::ExceptionOf<RendererError> {
	public:
		static constexpr const char* typeName = "RendererError";
		explicit RendererError(const char* message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<RendererError>(message, stackTrace) {}
		explicit RendererError(const std::string& message, const std::stacktrace stackTrace = std::stacktrace::current())
			: ExceptionOf<RendererError>(message, stackTrace) {}
		virtual ~RendererError() = default;
	};
}
