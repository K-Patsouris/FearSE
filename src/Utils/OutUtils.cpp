#include "OutUtils.h"


namespace OutUtils {

	// Debug
	void MessageBox(const string& msg) noexcept {
		RE::DebugMessageBox(msg.c_str());
	}

	bool DumpTextToFile(const string& path, const string& text, bool append) noexcept {
		std::ofstream file;
		if (append) {
			file.open(path.c_str(), std::ios::out | std::ios::app);
		}
		else {
			file.open(path.c_str(), std::ios::out | std::ios::trunc);
		}
		if (file.is_open()) {
			file << text;
			return true;
		}
		return false;
	}

	bool SendModEvent(const string& eventName, const string& strArg, const float numArg, RE::TESForm* sender) noexcept {
		if (const auto modCallbackEventSource = SKSE::GetModCallbackEventSource(); modCallbackEventSource) {
			const auto modEvent = SKSE::ModCallbackEvent{ eventName, strArg, numArg, sender };
			modCallbackEventSource->SendEvent(&modEvent);
			return true;
		}
		return false;
	}

}

