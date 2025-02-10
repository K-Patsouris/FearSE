#include "Scaleform.h"
#include "Common.h"
#include "Logger.h"
#include "RulesMenu.h"
#include "Utils/StringUtils.h"


namespace Scaleform {

	class prm_report_selection : public RE::GFxFunctionHandler {
	public:
		void Call(Params& params) override {
			if (params.argCount > 0) {
				const u32 btn_id = static_cast<u32>(params.args[0].GetNumber());
				Log::Info("Scaleform::prm_report_selection::Call: Notifying PlayerRules menu with button ID <{}> (<{:.2}>)"sv, btn_id, params.args[0].GetNumber());
				::Data::PlayerRules::menu_button_chosen(btn_id);
			}
			else {
				Log::Error("Scaleform::prm_report_selection::Call: Called with 0 parameters. Expected a button ID."sv);
			}
		}
	};

	// Return value doesn't seem to matter. It is never checked in SKSE source.
	bool install_hooks(RE::GFxMovieView* view, RE::GFxValue*) {

		if (auto def = view->GetMovieDef(); def == nullptr) {
			Log::Critical("Scaleform::install_hooks: Failed to get moviedef to determine if hooking needed. Hooks aborted!"sv);
			return true;
		}
		else if (auto swf_path = def->GetFileURL(); not StringUtils::StrICmp(swf_path, "Interface/EVM_MBVertical.swf")) {
			// Log::Info("Scaleform::install_hooks: Skipping hooks for <{}>", swf_path);
			return true;
		}
		
		RE::GFxValue globals;
		if (not view->GetVariable(&globals, "_global")) {
			Log::Critical("Scaleform::install_hooks: Failed to get _global. Hooks aborted!"sv);
			return true;
		}

		static prm_report_selection* reporter_ptr{ new prm_report_selection };

		RE::GFxValue prm;
		view->CreateObject(&prm);
		RE::GFxValue fn;
		view->CreateFunction(&fn, reporter_ptr);
		prm.SetMember("ButtonChosen", fn);
		globals.SetMember("prm", prm);

		// Log::Info("Scaleform::install_hooks: Registered hooks!"sv);

		return true;
	}

}

