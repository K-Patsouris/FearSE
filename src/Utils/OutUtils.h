#pragma once
#include "Common.h"

namespace OutUtils {

	// Debug
	void MessageBox(const string& message) noexcept;
	bool DumpTextToFile(const string& path, const string& text, bool append) noexcept;

	bool SendModEvent(const string& eventName, const string& strArg, const float numArg, RE::TESForm* sender) noexcept;
	
	
}
