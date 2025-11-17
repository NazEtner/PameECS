#pragma once

// static_assertをする関数を、グローバル名前空間で呼び出すためのマクロ
#define GLOBAL_CHECK(checker, value) \
[[maybe_unused]] inline constexpr auto PECS_GLOBAL_CHECK_RESULT_##checker = checker(value);

#define GLOBAL_CHECK_NON_ARG(checker) \
[[maybe_unused]] inline constexpr auto PECS_GLOBAL_CHECK_RESULT_##checker = checker();
