#pragma once
#include "../macros/dll.hpp"
#include <string>
#include <memory>

namespace PameECS::ABI {
	class SharedString {
	public:
		SharedString();
		~SharedString();

		SharedString(const SharedString&) = delete;
		SharedString& operator=(const SharedString&) = delete;

		SharedString(SharedString&&) = delete;
		SharedString& operator=(SharedString&&) = delete;

		PECS_DLL_SHARED SharedString* Copy() const;
		PECS_DLL_SHARED const char* GetCStringData() const;
		PECS_DLL_SHARED size_t GetSize() const;

		PECS_DLL_SHARED void SetFromCString(const char* cstr, size_t size);

		std::string ToStdString() const {
			return std::string(GetCStringData(), GetSize());
		}

		void FromStdString(const std::string& str) {
			SetFromCString(str.data(), str.size());
		}
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" PECS_DLL_SHARED PameECS::ABI::SharedString* CreateSharedString();
extern "C" PECS_DLL_SHARED void DestroySharedString(PameECS::ABI::SharedString* str);
