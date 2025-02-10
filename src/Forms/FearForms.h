#pragma once
#include "Common.h"
#include "Types/LazyVector.h"


namespace Fear {
	enum class KWD : size_t {
		MidQual = 0,
		LowQual = 1,
		Torn = 2,
		NonArmor = 3,
		NoSoles = 4,

		Total
	};
	enum class QST : size_t {
		Main = 0,

		Total
	};
	enum class FAC : size_t {
		Fear = 0,
		Unused_1 = 1,
		Unused_2 = 2,
		Thrillseeking = 3,
		Unused_3 = 4,
		FearsFemale = 5,
		FearsMale = 6,
		Naked = 7,
		Unused_4 = 8,
		Deprecated_1 = 9,
		Deprecated_2 = 10,
		Thrillseeker = 11,
		FearBlocked = 12,
		FearLocked = 13,
		Deprecated_3 = 14,

		Total
	};

	[[nodiscard]] bool FormsFilled() noexcept;


	[[nodiscard]] LazyVector::lazy_vector<KWD> KwdsToIdx(const LazyVector::lazy_vector<RE::BGSKeyword*>& kwds);


	[[nodiscard]] RE::BGSKeyword* Keyword(const KWD idx) noexcept;
	[[nodiscard]] RE::TESQuest* Quest(const QST idx) noexcept;
	[[nodiscard]] RE::TESFaction* Faction(const FAC idx) noexcept;


	[[nodiscard]] bool FillForms() noexcept;

}
