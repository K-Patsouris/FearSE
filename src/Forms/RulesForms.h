#pragma once
#include "Common.h"

namespace PlayerRules {
	enum class QST : size_t {
		MCM = 0,
		Main = 1,
		Libs = 2,
		Blocked = 3,

		Total
	};
	enum class FAC : size_t {
		TrainingFaction = 0,
		AddictionFaction = 1,
		PermanentFaction = 2,

		Total
	};
	enum class SPL : size_t {
		PlayerRules = 0,
		CrestLink = 1,
		Deprecated_1 = 2,
		Deprecated_2 = 3,
		RecentDodges = 4,
		DebuffMSCW = 5, 		// 	Ability	1 effect	SpeedMult/CarryWeight debuff. mag=50 => -50% speed/-100 carry weight
		DebuffResist = 6, 		// 	Ability	1 effect	Armor/MagicResist debuff. mag=50 => -50% magic resist/-250 armor
		DebuffPower = 7, 		// 	Ability	4 effects	0:Weapon, 1:Destr/Conj, 2:Alt/Ill, 3:Resto power debuff. mag0=0.5 => -50% weapon power, mag1-3=50 => -50% <skill> power
		DebuffCrafts = 8, 		// 	Ability	2 effects	0:Smith/Alch, 1:Ench. mag0-1=50 => -50% smithing/alchemy/enchanting power

		Total
	};


	[[nodiscard]] RE::TESQuest* Quest(const QST idx) noexcept;
	[[nodiscard]] RE::TESFaction* Faction(const FAC idx) noexcept;
	[[nodiscard]] RE::SpellItem* Spell(const SPL idx) noexcept;


	[[nodiscard]] bool FillForms() noexcept;


}
