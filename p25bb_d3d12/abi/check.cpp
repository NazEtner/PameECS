#include "check.hpp"
#include "../macros/debug.hpp"
#include <cstring>

extern "C" PECS_DLL_EXPORT_ONLY void GetCompilerInfo(CompilerInfo* infoOut) {
	if (!infoOut) return;
	std::memset(infoOut, 0xff, sizeof(CompilerInfo));
	infoOut->size = sizeof(CompilerInfo);
#if defined(_MSC_VER)
	infoOut->type = CompilerInfo::CompilerType::MSVC;
	infoOut->compilerVersion = _MSC_FULL_VER;

#if defined(_DLL)
	infoOut->useDynamicCRT = true;
#else
	infoOut->useDynamicCRT = false;
#endif

#if defined(_DEBUG)
	infoOut->isDebug = true;
#else
	infoOut->isDebug = false;
#endif

#if defined(_WIN64)
	infoOut->is64Bit = true;
#else
	infoOut->is64Bit = false;
#endif

#if defined(_ITERATOR_DEBUG_LEVEL)
	infoOut->iteratorDebugLevel = _ITERATOR_DEBUG_LEVEL;
#else
	infoOut->iteratorDebugLevel = 0;
#endif

#if defined(__cpp_exceptions)
	infoOut->cppExceptions = __cpp_exceptions;
#else
	infoOut->cppExceptions = 0;
#endif

#if defined(__cplusplus)
	infoOut->cpp = _MSVC_LANG;
#else
	infoOut->cpp = 0;
#endif
	return;
	// end of MSVC
#elif defined(__GNUC__)
#if defined(__clang__)
	infoOut->type = CompilerInfo::CompilerType::Clang;
	infoOut->compilerVersion = __clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__;
#else
	// GCC
	infoOut->type = CompilerInfo::CompilerType::GCC;
	infoOut->compilerVersion = __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__;
#endif
	infoOut->useDynamicCRT = true; // GCCは基本的に動的CRTを使う(静的CRTの場合は未定義ということで)
#if defined(_DEBUG) // マクロがあるので使えるはず
	infoOut->isDebug = true;
#else
	infoOut->isDebug = false;
#endif
#if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__) || defined(_LP64)
	infoOut->is64Bit = true;
#else
	infoOut->is64Bit = false;
#endif
	infoOut->iteratorDebugLevel = 0; // 判定不可

#if defined(__cpp_exceptions)
	infoOut->cppExceptions = __cpp_exceptions;
#else
	infoOut->cppExceptions = 0;
#endif

#if defined(__cplusplus)
	infoOut->cpp = __cplusplus;
#else
	infoOut->cpp = 0;
#endif
	return;
	// end of GCC
#endif

	infoOut->type = CompilerInfo::CompilerType::Unknown;
	infoOut->compilerVersion = 0;
	infoOut->useDynamicCRT = false;
	infoOut->isDebug = false;
	infoOut->is64Bit = false;
	infoOut->iteratorDebugLevel = 0;
	infoOut->cppExceptions = 0;
	infoOut->cpp;
}

extern "C" PECS_DLL_EXPORT_ONLY bool IsSameCompilerABI(const CompilerInfo* infoA, const CompilerInfo* infoB) {
	if (!infoA || !infoB) return false;

	uint32_t sizeA = *reinterpret_cast<const uint32_t*>(infoA);
	uint32_t sizeB = *reinterpret_cast<const uint32_t*>(infoB);

	if (sizeA != sizeB) return false;
	if (sizeA > sizeof(CompilerInfo)) return false;

	return std::memcmp(infoA, infoB, sizeA) == 0;
}
