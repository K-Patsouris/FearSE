#include "Events.h"
#include "Logger.h"

#include "Data.h"				// PlayerRules task completion stuff
#include "Periodic.h"			// Pause/Unpase in MenuOpenCloseEventHandler
#include "Forms/VanillaForms.h"	// PlayerRules task completion stuff
#include "Utils/MenuUtils.h"	// NotifyMenuChange for custom menu handling
#include "Utils/StringUtils.h"
using StringUtils::StrICmp;

#include "Utils/PrimitiveUtils.h"
using PrimitiveUtils::u32_xstr;

namespace Events {

	void PrintTID(const string& caller) {
		// auto asd = std::this_thread::get_id();
		// unsigned int tid = *reinterpret_cast<unsigned int*>(std::addressof(asd));
		// Log::Info("<{}> ran in thread <{}>"sv, caller, tid);
		Log::Info("<{}> ran in thread <{}>"sv, caller, std::bit_cast<unsigned int>(std::this_thread::get_id()));
	}

	// TESLoadGameEventHandler	Seemingly runs in main thread
	void TESLoadGameEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESLoadGameEvent registration aborted!"sv);
			return;
		}
		static TESLoadGameEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESLoadGameEvent"sv);
	}
	RE::BSEventNotifyControl TESLoadGameEventHandler::ProcessEvent(const RE::TESLoadGameEvent* event, RE::BSTEventSource<RE::TESLoadGameEvent>*) {
		if (event) {
			Periodic::Timer::Start();
		}
		return RE::BSEventNotifyControl::kContinue;
	}


	// MenuOpenCloseEventHandler
	void MenuOpenCloseEventHandler::Register() {
		auto source = RE::UI::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain UI singleton! MenuOpenCloseEvent registration aborted!"sv);
			return;
		}
		static MenuOpenCloseEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for MenuOpenCloseEvent"sv);
	}
	RE::BSEventNotifyControl MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* source) {
		/*	BSFixedString menuName;  // 00
			bool          opening;   // 08
			std::uint8_t  pad09;     // 09
			std::uint16_t pad0A;     // 0A
			std::uint32_t pad0C;     // 0C*/
		if (event and source) {
			Periodic::Timer::MenuEvent();
			MenuUtils::NotifyMenuChange(event->menuName.c_str(), event->opening);
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	// InputEventHandler
	void InputEventHandler::Register() {
		auto source = RE::BSInputDeviceManager::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain BSInputDeviceManager singleton! InputEventHandler registration aborted!"sv);
			return;
		}
		static InputEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for InputEvent"sv);
	}
	RE::BSEventNotifyControl InputEventHandler::ProcessEvent(RE::InputEvent* const* event, RE::BSTEventSource<RE::InputEvent*>*) {
		if (!event) {
			return RE::BSEventNotifyControl::kContinue;
		}
		static std::array<string_view, 4> DeviceNames{ "Keyboard", "Mouse", "Gamepad", "Virtual Keyboard" };
		for (auto it = *event; it != nullptr; it = it->next) {
			continue;
			auto btnev = it->AsButtonEvent();
			if (btnev and btnev->IsDown()) {
				u32 device = static_cast<u32>(btnev->GetDevice());
				Log::Info(">> InputHandler received <{}> button down event with id <{}>"sv,
						  device < DeviceNames.size() ? DeviceNames[device] : "unknown"sv,
						  btnev->GetIDCode()
				);
			}
		}
		
		return RE::BSEventNotifyControl::kContinue;
	}

	// ModCallbackEventHandler
	void ModCallbackEventHandler::Register() {
		const auto source = SKSE::GetModCallbackEventSource();
		if (!source) {
			Log::Error("Failed to obtain ModCallbackEventSource singleton! ModCallbackEvent registration aborted!"sv);
			return;
		}
		static ModCallbackEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for ModCallbackEvent"sv);
	}
	RE::BSEventNotifyControl ModCallbackEventHandler::ProcessEvent(const SKSE::ModCallbackEvent* event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) {
		/*	RE::BSFixedString eventName;
			RE::BSFixedString strArg;
			float             numArg;
			RE::TESForm*      sender;*/
		if (!event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MenuUtils::NotifyModEvent(event);

		if (!event->sender) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (StrICmp(event->eventName.c_str(), "AnimationStart")) {
			Data::Shared::BrawlEvent(event->sender, false);
			return RE::BSEventNotifyControl::kContinue;
		}
		else if (StrICmp(event->eventName.c_str(), "AnimationEnd")) {
			Data::Shared::BrawlEvent(event->sender, true);
			return RE::BSEventNotifyControl::kContinue;
		}

		return RE::BSEventNotifyControl::kContinue;
	}


	// TESActorLocationChangeEventHandler
	void TESActorLocationChangeEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESActorLocationChangeEvent registration aborted!"sv);
			return;
		}
		static TESActorLocationChangeEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESActorLocationChangeEvent"sv);
	}
	RE::BSEventNotifyControl TESActorLocationChangeEventHandler::ProcessEvent(const RE::TESActorLocationChangeEvent* event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) {
		/*	NiPointer<TESObjectREFR> actor;
			BGSLocation*             oldLoc;
			BGSLocation*             newLoc;
		Fires on game load. For player it's from oldLoc = nullptr to newLoc = wherever they load. For loaded NPCs probably too (untested).
			If loading a save from in-game and not from main-menu, oldLoc = location BEFORE loading the new save. Player doesn't get moved to a limbo or anything first. Just directly relocated.
				It also fires BEFORE the load starts, but AFTER the revert.
		Fires BEFORE autosaving, if autosaving on travel is enabled.
		Fires when just moving in the open world. No loading screens required.
		Not everything is a location. newLoc = nullptr when moving to a non-location, like a lot of the open world.
			When in a non-location, it will NOT fire until moving to an actual location. So it doesn't seem to be tied to cell change (there should be BGSActorCellEvent for that).
		The actual radius of a location is often bigger than it's discovery radius. This can fire when moving to a location but before getting close enough to discover it.*/
		if (event and event->actor and event->actor->IsPlayerRef()) {
			Data::Fear::PlayerChangedLocation(event->newLoc);
		}
		return RE::BSEventNotifyControl::kContinue;
	}


	// TESEquipEventHandler	runs in random non-main threads
	void TESEquipEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESEquipEvent registration aborted!"sv);
			return;
		}
		static TESEquipEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESEquipEvent"sv);
	}
	RE::BSEventNotifyControl TESEquipEventHandler::ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) {
		/*	NiPointer<TESObjectREFR> actor;         // 00	// Contains the pointer to the actor (RE::Actor*). Get it with .get()
			FormID                   baseObject;    // 08	// FormID of the object being equipped/unequipped
			FormID                   originalRefr;  // 0C	// Seems to always be 0
			std::uint16_t            uniqueID;      // 10	// As does this. Maybe there are unique items that use this field?
			bool                     equipped;      // 12	// true if the actor is equipping the baseObject and false if unequipping
			std::uint8_t             pad13;         // 13
			std::uint32_t            pad14;         // 14
			
				For baseObject:	LookupByID<TESBoundObject> returns valid ptr for  TESObjectARMO / TESObjectWEAP / SpellItem / TESObjectLIGH / AlchemyItem  but not for TESShout.
								Shields are TESObjectARMO. Potions (& Poisons?) are AlchemyItem. Torches are TESObjectLIGH.
								Unarmed ("Fist") is TESObjectWEAP with type kHandToHandMelee (0) and is auto-equipped after unequipping TESObjectWEAP or SpellItem.
									The player (and NPCs?) right after game start does NOT have Fists equipped but can still fight unarmed (maybe less dmg?). Swinging with a fist does not change it.
									In this state, EQUIPPING any TESObjectWEAP or SpellItem will NOT UNEQUIP any Fist as there are none equipped.
									Normally, unequipping any TESObjectWEAP or SpellItem will equip a Fist in that hand the next time the game is unpaused (so unequipping from menu will do this
									only after leaving the menu and the game gets unpaused). However, equipping a TESObjectWEAP or SpellItem in a SINGLE in this state and then unequipping it,
									will cause Fist to be equipped in BOTH hands. So, unequipping TESObjectWEAP or SpellItem seems to fill ALL truly empty hands with Fist.
								LookupByID<...> gives valid ptr only for the one it is.
		*/
		if (event) {
			// Idk if copying the NiPointer is required. If nothing deletes the event object or changes its actor member while we process, then just using the raw pointer should be fine.
			// The Is3DLoaded() check is there to filter the "equip things on load" thing. Every Actor "reequips" all their meant-to-be-equipped gear upon being loaded.
			// We don't want this. Besides the extra useless work, it triggers our on-equip stuff when it reasonably shouldn't.
			// An Actor should not be able to equip things they already had equipped, as far as we, on a high level, are concerned.
			// Fortunately, it so happens that Is3DLoaded() will always return false while the game does that reequip thing, so use it to filter.
			if (auto objref = event->actor.get(); objref and (objref->GetFormType() == RE::Actor::FORMTYPE) and objref->Is3DLoaded()) {
				if (RE::TESBoundObject* obj = RE::TESForm::LookupByID<RE::TESBoundObject>(event->baseObject); obj) { // It's ok to ignore shouts. We don't use those.
					RE::Actor* act = static_cast<RE::Actor*>(objref);

					if (event->equipped) {
						Data::Shared::ProcessEquip(act, obj);
					}
					else {
						Data::Shared::ProcessUnequip(act, obj);
					}

					/*	// Printing
					const auto formtype = obj->GetFormType();
					const i64 typeidx =	-1
											+ static_cast<i64>(formtype == RE::TESObjectARMO::FORMTYPE)		// +1 = 0
											+ (static_cast<i64>(formtype == RE::TESObjectWEAP::FORMTYPE) * 2)	// +2 = 1
											+ (static_cast<i64>(formtype == RE::SpellItem::FORMTYPE) * 3);	// +3 = 2
					if (typeidx >= 0) {
						const array<string, 2> evnt{ "-------", "++++" };
						const array<string, 3> type{ "Armor", "Weapon", "Spell" };
						const array<string, 2> load{ "--", "+" };

						Log::ToConsole("{}({}) {} {}: <{}[{}]>"sv, 
									   act->GetDisplayFullName(),
									   load[act->Is3DLoaded()],
									   evnt[event->equipped],
									   type[typeidx],
									   obj->GetName(),
									   u32_xstr(obj->formID));
					}
					return RE::BSEventNotifyControl::kContinue;*/

				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}


	// DragonSoulsGainedEventHandler
	void DragonSoulsGainedEventHandler::Register() {
		auto source = RE::DragonSoulsGained::GetEventSource();
		if (!source) {
			Log::Error("Failed to obtain DragonSoulsGained event source! DragonSoulsGained::Event registration aborted!"sv);
			return;
		}
		static DragonSoulsGainedEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for DragonSoulsGained::Event"sv);
	}
	RE::BSEventNotifyControl DragonSoulsGainedEventHandler::ProcessEvent(const RE::DragonSoulsGained::Event* event, RE::BSTEventSource<RE::DragonSoulsGained::Event>*) {
		/*	float         souls;  // 00		ALWAYS 0.0f!!!
			std::uint32_t pad04;  // 04*/
		if (event) {
			Data::PlayerRules::PlayerAbsorbedDragonSoul();
		}
		return RE::BSEventNotifyControl::kContinue;
	}


	// ActorKillEventHandler
	void ActorKillEventHandler::Register() {
		auto source = RE::ActorKill::GetEventSource();
		if (!source) {
			Log::Error("Failed to obtain ActorKill event source! ActorKill::Event registration aborted!"sv);
			return;
		}
		static ActorKillEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for ActorKill::Event"sv);
	}
	RE::BSEventNotifyControl ActorKillEventHandler::ProcessEvent(const RE::ActorKill::Event* event, RE::BSTEventSource<RE::ActorKill::Event>*) {
		/*	Actor* killer;  // 00
			Actor* victim;  // 08*/
		if (event) {
			if ((event->killer and event->killer->IsPlayerRef()) bitand (event->victim != nullptr)) { // Can eliminate a branch with a bitand
				if (const auto race = event->victim->GetRace(); race) {
					if (Vanilla::IsDraugrRace(race)) { Data::PlayerRules::PlayerKillDraugr(); }
					else if (Vanilla::IsDaedraRace(race)) { Data::PlayerRules::PlayerKillDaedra(); }
					else if (Vanilla::IsDragonPriestRace(race)) { Data::PlayerRules::PlayerKillDragonPriest(); }
				}
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}



	// TESFastTravelEndEventHandler
	void TESFastTravelEndEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESFastTravelEndEvent registration aborted!"sv);
			return;
		}
		static TESFastTravelEndEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESFastTravelEndEvent"sv);
	}
	RE::BSEventNotifyControl TESFastTravelEndEventHandler::ProcessEvent(const RE::TESFastTravelEndEvent* event, RE::BSTEventSource<RE::TESFastTravelEndEvent>*) {
		/*	float         fastTravelEndHours;  // 00	The hours the travel took from start to finish, NOT the total game hours passed at the end
			std::uint32_t pad04;               // 04*/
		if (event) {
			Data::Shared::FastTravelEnd(event->fastTravelEndHours);
		}
		return RE::BSEventNotifyControl::kContinue;
	}


	// TESWaitStartEventHandler
	void TESWaitStartEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESWaitStartEvent registration aborted!"sv);
			return;
		}
		static TESWaitStartEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESWaitStartEvent"sv);
	}
	RE::BSEventNotifyControl TESWaitStartEventHandler::ProcessEvent(const RE::TESWaitStartEvent* event, RE::BSTEventSource<RE::TESWaitStartEvent>*) {
		if (event) {
			Data::PlayerRules::IdleTimeStart();
		}
		return RE::BSEventNotifyControl::kContinue;
	}
	// TESWaitStopEventHandler
	void TESWaitStopEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESWaitStopEvent registration aborted!"sv);
			return;
		}
		static TESWaitStopEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESWaitStopEvent"sv);
	}
	RE::BSEventNotifyControl TESWaitStopEventHandler::ProcessEvent(const RE::TESWaitStopEvent* event, RE::BSTEventSource<RE::TESWaitStopEvent>*) {
		if (event) {
			Data::PlayerRules::IdleTimeEnd();
		}
		return RE::BSEventNotifyControl::kContinue;
	}


	// TESSleepStartEventHandler
	void TESSleepStartEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESSleepStartEvent registration aborted!"sv);
			return;
		}
		static TESSleepStartEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESSleepStartEvent"sv);
	}
	RE::BSEventNotifyControl TESSleepStartEventHandler::ProcessEvent(const RE::TESSleepStartEvent* event, RE::BSTEventSource<RE::TESSleepStartEvent>* ) {
		if (event) {
			Data::PlayerRules::IdleTimeStart();
		}
		return RE::BSEventNotifyControl::kContinue;
	}
	// TESSleepStopEventHandler
	void TESSleepStopEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESSleepStopEvent registration aborted!"sv);
			return;
		}
		static TESSleepStopEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESSleepStopEvent"sv);
	}
	RE::BSEventNotifyControl TESSleepStopEventHandler::ProcessEvent(const RE::TESSleepStopEvent* event, RE::BSTEventSource<RE::TESSleepStopEvent>*) {
		// 	bool interrupted;  // 0
		if (event) {
			Data::PlayerRules::IdleTimeEnd();
		}
		return RE::BSEventNotifyControl::kContinue;
	}









	// TESFormDeleteEventHandler
	// Apparently this only fires for deleted TESObjectREFR. Unreliable. Don't use as a pointer validity mechanic.
	/*void TESFormDeleteEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESFormDeleteEvent registration aborted!"sv);
			return;
		}
		static TESFormDeleteEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESFormDeleteEvent"sv);
	}
	RE::BSEventNotifyControl TESFormDeleteEventHandler::ProcessEvent(const RE::TESFormDeleteEvent* event, RE::BSTEventSource<RE::TESFormDeleteEvent>*) {
		// FormID        formID;  // 00
		// std::uint32_t pad04;   // 04
		if (event and event->formID != 0) {
			;
		}
		return RE::BSEventNotifyControl::kContinue;
	}*/

	// PositionPlayerEventHandler
	/*void PositionPlayerEventHandler::Register() {
		auto source = RE::PlayerCharacter::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain PlayerCharacter singleton! PositionPlayerEvent registration aborted!"sv);
			return;
		}
		static PositionPlayerEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for PositionPlayerEvent"sv);
	}
	RE::BSEventNotifyControl PositionPlayerEventHandler::ProcessEvent(const RE::PositionPlayerEvent* event, RE::BSTEventSource<RE::PositionPlayerEvent>*) {
		if (event) {
			// check event->type to see what happened
		}

		return RE::BSEventNotifyControl::kContinue;
	}*/

	// TESCellFullyLoadedEventHandler
	/*void TESCellFullyLoadedEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESCellFullyLoadedEvent registration aborted!"sv);
			return;
		}
		static TESCellFullyLoadedEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESCellFullyLoadedEvent"sv);
	}
	RE::BSEventNotifyControl TESCellFullyLoadedEventHandler::ProcessEvent(const RE::TESCellFullyLoadedEvent* event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) {
		// 	TESObjectCELL* cell;
		if (event and event->cell) {
			// Log::ToConsole("\t<{}[{}]> fully loaded"sv, event->cell->GetName(), u32_xstr(event->cell->formID));
		}
		return RE::BSEventNotifyControl::kContinue;
	}*/

	// LevelIncreaseEventHandler
	/*void LevelIncreaseEventHandler::Register() {
		auto source = RE::LevelIncrease::GetEventSource();
		if (!source) {
			Log::Error("Failed to obtain LevelIncrease event source! LevelIncreaseEventHandler registration aborted!"sv);
			return;
		}
		static LevelIncreaseEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for LevelIncrease::Event"sv);
	}
	RE::BSEventNotifyControl LevelIncreaseEventHandler::ProcessEvent(const RE::LevelIncrease::Event* event, RE::BSTEventSource<RE::LevelIncrease::Event>*) {
		// PlayerCharacter* player;    // 00
		// std::uint16_t    newLevel;  // 08
		// std::uint16_t    pad0A;     // 0A
		// std::uint32_t    pad0C;     // 0C
		if (event) {
			// Data::PlayerRules::PlayerLevelUp();
		}
		return RE::BSEventNotifyControl::kContinue;
	}*/

	// TESTrackedStatsEventHandler
	/*void TESTrackedStatsEventHandler::Register() {
		auto source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log::Error("Failed to obtain ScritEventSourceHolder singleton! TESTrackedStatsEvent registration aborted!"sv);
			return;
		}
		static TESTrackedStatsEventHandler singleton;
		source->AddEventSink(&singleton);
		Log::Info("Registered for TESTrackedStatsEvent"sv);
	}
	RE::BSEventNotifyControl TESTrackedStatsEventHandler::ProcessEvent(const RE::TESTrackedStatsEvent* event, RE::BSTEventSource<RE::TESTrackedStatsEvent>*) {
		// BSFixedString stat;   // 00
		// std::int32_t  value;  // 08
		// std::uint32_t pad0C;  // 0C
		if (event) {
			if (StringUtils::StrICmp(event->stat.c_str(), "Level Increases")) {
				// Data::PlayerRules::PlayerLevelUp();
			}
			else if (StringUtils::StrICmp(event->stat.c_str(), "Dragon Souls Collected")) {
				Data::PlayerRules::PlayerAbsorbedDragonSoul();
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}*/

}

