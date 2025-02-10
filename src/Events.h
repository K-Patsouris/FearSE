#pragma once
#include "Common.h"

namespace Events {

	// Fires after loading a save, when the GAME is loaded (not when the SAVE is loaded). Basically OnPlayerLoadGame without the silly Alias script thingy.
	class TESLoadGameEventHandler : public RE::BSTEventSink<RE::TESLoadGameEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent* event, RE::BSTEventSource<RE::TESLoadGameEvent>* source) override;
	};

	// Fires every time a menu is opened or closed. Includes every menu, some of which aren't very intuitive like the cursor. All have their own files in CommonLibSSE though.
	class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* source) override;
	};

	// 
	class InputEventHandler : public RE::BSTEventSink<RE::InputEvent*> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* event, RE::BSTEventSource<RE::InputEvent*>*) override;
	};

	// Only captures events from SendModEvent. Events sent via ModEvent script aren't caught.
	class ModCallbackEventHandler : public RE::BSTEventSink<SKSE::ModCallbackEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* event, RE::BSTEventSource<SKSE::ModCallbackEvent>* source) override;
	};

	// Fires every time an actor changes location (NOT cell; similar, but when won't fire when moving through open world cells, which have nullptr location)
	class TESActorLocationChangeEventHandler : public RE::BSTEventSink<RE::TESActorLocationChangeEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override;
	};

	// Fires every time an actor equips or unequips and item or a spell
	class TESEquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>* source) override;
	};

	// Fires on dragon soul absorb. event->souls seems to always be 0.0f
	class DragonSoulsGainedEventHandler : public RE::BSTEventSink<RE::DragonSoulsGained::Event> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::DragonSoulsGained::Event* event, RE::BSTEventSource<RE::DragonSoulsGained::Event>* source) override;
	};

	// Fires when ANY actor is killed by ANY actor
	class ActorKillEventHandler : public RE::BSTEventSink<RE::ActorKill::Event> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::ActorKill::Event* event, RE::BSTEventSource<RE::ActorKill::Event>* source) override;
	};



	// For the "wait", "sleep", and "fast travel end" events, if autosave for them is enabled, autosaving happens BEFORE the thing starts, so the pre-wait data is saved.
	
	// Fires every time fast travel ends
	class TESFastTravelEndEventHandler : public RE::BSTEventSink<RE::TESFastTravelEndEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESFastTravelEndEvent* event, RE::BSTEventSource<RE::TESFastTravelEndEvent>* source) override;
	};

	// Fires every time wait starts		
	class TESWaitStartEventHandler : public RE::BSTEventSink<RE::TESWaitStartEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESWaitStartEvent* event, RE::BSTEventSource<RE::TESWaitStartEvent>* source) override;
	};
	// Fires every time wait stops
	class TESWaitStopEventHandler : public RE::BSTEventSink<RE::TESWaitStopEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESWaitStopEvent* event, RE::BSTEventSource<RE::TESWaitStopEvent>* source) override;
	};

	// Fires every time wait starts
	class TESSleepStartEventHandler : public RE::BSTEventSink<RE::TESSleepStartEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESSleepStartEvent* event, RE::BSTEventSource<RE::TESSleepStartEvent>* source) override;
	};
	// Fires every time wait stops
	class TESSleepStopEventHandler : public RE::BSTEventSink<RE::TESSleepStopEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESSleepStopEvent* event, RE::BSTEventSource<RE::TESSleepStopEvent>* source) override;
	};




	// UNUSED
	// Fires every time a form (reference?) is deleted. Unreliable.
	/*class TESFormDeleteEventHandler : public RE::BSTEventSink<RE::TESFormDeleteEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* event, RE::BSTEventSource<RE::TESFormDeleteEvent>*) override;
	};*/

	// Fires on Fast Travel (kPre -> kPost -> kFinish) and maybe on package updates (untested). Does NOT fire after loading a save.
	/*class PositionPlayerEventHandler : public RE::BSTEventSink<RE::PositionPlayerEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::PositionPlayerEvent* event, RE::BSTEventSource<RE::PositionPlayerEvent>* source) override;
	};*/

	// Fires after a cell finishes loading. Rudimentary testing found inconsistencies. No use for it atm anyway.
	/*class TESCellFullyLoadedEventHandler : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;
	};*/

	// Fires on player levelup
	/*class LevelIncreaseEventHandler : public RE::BSTEventSink<RE::LevelIncrease::Event> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::LevelIncrease::Event* event, RE::BSTEventSource<RE::LevelIncrease::Event>* source) override;
	};*/
	// Fires every time some tracked stats increase. Unreliable.
	/*class TESTrackedStatsEventHandler : public RE::BSTEventSink<RE::TESTrackedStatsEvent> {
	public:
		static void Register();
	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESTrackedStatsEvent* event, RE::BSTEventSource<RE::TESTrackedStatsEvent>* source) override;
	};*/


}
