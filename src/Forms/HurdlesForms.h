#pragma once
#include "Common.h"
#include "Types/LazyVector.h"

namespace Hurdles {
	// Placeholder Hurdle IDs stuff. Some used to carry special meaning. Atm pointless. Redo once Hurdles actually implemented in game assets.
	enum class KWD : size_t {
		Hurdle_1 = 0,
		Hurdle_2 = 1,
		Hurdle_3 = 2,
		Hurdle_4 = 3,
		Hurdle_5 = 4,
		Hurdle_6 = 5,
		Hurdle_7 = 6,
		Hurdle_8 = 7,
		Hurdle_9 = 8,
		Hurdle_10 = 9,
		Hurdle_11 = 10,
		Hurdle_12 = 11,
		Hurdle_13 = 12,
		Hurdle_14 = 13,
		Hurdle_15 = 14,
		Hurdle_16 = 15,
		Hurdle_17 = 16,
		Hurdle_18 = 17,
		Hurdle_19 = 18,
		Hurdle_20 = 19,
		Hurdle_21 = 20,
		Hurdle_22 = 21,
		Hurdle_23 = 22,
		Hurdle_24 = 23,
		Hurdle_25 = 24,
		Hurdle_26 = 25,
		Hurdle_27 = 26,

		Total
	};
	enum class EFF : size_t {
		BadEff_1 = 0,
		BadEff_2 = 1,

		Total
	};



	[[nodiscard]] LazyVector::lazy_vector<KWD> KwdsToIdx(const LazyVector::lazy_vector<RE::BGSKeyword*>& kwds);

	[[nodiscard]] RE::BGSKeyword* Keyword(const KWD idx) noexcept;


	[[nodiscard]] RE::EffectSetting* Effect(const EFF idx) noexcept;



	[[nodiscard]] bool FillForms() noexcept;


}

