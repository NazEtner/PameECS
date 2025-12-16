#pragma once
#include "../macros/dll.hpp"
#include "../macros/assertion.hpp"
#include <cstdint>
#include <type_traits>

// C ABIを使ってコンパイラ情報を取得できるようにする
// このヘッダー(check.hpp)と、ソースファイルのコピーをロードしたいDLLのプロジェクトに含めること
extern "C" {
	struct alignas(4) CompilerInfo {
		uint32_t size;
		enum class CompilerType : uint32_t {
			Unknown = 0,
			MSVC,
			GCC,
			Clang,
			Max,
		} type;
		uint32_t compilerVersion;
		// ここからはboolにキャストするべき値
		uint8_t useDynamicCRT;
		uint8_t isDebug;
		uint8_t is64Bit;
		// ここまで
		uint8_t padding;
		uint32_t iteratorDebugLevel;
		uint32_t cppExceptions;
		uint32_t reserved[2];
	};

	// ロードしたいDLLと、自身の両方で存在できるようにするためにEXPORT_ONLY
	PECS_DLL_EXPORT_ONLY void GetCompilerInfo(CompilerInfo* infoOut);
	PECS_DLL_EXPORT_ONLY bool IsSameCompilerABI(const CompilerInfo* infoA, const CompilerInfo* infoB);
}

namespace PameECS::ABI::Check::Internal::Assertions {
	constexpr bool IsSameCompilerABIImpl() {
		static_assert(sizeof(CompilerInfo) == 32, "CompilerInfo struct size mismatch.");
		static_assert(alignof(CompilerInfo) == 4, "CompilerInfo struct alignment mismatch.");
		static_assert(std::is_trivially_copyable_v<CompilerInfo>);
		static_assert(std::is_standard_layout_v<CompilerInfo>);
		// 戻り値に意味はない
		return true;
	}

	GLOBAL_CHECK_NON_ARG(IsSameCompilerABIImpl)
}
