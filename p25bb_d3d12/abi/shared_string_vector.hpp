#pragma once
#include "shared_string.hpp"
#include "../macros/dll.hpp"
#include <vector>
#include <memory>

namespace PameECS::ABI {
	class SharedStringVector {
	public:
		SharedStringVector();
		~SharedStringVector();

		SharedStringVector(const SharedStringVector&) = delete;
		SharedStringVector& operator=(const SharedStringVector&) = delete;

		SharedStringVector(SharedStringVector&&) = delete;
		SharedStringVector& operator=(SharedStringVector&&) = delete;

		PECS_DLL_SHARED size_t GetSize() const;
		PECS_DLL_SHARED PameECS::ABI::SharedString* GetAt(size_t index) const;
		PECS_DLL_SHARED void PushBack(const PameECS::ABI::SharedString* str);
		PECS_DLL_SHARED void Clear();

		std::vector<std::string> ToStdStringVector() const {
			std::vector<std::string> result;
			auto size = GetSize();
			result.reserve(size);
			for (size_t i = 0; i < size; i++) {
				auto str = GetAt(i);
				result.emplace_back(str->GetCStringData(), str->GetSize());
			}
			return result;
		}

		void FromStdStringVector(const std::vector<std::string>& vec) {
			Clear();
			for (const auto& str : vec) {
				auto sharedStr = CreateSharedString();
				sharedStr->FromStdString(str);
				PushBack(sharedStr);
			}
		}

		void PushBack(const std::string& str) {
			auto sharedStr = CreateSharedString();
			sharedStr->FromStdString(str);
			PushBack(sharedStr);
		}
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" PECS_DLL_SHARED PameECS::ABI::SharedStringVector* CreateSharedStringVector();
extern "C" PECS_DLL_SHARED void DestroySharedStringVector(PameECS::ABI::SharedStringVector* strVec);
