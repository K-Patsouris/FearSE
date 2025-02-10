#pragma once
#include "Types/TrivialHandle.h"

namespace Vanilla {

	enum class RCE : size_t {
		// Skyrim.esm
		Draugr = 0,			// DraugrRace

		Dremora = 1,		// DremoraRace
		AtronachFlame = 2,	// AtronachFlameRace
		AtronachFrost = 3,	// AtronachFrostRace
		AtronachStorm = 4,	// AtronachStormRace

		DragonPriest = 5,	// DragonPriestRace

		Argonian = 6,		// ArgonianRace
		Breton = 7,			// BretonRace
		DarkElf = 8,		// DarkElfRace
		HighElf = 9,		// HighElfRace
		Imperial = 10,		// ImperialRace
		Khajiit = 11,		// KhajiitRace
		Nord = 12,			// NordRace
		Orc = 13,			// OrcRace
		Redguard = 14,		// RedguardRace
		WoodElf = 15,		// WoodElfRace

		// Dragonborn.es,
		Lurker = 16,		// DLC2LurkerRace
		Seeker = 17,		// DLC2SeekerRace

		Total
	};

	enum class FAC : size_t {
		// Skyrim.esm
		Creature = 0,	// CreatureFaction

		Total
	};

	enum class KWD : size_t {
		NPC = 0,		// ActorTypeNPC			All Dremora and playable races have this.
		Creature = 1,	// ActorTypeCreature		Almost every non-playable has this. Netch race only has ActorTypeAnimal. Maybe more exceptions.
		Daedra = 2,		// ActorTypeDaedra		Daedra races have this, except Lurker race. Attronach races and Lucker race have ActorTypeCreature. Dremora race has ActorTypeNPC.
		Animal = 3,		// ActorTypeAnimal		Races with this also generally have ActorTypeCreature. Netch race only has this. Maybe more exceptions.
		Dwarven = 4,	// ActorTypeDwarven		Automaton races have this. Some Ash Spawn race also has this. Races with this seem not to have ActorTypeCreature.
		Dragon = 5,		// ActorTypeDragon		Dragon races have both this and ActorTypeCreature.

		Total
	};




	[[nodiscard]] bool BodyKwdFind(const RE::BGSKeyword* kwd) noexcept;
	[[nodiscard]] bool IsNPCRace(const RE::TESRace* race) noexcept;
	[[nodiscard]] bool IsDaedraRace(const RE::TESRace* kwd) noexcept;
	[[nodiscard]] bool IsDraugrRace(const RE::TESRace* kwd) noexcept;
	[[nodiscard]] bool IsDragonPriestRace(const RE::TESRace* kwd) noexcept;

	[[nodiscard]] RCE RaceToID(const RE::TESRace* race) noexcept;

	[[nodiscard]] const std::array<RE::BGSKeyword*, 33>& HostileLocKeywordss() noexcept;
	[[nodiscard]] const std::array<RE::BGSKeyword*, 24>& PeacefulLocKeywords() noexcept;
	[[nodiscard]] const std::array<RE::BGSKeyword*, 2>& BodyKeywords() noexcept;

	[[nodiscard]] RE::TESFaction* Faction(const FAC fac) noexcept;

	[[nodiscard]] RE::PlayerCharacter* Player() noexcept;
	[[nodiscard]] TrivialHandle::trivial_handle PlayerHandle() noexcept;

	[[nodiscard]] RE::TESObjectMISC* Gold() noexcept;

	[[nodiscard]] RE::TESGlobal* GameDaysPassed() noexcept;


	[[nodiscard]] bool FillForms() noexcept;

}



