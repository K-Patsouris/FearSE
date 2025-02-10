#include "FearForms.h"
#include "Logger.h"
#include "Utils/GameDataUtils.h"
#include "Utils/PrimitiveUtils.h"


namespace Fear {
	using namespace LazyVector;
	using PrimitiveUtils::u32_xstr;

	std::atomic<bool> formsFilled{ false };

	array<RE::BGSKeyword*, static_cast<size_t>(KWD::Total)> KEYWORDS{};
	array<RE::TESQuest*, static_cast<size_t>(QST::Total)> QUESTS{};
	array<RE::TESFaction*, static_cast<size_t>(FAC::Total)> FACTIONS{};


	bool FormsFilled() noexcept { return formsFilled.load(std::memory_order_acquire); }

	lazy_vector<KWD> KwdsToIdx(const lazy_vector<RE::BGSKeyword*>& kwds) {
		lazy_vector<KWD> result{};
		if (result.reserve(KEYWORDS.size())) {
			for (auto IT = KEYWORDS.begin(), END = KEYWORDS.end(); IT != END; ++IT) { // Not worth trying to quit early with just 5 iterations
				for (auto it = kwds.begin(), end = kwds.end(); it != end; ++it) {
					if (*IT == *it) {
						result.append(static_cast<KWD>(IT - KEYWORDS.begin())); // Cast is noop
						break;
					}
				}
			}
		}
		return result;
	}

	RE::BGSKeyword* Keyword(const KWD idx) noexcept { return KEYWORDS[static_cast<size_t>(idx)]; }
	RE::TESQuest* Quest(const QST idx) noexcept { return QUESTS[static_cast<size_t>(idx)]; }
	RE::TESFaction* Faction(const FAC idx) noexcept { return FACTIONS[static_cast<size_t>(idx)]; }



	static bool FillKeywordForms(RE::FormID fearOffset) noexcept {
		static constexpr array<RE::FormID, KEYWORDS.size()> AllKeywordIDs{
			0x8E854,	// MidQual
			0x8E853,	// LowQual
			0x8E855,	// Torn
			0x8C7F6,	// NonArmor
			0x8F326		// NoSoles
		};
		for (u64 idx{ 0 }; idx < KEYWORDS.size(); ++idx) {
			KEYWORDS[idx] = RE::TESForm::LookupByID<RE::BGSKeyword>(fearOffset + AllKeywordIDs[idx]);
			if (!KEYWORDS[idx]) {
				Log::Critical("Fear::FillKeywordForms: Failed to obtain Keyword from formID {}"sv, u32_xstr(AllKeywordIDs[idx]));
				return false;
			}
		}
		Log::Info("Fear::FillKeywordForms: Successfully obtained {} Keyword forms"sv, KEYWORDS.size());
		return true;
	}
	static bool FillQuestForms(RE::FormID fearOffset) noexcept {
		static constexpr array<RE::FormID, QUESTS.size()> AllQuestIDs {
			0x4290F,	// Main
		};
		for (u64 idx{ 0 }; idx < QUESTS.size(); ++idx) {
			QUESTS[idx] = RE::TESForm::LookupByID<RE::TESQuest>(fearOffset + AllQuestIDs[idx]);
			if (!QUESTS[idx]) {
				Log::Critical("Fear::FillQuestForms: Failed to obtain Quest from formID {}"sv, u32_xstr(AllQuestIDs[idx]));
				return false;
			}
		}
		Log::Info("Fear::FillQuestForms: Successfully obtained {} Quest forms"sv, QUESTS.size());
		return true;
	}
	static bool FillFactionForms(RE::FormID fearOffset) noexcept {
		static constexpr array<RE::FormID, FACTIONS.size()> AllFactionIDs{
			0x3FC36,	// Fear
			0x8FEAA,	// Unused_1
			0x8FEAB,	// Unused_2
			0x8FEA3,	// Thrillseeking
			0x8FEAC,	// Unused_3
			0x8FEAD,	// FearsFemale
			0x8FEAE,	// FearsMale
			0x77F87,	// Naked
			0x79A72,	// Unused_4
			0x25837,	// Deprecated_1
			0x7649B,	// Deprecated_2
			0x713DA,	// Thrillseeker
			0x6E8C6,	// FearBlocked
			0x7649C,	// FearLocked
			0x7C025		// Deprecated_3
		};
		for (u64 idx{ 0 }; idx < FACTIONS.size(); ++idx) {
			FACTIONS[idx] = RE::TESForm::LookupByID<RE::TESFaction>(fearOffset + AllFactionIDs[idx]);
			if (!FACTIONS[idx]) {
				Log::Critical("Fear::FillFactionForms: Failed to obtain Faction from formID {}"sv, PrimitiveUtils::u32_xstr(AllFactionIDs[idx]));
				return false;
			}
		}
		Log::Info("Fear::FillFactionForms: Successfully obtained {} Faction forms"sv, FACTIONS.size());
		return true;
	}
	bool FillForms() noexcept {
		if (formsFilled.load()) {
			return true;
		}
		formsFilled.store(false);
		RE::FormID fearOffset;
		if (!GameDataUtils::GetPluginFormIDOffset(fearOffset, "FearSE.esm")) {
			Log::Critical("Fear::FillForms: Failed to get plugin offset!"sv);
		} else if (!FillKeywordForms(fearOffset) or !FillQuestForms(fearOffset) or !FillFactionForms(fearOffset)) {
			Log::Critical("Fear::FillForms: Failed to initialize data!"sv);
		} else {
			formsFilled.store(true);
			Log::Info("Fear::FillForms: Initialized data successfully"sv);
		}
		return formsFilled.load();
	}

}
