#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class RendererErrorTag {
	public:
		static constexpr const char* typeName = "RendererError";
	};

	using RendererError = Pame::Exceptions::ExceptionOf<RendererErrorTag>;
}
