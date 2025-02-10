#include "VanillaForms.h"
#include "Common.h"
#include "Logger.h"
#include "Utils/GameDataUtils.h"
#include "Utils/PrimitiveUtils.h"


namespace Vanilla {
	using GameDataUtils::GetPluginFormIDOffsets;
	using PrimitiveUtils::u32_xstr;


	array<RE::BGSKeyword*, 33> HostileLocKwds{};
	array<RE::BGSKeyword*, 24> PeacefulLocKwds{};
	array<RE::BGSKeyword*, 2> BodyKwds{};
	array<RE::TESRace*, static_cast<size_t>(RCE::Total)> Races{};
	array<RE::TESFaction*, static_cast<size_t>(FAC::Total)> Factions{};
	array<RE::BGSKeyword*, static_cast<size_t>(KWD::Total)> RaceKwds{};

	RE::PlayerCharacter* PlayerPtr{ nullptr };
	TrivialHandle::trivial_handle PlayerHandle_{ nullptr };
	RE::TESObjectMISC* GoldPtr{ nullptr };
	RE::TESGlobal* GameDaysPassedPtr{ nullptr };


	bool BodyKwdFind(const RE::BGSKeyword* kwd) noexcept { return (kwd == BodyKwds[0]) bitor (kwd == BodyKwds[1]); }
	bool IsNPCRace(const RE::TESRace* race) noexcept {
		u8 result = 0;
		for (auto it = race->keywords, end = race->keywords + race->numKeywords; it != end; ++it) {
			result |= (*it == RaceKwds[static_cast<size_t>(KWD::NPC)]);
		}
		return result;
	}
	bool IsDaedraRace(const RE::TESRace* race) noexcept {
		u8 result = 0;
		for (auto it = race->keywords, end = race->keywords + race->numKeywords; it != end; ++it) {
			result |= (*it == RaceKwds[static_cast<size_t>(KWD::Daedra)]);
		}
		return result bitor (race == Races[static_cast<size_t>(RCE::Lurker)]); // Lucker race doesn't have the Daedra keyword zzz
	}
	bool IsDraugrRace(const RE::TESRace* race) noexcept { return race == Races[static_cast<u64>(RCE::Draugr)]; }
	bool IsDragonPriestRace(const RE::TESRace* race) noexcept { return race == Races[static_cast<u64>(RCE::DragonPriest)]; }

	const array<RE::BGSKeyword*, 33>& HostileLocKeywordss() noexcept { return HostileLocKwds; }
	const array<RE::BGSKeyword*, 24>& PeacefulLocKeywords() noexcept { return PeacefulLocKwds; }
	const array<RE::BGSKeyword*, 2>& BodyKeywords() noexcept { return BodyKwds; }

	RCE RaceToID(const RE::TESRace* race) noexcept {
		for (size_t i = 0; i < Races.size(); ++ i) {
			if (race == Races[i]) {
				return static_cast<RCE>(i);
			}
		}
		return RCE::Total;
	}

	RE::TESFaction* Faction(const FAC fac) noexcept { return Factions[static_cast<u64>(fac)]; }

	RE::PlayerCharacter* Player() noexcept { return PlayerPtr; }
	TrivialHandle::trivial_handle PlayerHandle() noexcept { return PlayerHandle_; }

	RE::TESObjectMISC* Gold() noexcept { return GoldPtr; }

	RE::TESGlobal* GameDaysPassed() noexcept { return GameDaysPassedPtr; }


	static bool GetHostileLKwds(RE::FormID skyrimOffset, RE::FormID dragonbornOffset) noexcept {
		static constexpr u64 HostileLocKwdDBStart = 29u;
		static constexpr array<RE::FormID, HostileLocKwds.size()> AllHostileLocKwdIDs{
			// Skyrim.esm hostile	0-28
			0x130EF,	// LocSetCave
			0x100819,	// LocSetCaveIce
			0x130F0,	// LocSetDwarvenRuin
			0x1926A,	// LocSetMilitaryCamp	Only hostile if attacked, but military aura yada yada
			0x130F1,	// LocSetMilitaryFort	These are actually hostile, like Fellglow Keep
			0x130F2,	// LocSetNordicRuin
			0x130DE,	// LocTypeAnimalDen
			0x130DF,	// LocTypeBanditCamp
			0x1CD58,	// LocTypeCemetery		Not combat zone per se, but creepy
			0xF5E80,	// LocTypeClearable
			0x130E0,	// LocTypeDragonLair
			0x130E1,	// LocTypeDragonPriestLair
			0x130E2,	// LocTypeDraugrCrypt
			0x130DB,	// LocTypeDungeon
			0x130E3,	// LocTypeDwarvenAutomatons
			0x130E4,	// LocTypeFalmerHive
			0x130EE,	// LocTypeForswornCamp
			0x130E5,	// LocTypeGiantCamp
			0x130E6,	// LocTypeHagravenNest
			0x1CD59,	// LocTypeJail
			0x130E8,	// LocTypeMilitaryCamp	This is on the same location the ..Set...Camp above is
			0x130E7,	// LocTypeMilitaryFort	Same but used in more places
			0x18EF1,	// LocTypeMine
			0x1CD5B,	// LocTypeShip			Katariah only
			0x1929F,	// LocTypeShipwreck		Also not combat zone per se, but cold and desolate
			0x130EA,	// LocTypeSprigganGrove
			0x130EB,	// LocTypeVampireLair
			0x130EC,	// LocTypeWarlockLair
			0x130ED,	// LocTypeWerewolfLair

			// Dragonborn.esm hostile	29-32
			0x2C969,	// DLC2LocTypeAshSpawn
			0x1DEBD,	// DLC2LocTypePillar
			0x2972E,	// DLC2LocTypeRieklingCamp
			0x2C96A		// LocTypeWerebearLair
		};
		for (u64 i = 0; i < AllHostileLocKwdIDs.size(); ++i) {
			RE::FormID offset = (i < HostileLocKwdDBStart) ? skyrimOffset : dragonbornOffset;
			if (const auto kwd = RE::TESForm::LookupByID<RE::BGSKeyword>(offset + AllHostileLocKwdIDs[i]); kwd) {
				HostileLocKwds[i] = kwd;
			}
			else {
				Log::Critical("Vanilla::FillHostileLocKwdForms: Failed to obtain Keyword from formID {}"sv, u32_xstr(offset + AllHostileLocKwdIDs[i]));
				return false;
			}
		}
		Log::Info("Vanilla::FillHostileLocKwdForms: Successfully obtained {} Keyword forms"sv, HostileLocKwds.size());
		return true;
	}
	static bool GetPeacefulLKwds(RE::FormID skyrimOffset, RE::FormID hearthfiresOffset) noexcept {
		static constexpr u64 PeacefulLocKwdHFStart = 22u;
		static constexpr array<RE::FormID, PeacefulLocKwds.size()> AllPeacefulLocKwdIDs{
			// Skyrim.esm peaceful	0-21
			0x1CD55,	// LocTypeBarracks
			0x1CD57,	// LocTypeCastle
			0x13168,	// LocTypeCity
			0x130DC,	// LocTypeDwelling
			0x18EF0,	// LocTypeFarm
			0x1CD5A,	// LocTypeGuild
			0x39793,	// LocTypeHabitation
			0xA6E84,	// LocTypeHabitationHasInn
			0x16771,	// LocTypeHold
			0x868E2,	// LocTypeHoldCapital
			0x868E1,	// LocTypeHoldMajor
			0x868E3,	// LocTypeHoldMinor
			0x1CB85,	// LocTypeHouse
			0x1CB87,	// LocTypeInn
			0x18EF2,	// LocTypeLumberMill
			0x130E9,	// LocTypeOrcStronghold
			0xFC1A3,	// LocTypePlayerHouse	The city prebuilt ones, not the Hearthfire 3
			0x13167,	// LocTypeSettlement
			0x504F9,	// LocTypeStewardsDwelling
			0x1CB86,	// LocTypeStore
			0x1CD56,	// LocTypeTemple
			0x13166,	// LocTypeTown

			// HearthFires.esm peaceful	22-23
			0x308D,		// BYOHHouseLocationKeyword
			0x4D57		// BYOH_LocTypeHomestead
		};
		for (u64 i = 0u; i < AllPeacefulLocKwdIDs.size(); ++i) {
			RE::FormID offset = (i < PeacefulLocKwdHFStart) ? skyrimOffset : hearthfiresOffset;
			if (const auto kwd = RE::TESForm::LookupByID<RE::BGSKeyword>(offset + AllPeacefulLocKwdIDs[i]); kwd) {
				PeacefulLocKwds[i] = kwd;
			}
			else {
				Log::Critical("Vanilla::FillPeacefulLocKwdForms: Failed to obtain Keyword from formID {}"sv, u32_xstr(offset + AllPeacefulLocKwdIDs[i]));
				return false;
			}
		}
		Log::Info("Vanilla::FillPeacefulLocKwdForms: Successfully obtained {} Keyword forms"sv, PeacefulLocKwds.size());
		return true;
	}
	static bool GetBodyKwds(RE::FormID skyrimOffset) noexcept {
		BodyKwds[0] = RE::TESForm::LookupByID<RE::BGSKeyword>(skyrimOffset + 0x6C0EC);
		BodyKwds[1] = RE::TESForm::LookupByID<RE::BGSKeyword>(skyrimOffset + 0xA8657);
		if (BodyKwds[0] and BodyKwds[1]) {
			Log::Info("Vanilla::FillBodyKwdForms: Successfully obtained 2 Keyword forms"sv);
			return true;
		}
		Log::Critical("Vanilla::FillBodyKwdForms: Failed to obtain ArmorCuirass and/or ClothingBody keywords!"sv);
		return false;
	}
	static bool GetRaces(RE::FormID skyrimOffset, RE::FormID dragonbornOffset) noexcept {
		static constexpr array<RE::FormID, Races.size()> AllRaceIDs {
			// Skyrim.esm
			0xD53,		// DraugrRace

			0x131F0,	// DremoraRace
			0x131F5,	// AtronachFlameRace
			0x131F6,	// AtronachFrostRace
			0x131F7,	// AtronachStormRace

			0x131EF,	// DragonPriestRace

			0x13740,	// ArgonianRace
			0x13741,	// BretonRace
			0x13742,	// DarkElfRace
			0x13743,	// HighElfRace
			0x13744,	// ImperialRace
			0x13745,	// KhajiitRace
			0x13746,	// NordRace
			0x13747,	// OrcRace
			0x13748,	// RedguardRace
			0x13749,	// WoodElfRace

			// Dragonborn.esm
			0x14495,	// DLC2LurkerRace
			0x1DCB9,	// DLC2SeekerRace
		};
		static constexpr u64 DBStart = AllRaceIDs.size() - 2;
		for (u64 idx{ 0 }; idx < Races.size(); ++idx) {
			Races[idx] = RE::TESForm::LookupByID<RE::TESRace>((idx < DBStart ? skyrimOffset : dragonbornOffset) + AllRaceIDs[idx]);
			if (!Races[idx]) {
				Log::Critical("Vanilla::FillRaceForms: Failed to obtain Race from formID {}"sv, u32_xstr(AllRaceIDs[idx]));
				return false;
			}
		}
		Log::Info("Vanilla::FillRaceForms: Successfully obtained {} Race forms"sv, Races.size());
		return true;
	}
	static bool GetFactions(RE::FormID skyrimOffset) noexcept {
		static constexpr array<RE::FormID, Factions.size()> AllFactionIDs {
			// Skyrim.esm
			0x13,		// CreatureFaction
		};
		for (u64 idx{ 0 }; idx < Factions.size(); ++idx) {
			Factions[idx] = RE::TESForm::LookupByID<RE::TESFaction>(skyrimOffset + AllFactionIDs[idx]);
			if (!Factions[idx]) {
				Log::Critical("Vanilla::FillFactionForms: Failed to obtain Faction from formID {}"sv, u32_xstr(AllFactionIDs[idx]));
				return false;
			}
		}
		Log::Info("Vanilla::FillFactionForms: Successfully obtained {} Faction forms"sv, Factions.size());
		return true;
	}
	static bool GetRaceKwds(RE::FormID skyrimOffset) noexcept {
		static constexpr array<RE::FormID, RaceKwds.size()> AllRaceKwdIDs {
			0x13794,	// ActorTypeNPC
			0x13795,	// ActorTypeCreature
			0x13797,	// ActorTypeDaedra
			0x13798,	// ActorTypeAnimal
			0x1397A,	// ActorTypeDwarven
			0x35D59		// ActorTypeDragon
		};
		for (u64 i = 0; i < AllRaceKwdIDs.size(); ++i) {
			if (const auto kwd = RE::TESForm::LookupByID<RE::BGSKeyword>(skyrimOffset + AllRaceKwdIDs[i]); kwd) {
				RaceKwds[i] = kwd;
			}
			else {
				Log::Critical("Vanilla::FillRaceKwdForms: Failed to obtain Keyword from formID {}"sv, u32_xstr(skyrimOffset + AllRaceKwdIDs[i]));
				return false;
			}
		}
		Log::Info("Vanilla::FillRaceKwdForms: Successfully obtained {} Keyword forms"sv, RaceKwds.size());
		return true;
	}
	bool FillForms() noexcept {
		static constexpr array<string_view, 5> baseModNames{
			"Skyrim.esm",			// [0]
			"Update.esm"sv,			// [1]
			"Dawnguard.esm"sv,		// [2]
			"HearthFires.esm"sv,	// [3]
			"Dragonborn.esm"sv		// [4]
		};
		array<RE::FormID, 5> offs{};
		if (!GetPluginFormIDOffsets(offs, baseModNames)) {
			Log::Error("Vanilla::FillForms: Failed to get FormID offs!"sv);
		} else if (PlayerPtr = RE::PlayerCharacter::GetSingleton(); !PlayerPtr) {
			Log::Critical("Vanilla::FillForms: Failed to find Player form!"sv);
		} else if (PlayerHandle_ = RE::PlayerCharacter::GetSingleton(); !PlayerHandle_) {
			Log::Critical("Vanilla::FillForms: Failed to find Player handle!"sv);
		} else if (GoldPtr = RE::TESForm::LookupByID<RE::TESObjectMISC>(0xF); !GoldPtr) {
			Log::Critical("Vanilla::FillForms: Failed to find Gold form!"sv);
		} else if (GameDaysPassedPtr = RE::TESForm::LookupByID<RE::TESGlobal>(0x39); !GameDaysPassedPtr) {
			Log::Critical("Vanilla::FillForms: Failed to find GameDaysPassed form!"sv);
		} else if (!GetHostileLKwds(offs[0], offs[4]) or !GetPeacefulLKwds(offs[0], offs[3]) or !GetBodyKwds(offs[0]) or !GetRaces(offs[0], offs[4]) or !GetFactions(offs[0]) or !GetRaceKwds(offs[0])) {
			Log::Critical("Vanilla::FillForms: Failed to initialize data!"sv);
		} else {
			Log::Info("Vanilla::FillForms: Initialized data successfully"sv);
			return true;
		}
		return false;
	}
	
}

