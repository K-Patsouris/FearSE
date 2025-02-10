#pragma once
#include "Common.h"
#include "Types/LazyVector.h"


namespace GameDataUtils {
	using LazyVector::lazy_vector;


	// Date
	float DaysPassed() noexcept;

	// Scan
	struct SkipFlags {
		SkipFlags() noexcept
			: skip_dead(true)
			, skip_childs(true)
			, skip_disabled(true)
			, skip_deleted(true)
		{}
		SkipFlags(bool skip_dead, bool skip_childs, bool skip_disabled, bool skip_deleted) noexcept
			: skip_dead(skip_dead)
			, skip_childs(skip_childs)
			, skip_disabled(skip_disabled)
			, skip_deleted(skip_deleted)
		{}
		bool skip_dead = true, skip_childs = true, skip_disabled = true, skip_deleted = true;
	};
	// Result will contain the player's RE::ActorPtr first, and will not contain nullptr.
	lazy_vector<RE::ActorPtr> GetHighActors(const SkipFlags flags) noexcept;
	// Result will contain the player's RE::ActorPtr first, and will not contain nullptr.
	lazy_vector<RE::ActorPtr> GetHighValidActors() noexcept;


	RE::SEX GetSex(const RE::Actor* act) noexcept;

	bool RemoveGold(RE::Actor* act, const i32 amount) noexcept;


	// Spells

	// Expects valid act, spell
	bool AddSpell(RE::Actor* act, RE::SpellItem* spell, const bool silent) noexcept;
	// Expects valid act, spell
	bool CastSpellImmediate(RE::Actor* act, RE::SpellItem* spell, const float mag, RE::Actor* blame) noexcept;
	// Expects valid act, spell
	bool DispelSpell(RE::Actor* act, RE::SpellItem* spell) noexcept;
	// Expects valid spell, index, and targeted baseEffect
	bool SetNthEffectDescription(RE::SpellItem* spell, const u32 index, const string& desc) noexcept;
	// Expects valid spell, index, and targeted effectItem
	void SetNthEffectMagnitude(RE::SpellItem* spell, const u32 index, const float mag) noexcept;
	// Expects valid spell, indices [idx_first, idx_last], and targeted effectItems
	void SetEffectsMagnitude(RE::SpellItem* spell, const u32 idx_first, const u32 idx_last, const float mag) noexcept;
	// Expects valid act
	u32 CountSpellType(RE::Actor* act, const RE::MagicSystem::SpellType type) noexcept;

	// AVs

	float GetHealthMod(RE::Actor* act, const RE::ACTOR_VALUE_MODIFIER mod) noexcept;
	float GetHealthMax(RE::Actor* act) noexcept;
	float GetHealthRatio(RE::Actor* act) noexcept;
	void ModActorValue(RE::Actor* act, const RE::ActorValue av, const RE::ACTOR_VALUE_MODIFIER mod, const float val) noexcept;

	// Plugin files

	bool GetPluginFormIDOffset(RE::FormID& formID_out, string_view pluginName, RE::TESDataHandler* dataHandler = nullptr) noexcept;
	template<u64 size>
	bool GetPluginFormIDOffsets(array<RE::FormID, size>& formIDs_out, const array<string_view, size>& pluginNames, RE::TESDataHandler* dataHandler = nullptr) noexcept {
		if (!dataHandler) {
			dataHandler = RE::TESDataHandler::GetSingleton();
		}
		if (!dataHandler) {
			return false;
		}
		for (u64 i = 0; i < size; ++i) {
			try {
				if (const auto file = dataHandler->LookupModByName(pluginNames[i]); file and file->compileIndex != 0xFF) { // Up to FE, which is for light plugins. FF is reserved by the game.
					// Copied from TESDataHandler		LookupModByName() finds both regular and light plugins
					// compileIndex is the load order priority. For regular plugins this goes up to FD (254 limit). For light plugins it is always FE.
					// smallFileCompileIndex is the light plugin load order priority. For regular plugins it is always 0. For light plugins it goes up to FFF (4096 limit).
					// 	So, the max offset calcualted for regular plugins is FD00 0000, and for light plugins FEFF F000.
					// 		(each 4 binary position shifts is 1 hex position shift)
					formIDs_out[i] = (file->compileIndex << 24) + (file->smallFileCompileIndex << 12);
				}
				else {
					return false;
				}
			}
			catch (...) { return false; }
		}
		return true;
	}

}
