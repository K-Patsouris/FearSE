#include "RulesForms.h"
#include "Logger.h"
#include "Utils/GameDataUtils.h"
#include "Utils/PrimitiveUtils.h"


namespace PlayerRules {
	using GameDataUtils::GetPluginFormIDOffset;
	using PrimitiveUtils::u32_xstr;


	array<RE::TESQuest*, static_cast<size_t>(QST::Total)> QUESTS{};
	array<RE::TESFaction*, static_cast<size_t>(FAC::Total)> FACTIONS{};
	array<RE::SpellItem*, static_cast<size_t>(SPL::Total)> SPELLS{};



	RE::TESQuest* Quest(const QST idx) noexcept { return QUESTS[static_cast<size_t>(idx)]; }
	RE::TESFaction* Faction(const FAC idx) noexcept { return FACTIONS[static_cast<size_t>(idx)]; }
	RE::SpellItem* Spell(const SPL idx) noexcept { return SPELLS[static_cast<size_t>(idx)]; }



	static bool FillQuestForms(RE::FormID prOffset) noexcept {
		static constexpr array<RE::FormID, QUESTS.size()> AllQuestIDs{
			0xD64,	// MCM
			0xD65,	// Main
			0xD8A,	// Libs
			0x290F	// Blocked
		};
		for (u64 idx{ 0 }; idx < QUESTS.size(); ++idx) {
			QUESTS[idx] = RE::TESForm::LookupByID<RE::TESQuest>(prOffset + AllQuestIDs[idx]);
			if (!QUESTS[idx]) {
				Log::Critical("PlayerRules::FillQuestForms: Failed to obtain Quest from formID {}"sv, u32_xstr(AllQuestIDs[idx]));
				return false;
			}
		}
		Log::Info("PlayerRules::FillQuestForms: Successfully obtained {} Quest forms"sv, QUESTS.size());
		return true;
	}
	static bool FillFactionForms(RE::FormID prOffset) noexcept {
		static constexpr array<RE::FormID, FACTIONS.size()> AllFactionIDs{
			0x1383,		// TrainingFaction
			0x1645B,	// AddictionFaction
			0x1645C,	// PermanentFaction
		};
		for (u64 idx{ 0 }; idx < FACTIONS.size(); ++idx) {
			FACTIONS[idx] = RE::TESForm::LookupByID<RE::TESFaction>(prOffset + AllFactionIDs[idx]);
			if (!FACTIONS[idx]) {
				Log::Critical("PlayerRules::FillFactionForms: Failed to obtain Faction from formID {}"sv, u32_xstr(AllFactionIDs[idx]));
				return false;
			}
		}
		Log::Info("PlayerRules::FillFactionForms: Successfully obtained {} Faction forms"sv, FACTIONS.size());
		return true;
	}
	static bool FillSpellForms(RE::FormID prOffset) noexcept {
		static constexpr array<RE::FormID, SPELLS.size()> AllSpellIDs{
			0xD69,		// PlayerRules
			0xD70,		// CrestLink
			0xE01,
			0xE02,
			0xE05,		// RecentDodges
			0x27807, 	// DebuffMSCW
			0x27808, 	// DebuffResist
			0x2780A, 	// DebuffPower
			0x2781F, 	// DebuffCrafts
		};
		for (u64 idx{ 0 }; idx < SPELLS.size(); ++idx) {
			SPELLS[idx] = RE::TESForm::LookupByID<RE::SpellItem>(prOffset + AllSpellIDs[idx]);
			if (!SPELLS[idx]) {
				Log::Critical("PlayerRules::FillSpellForms: Failed to obtain Spell from formID {}"sv, u32_xstr(AllSpellIDs[idx]));
				return false;
			}
		}
		Log::Info("PlayerRules::FillSpellForms: Successfully obtained {} Spell forms"sv, SPELLS.size());
		return true;
	}
	bool FillForms() noexcept {
		RE::FormID prOffset{};
		if (!GetPluginFormIDOffset(prOffset, "PlayerRules.esm")) {
			Log::Critical("PlayerRules::FillForms: failed to get plugin offset!"sv);
		} else if (!FillQuestForms(prOffset) or !FillFactionForms(prOffset) or !FillSpellForms(prOffset)) {
			Log::Critical("PlayerRules::FillForms: failed to initialize form data!"sv);
		} else {
			Log::Info("PlayerRules::FillForms: initialized form data successfully"sv);
			return true;
		}
		return false;
	}
}
