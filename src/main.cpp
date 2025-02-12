#include "Common.h"
#include "Logger.h"
#include "MiscFuncs/GenericFunctions.h"	// Papyrus functions
#include "MiscFuncs/TestFunctions.h"	// Papyrus functions
#include "Data.h"						// Papyrus functions
#include "RulesMenu.h"					// Custom OD Menu
#include "Forms/FearForms.h"			// Form lookup cache
#include "Forms/RulesForms.h"			// Form lookup cache
#include "Forms/VanillaForms.h"			// Form lookup cache
#include "Forms/HurdlesForms.h"			// Form lookup cache
#include "CustomMenu.h"					// Custom UI menu, ala SKSE
#include "Events.h"						// Event registration
#include "Periodic.h"					// Timer threads start
#include "Scaleform.h"					// Scaleform hooks
#include "Serialization.h"				// Serialization callbacks


[[nodiscard]] static bool InitializeLog() {
	auto path = SKSE::log::log_directory();
	if (!path) {
		return false;
	}

	*path /= string_view{ string{ Version::PROJECT } + ".log" };
	if (!Log::Init(path.value().string())) {
		return false;
	}

	static auto file_token = Log::RegisterSink(Log::FileCallback, { .skse = Log::Severity::info, .papyrus = Log::Severity::error }); // info, error
	static auto console_token = Log::RegisterSink(Log::ConsoleCallback, { .skse = Log::Severity::info, .papyrus = Log::Severity::info }); // error, info
	Log::Info("{} v{}.{}.{}"sv, Version::PROJECT, Version::MAJOR, Version::MINOR, Version::PATCH);
	return true;
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (!InitializeLog()) {
		SKSE::stl::report_and_fail("Failed to initialize logging"sv);
	}

	if (a_skse->IsEditor()) {
		Log::Critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		Log::Critical("Unsupported runtime version {}"sv, ver.string());
		return false;
	}

	return true;
}

static void MessageHandler(SKSE::MessagingInterface::Message* msg) {
	if (!msg) {
		return;
	}
	switch (msg->type) {
	case SKSE::MessagingInterface::kDataLoaded: { // SKSE Plugins and .esm/.esp/.esl files have been loaded, so we can start interacting with their forms. Happens before Main Menu finishes loading.

		const auto papyrus = SKSE::GetPapyrusInterface();
		if (!papyrus) {
			Log::Critical("Failed to get Papyrus interface! Plugin will not be available!"sv);
			return;
		}
		// Register generic Papyrus functions
		if (papyrus->Register(Functions::Register) and papyrus->Register(TestFunctions::Register)) {
			Log::Info("Registered generic (and debug) Papyrus functions."sv); 
		} else {
			Log::Warning("Failed to register Papyrus functions!"sv);
		}
		
		// Fill basic forms (required for PlayerRules)
		if (!Vanilla::FillForms()) {
			Log::Critical("Failed to find the required vanilla forms! Only generic Papyrus functions will be available!"sv);
			return;
		}
		if (!PlayerRules::FillForms()) {
			Log::Critical("Failed to find the required PlayerRules forms! Only generic Papyrus functions will be available!"sv);
			return;
		}
		if (!Hurdles::FillForms()) {
			Log::Critical("Failed to find the required vanilla forms! Only generic Papyrus functions will be available!"sv);
			return;
		}
		Log::Info("Loaded all basic required forms."sv);

		// Basic forms are filled from here onwards
		
		if (const auto serialization = SKSE::GetSerializationInterface(); !serialization) {
			Log::Critical("Failed to get Serialization interface! Only generic Papyrus functions will be available!"sv);
			return;
		} else if (!papyrus->Register(Data::Functions::RegisterBasic)) {
			Log::Critical("Failed to register basic plugin interface Papyrus functions! Only generic Papyrus functions will be available!"sv);
			return;
		} else {
			// Got serialization interface and registered main Papyrus functions. Can now register everything else.
			serialization->SetUniqueID(Serialization::PluginSerializationType);
			serialization->SetSaveCallback(Serialization::SaveCallback);
			serialization->SetLoadCallback(Serialization::LoadCallback);
			serialization->SetRevertCallback(Serialization::RevertCallback);

			// Register event callbacks
			Events::TESLoadGameEventHandler::Register();
			Events::MenuOpenCloseEventHandler::Register();
			Events::InputEventHandler::Register();
			Events::ModCallbackEventHandler::Register();
			Events::TESActorLocationChangeEventHandler::Register();
			Events::TESEquipEventHandler::Register();
			Events::DragonSoulsGainedEventHandler::Register();
			Events::ActorKillEventHandler::Register();

			Events::TESFastTravelEndEventHandler::Register();
			Events::TESSleepStartEventHandler::Register();
			Events::TESSleepStopEventHandler::Register();
			Events::TESWaitStartEventHandler::Register();
			Events::TESWaitStopEventHandler::Register();
			
			// Register custom menu
			// CustomMenu::MenuManagerSingleton.RegisterMenu();
			Data::PlayerRules::register_pr_menu();

			Log::Info("Registered serialization callbacks, event callbacks, and plugin interface Papyrus functions. PlayerRules functionality is available."sv);
		}

		if (!Fear::FillForms()) { // No Fear forms is not an error.
			Log::Warning("Failed to find required Fear forms! Safe to ignore if you are not using the Fear rework part of the mod."sv);
		} else {
			if (!papyrus->Register(Data::Functions::RegisterFear)) {
				Log::Warning("Failed to register Fear interface Papyrus functions! Fear rework will not be available. Safe to ignore if you are not using the Fear rework part of the mod."sv);
			} else {
				Data::Shared::InstallHooks();
				Log::Info("Loaded Fear-required forms, registered Fear interface Papyrus functions, and applied Fear-required hooks. Fear rework is available."sv);
			}
		}

		break;
	}
	/*case SKSE::MessagingInterface::kPostLoadGame: {
		// CAUTION: This fires BEFORE loading screen starts.
		// For example, if you try to load a save but miss an .esp you'll get the usual messagebox warning you about it.
		// At this point this will have ALREADY fired. It WILL fire again after load though.
		break;
	}*/
	}
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
	Log::Info("Plugin Loaded"sv);

	SKSE::Init(skse);

	if (const auto messaging = SKSE::GetMessagingInterface(); !messaging or !messaging->RegisterListener(MessageHandler)) {
		Log::Critical("Failed to get Messaging interface or to register listener! Plugin will not be available!"sv);
		return false;
	}

	if (const auto scaleform = SKSE::GetScaleformInterface(); !scaleform or !scaleform->Register(Scaleform::install_hooks, "ODCommon")) {
		Log::Critical("Failed to get get Scaleform interface! Scaleform hooks aborted!"sv);
	}

	return true;
}

