#pragma once
#include <exceptions/exception_of.hpp>

namespace PameECS::Exceptions {
	class FileErrorTag {
	public:
		static constexpr const char* typeName = "FileError";
	};

	using FileError = Pame::Exceptions::ExceptionOf<FileErrorTag>;
}
