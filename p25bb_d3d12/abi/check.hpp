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
		// コンパイラのバージョン
		// vtable等の扱いや、名前修飾が変わったときの互換性問題を対策するため
		// 全てのバイナリが同じコンパイラでビルドされることを前提にしている
		uint32_t compilerVersion;
		// ここからはboolにキャストするべき値
		// /MDか/MDd(MSVC)になっているか
		// new deleteや、STL(これだけでは互換性は保証できないが)の安全性をある程度保証するため
		uint8_t useDynamicCRT;
		// デバッグビルドか
		// リリースビルドとデバッグビルドでサイズが変わるクラスや構造体があるかもしれないので
		uint8_t isDebug;
		// 64bit向けビルドか
		// size_tのサイズが合わなくなるのの対策するため
		// というか、関数呼び出しや使う命令すら変わるので動かないはず
		// そもそもロードできるかすら疑問だけど一応
		uint8_t is64Bit;
		// ここまで
		// 意味はないデータ(多分(適切に初期化されていれば)0xFF)
		uint8_t padding;
		// イテレータのデバッグレベル
		// よくわかってないけど、これの値が違うとstd::vector等のSTLコンテナのサイズや挙動が変わりそうなので
		uint32_t iteratorDebugLevel;
		// C++例外のバージョン
		// 正直例外が有効になっているかだけでいい
		uint32_t cppExceptions;
		// C++標準
		// (ほとんどないと思うが)C++標準の例えばメモリアラインメント等の規定に破壊的な変更があった場合の対策
		uint32_t cpp;
		// 意味はないデータ(多分0xFFFFFFFF)
		uint32_t reserved[1];
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
