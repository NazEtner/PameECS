#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class AudioErrorTag {
	public:
		static constexpr const char* typeName = "AudioError";
	};

	using AudioError = Pame::Exceptions::ExceptionOf<AudioErrorTag>;
}
