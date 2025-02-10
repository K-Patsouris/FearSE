#include "HurdlesForms.h"
#include "Logger.h"
#include "Utils/GameDataUtils.h"
#include "Utils/PrimitiveUtils.h"



namespace Hurdles {
	using namespace LazyVector;
	using GameDataUtils::GetPluginFormIDOffsets;
	using PrimitiveUtils::u32_xstr;

	
	array<RE::BGSKeyword*, static_cast<size_t>(KWD::Total)> KEYWORDS;
	array<RE::EffectSetting*, static_cast<size_t>(EFF::Total)> EFFECTS;


	lazy_vector<KWD> KwdsToIdx(const lazy_vector<RE::BGSKeyword*>& kwds) {
		lazy_vector<KWD> result{};
		if (result.reserve(KEYWORDS.size())) {
			for (auto it = kwds.begin(), end = kwds.end(); it != end; ++it) { // Gonna go on a limb and assume most things have less than 27 keywords
				for (auto IT = KEYWORDS.begin(), END = KEYWORDS.end(); IT != END; ++IT) {
					if (*IT == *it) {
						result.append(static_cast<KWD>(IT - KEYWORDS.begin())); // Cast is noop
						break;
					}
				}
			}
		}
		return result;
	}


	RE::BGSKeyword* Keyword(const KWD idx) noexcept { return KEYWORDS[static_cast<size_t>(idx)];  }

	RE::EffectSetting* Effect(const EFF idx) noexcept { return EFFECTS[static_cast<size_t>(idx)];  }


	static bool FillKeywordForms(RE::FormID aOffset) noexcept {
		static constexpr array<RE::FormID, KEYWORDS.size()> AllKeywordIDs{
			// All keywords are in "Assets.esm"
			0x3DF9,		// Hurdle_1
			0x3DF8,		// Hurdle_2
			0x11B1A,	// Hurdle_3
			0x3330,		// Hurdle_4
			0x3DF7,		// Hurdle_5
			0x27F29,	// Hurdle_6
			0x27F28,	// Hurdle_7
			0xCA39,		// Hurdle_8
			0x23E70,	// Hurdle_9
			0x7EB8,		// Hurdle_10
			0x3DFA,		// Hurdle_11
			0x2AFA1,	// Hurdle_12
			0x1F306,	// Hurdle_13
			0xCA3A,		// Hurdle_14
			0x2C531,	// Hurdle_15
			0x17C43,	// Hurdle_16
			0x3331,		// Hurdle_17
			0x1DD7C,	// Hurdle_18
			0x1DD7D,	// Hurdle_19
			0x2AFA2,	// Hurdle_20
			0x2AFA3,	// Hurdle_21
			0x7EB9,		// Hurdle_22
			0x3894,		// Hurdle_23
			0x63CA,		// Hurdle_24
			0xFAC9,		// Hurdle_25
			0xFACA,		// Hurdle_26
			0xFACB,		// Hurdle_27
		};
		for (u64 idx{ 0 }; idx < KEYWORDS.size(); ++idx) {
			KEYWORDS[idx] = RE::TESForm::LookupByID<RE::BGSKeyword>(aOffset + AllKeywordIDs[idx]);
			if (!KEYWORDS[idx]) {
				Log::Critical("Hurdles::FillKeywordForms: Failed to obtain Keyword from formID {}"sv, u32_xstr(AllKeywordIDs[idx]));
				return false;
			}
		}
		Log::Info("Hurdles::FillKeywordForms: Successfully obtained {} Keyword forms"sv, KEYWORDS.size());
		return true;
	}
	static bool FillEffectForms(RE::FormID effOffset) noexcept {
		static constexpr array<RE::FormID, EFFECTS.size()> AllKeywordIDs{
			// Effects are in "Effects.esm"
			0x48596,	// BadEff_1
			0x4859A		// BadEff_2
		};
		for (u64 idx{ 0 }; idx < EFFECTS.size(); ++idx) {
			EFFECTS[idx] = RE::TESForm::LookupByID<RE::EffectSetting>(effOffset + AllKeywordIDs[idx]);
			if (!EFFECTS[idx]) {
				Log::Critical("Hurdles::FillEffectForms: Failed to obtain Effect from formID {}"sv, u32_xstr(AllKeywordIDs[idx]));
				return false;
			}
		}
		Log::Info("Hurdles::FillEffectForms: Successfully obtained {} Effect forms"sv, KEYWORDS.size());
		return true;
	}
	bool FillForms() noexcept {
		static constexpr array<string_view, 2> baseModNames{
			"Assets.esm",			// [0]
			"Effects.esm"		// [1]
		};
		array<RE::FormID, 2> offsets{};
		if (!GetPluginFormIDOffsets(offsets, baseModNames)) {
			Log::Critical("Hurdles::FillForms: Failed obtain plugin offsets!"sv);
		} else if (!FillKeywordForms(offsets[0]) or !FillEffectForms(offsets[1])) {
			Log::Critical("Hurdles::FillForms: Failed to initialize data!"sv);
		} else {
			Log::Info("Hurdles::FillForms: Initialized data successfully"sv);
			return true;
		}
		return false;
	}

}
