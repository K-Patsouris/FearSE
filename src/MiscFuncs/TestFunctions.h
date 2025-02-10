#pragma once
#include "Common.h"

namespace TestFunctions {
	inline constexpr string_view script = "ODFunctionsTest"sv;
	bool Register(IVM* vm);
}
