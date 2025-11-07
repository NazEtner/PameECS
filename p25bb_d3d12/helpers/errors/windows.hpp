#pragma once
#include <string>
#include <windows.h>

namespace PameECS::Helpers::Errors::Windows {
	inline std::string GetLastErrorMessage() noexcept {
		DWORD error = GetLastError();
		void* messageBuffer;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&messageBuffer,
			0,
			NULL
		);

		auto ret = std::string(static_cast<char*>(messageBuffer));

		LocalFree(messageBuffer);

		ret.erase(std::remove_if(ret.begin(), ret.end(), [](char c) {
			return c == '\r' || c == '\n';
			}), ret.end());

		return ret;
	}
}
