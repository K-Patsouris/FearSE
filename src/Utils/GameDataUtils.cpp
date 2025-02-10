#include "GameDataUtils.h"
#include "Forms/VanillaForms.h"


namespace GameDataUtils {
	
	// Date
	float DaysPassed() noexcept { return Vanilla::GameDaysPassed()->value; }

	// Scan
	lazy_vector<RE::ActorPtr> GetHighActors(const SkipFlags flags) noexcept {
		if (const RE::ProcessLists* lists = RE::ProcessLists::GetSingleton(); lists) {
			lazy_vector<RE::ActorPtr> result{};
			if (!result.reserve(1ui64 + lists->highActorHandles.size())) {
				return {};
			}
			result.append(RE::PlayerCharacter::GetSingleton());
			for (const auto handle : lists->highActorHandles) {
				if (const auto actor = handle.get();
					actor
					and (!flags.skip_dead or !actor->IsDead())
					and (!flags.skip_childs or !actor->IsChild())
					and (!flags.skip_disabled or !actor->IsDisabled())
					and (!flags.skip_deleted or !actor->IsDeleted()))
				{
					result.append(actor);
				}
			}
			return result;
		}
		return {};
	}

	lazy_vector<RE::ActorPtr> GetHighValidActors() noexcept {
		if (const RE::ProcessLists* lists = RE::ProcessLists::GetSingleton(); lists) {
			lazy_vector<RE::ActorPtr> result{};
			if (!result.reserve(1ui64 + lists->highActorHandles.size())) {
				return {};
			}
			result.append(RE::PlayerCharacter::GetSingleton());
			for (const auto handle : lists->highActorHandles) {
				if (const auto actor = handle.get(); actor and !actor->IsDead() and !actor->IsChild() bitand !actor->IsDisabled() bitand !actor->IsDeleted()) {
					result.append(actor);
				}
			}
			return result;
		}
		return {};
	}



	RE::SEX GetSex(const RE::Actor* act) noexcept {
		const auto base = act->GetActorBase();
		return base ? base->GetSex() : RE::SEX::kNone;
	}

	bool RemoveGold(RE::Actor* act, const i32 amount) noexcept {
		if (amount > 0) {
			act->RemoveItem(Vanilla::Gold(), amount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			return true;
		}
		return false;
	}


	// Spells

	// For abilities:	Snapshots effect magnitude at time of call. Changes to effect description are still seen next time magic menu opens.
	// 					Magnitude is saved and loaded as it was snapshotted. Description resets to record value.
	// 					It also seems to be always silent. Just call act->AddSpell(spell) instead. Use this only for adding castables.
	bool AddSpell(RE::Actor* act, RE::SpellItem* spell, const bool silent) noexcept {
		if (!silent) {
			return act->AddSpell(spell);
		}
		// Very hacky way to not display the notification but it gets the job done
		RE::BSFixedString oldname{ spell->fullName };
		spell->fullName = "";
		bool result = act->AddSpell(spell);
		spell->fullName = std::move(oldname);
		return result;
	}

	// Forces magnitude of ALL magic effects of spell to the float passed UNLESS that is 0.0f.
	bool CastSpellImmediate(RE::Actor* act, RE::SpellItem* spell, const float mag, RE::Actor* blame) noexcept {
		if (auto caster = act->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
			caster->CastSpellImmediate(spell, true, act, 1.0f, false, mag, blame);
			return true;
		}
		return false;
	}
	
	bool DispelSpell(RE::Actor* act, RE::SpellItem* spell) noexcept {
		auto handle = act->GetHandle(); // Seems irrelevant. DispelEffect seems to work just fine even if a handle is created and passed on the spot. Maybe the handle is supposed to be a dispeller?
		return act->DispelEffect(spell, handle);
	}

	bool SetNthEffectDescription(RE::SpellItem* spell, const u32 index, const string& desc) noexcept {
		spell->effects[index]->baseEffect->magicItemDescription = desc;
		return true;
	}

	void SetNthEffectMagnitude(RE::SpellItem* spell, const u32 index, const float mag) noexcept { spell->effects[index]->effectItem.magnitude = mag; }
	void SetEffectsMagnitude(RE::SpellItem* spell, const u32 idx_first, const u32 idx_last, const float mag) noexcept {
		for (u32 idx = idx_first; idx <= idx_last; ++idx) {
			spell->effects[idx]->effectItem.magnitude = mag;
		}
	}

	u32 CountSpellType(RE::Actor* act, const RE::MagicSystem::SpellType type) noexcept {
		u32 result = 0;
		if (auto effects = act->GetActiveEffectList(); effects) {
			for (const auto* effect : *effects) {
				if (effect and effect->spell and effect->spell->GetSpellType() == type) {
					++result;
				}
			}
		}
		return result;
	}
	
	/*Dispel specific active effects
	auto actives = act->GetActiveEffectList();
	if (!actives) {
		Util::PrintToConsole("Actives invalid!");
		return false;
	}

	u32 counter = 0;
	auto cur = actives->begin();
	auto end = actives->end();
	while (cur != end) {
		counter++;
		if (counter >= 100u) {
			Util::PrintToConsole("Failsafe triggering because loop iterated 100 times. Exiting...");
			break;
		}

		if (auto item = *cur; item ** item->spell == spell) {
			Util::PrintToConsole("Found effect of spell in interation " + to_string(counter) + "! Dispelling...");
			msg = act->HasMagicEffect(item->effect->baseEffect) ? "Actor had effect, " : "Actor did not have effect, ";
			item->Dispel(true);
			msg += act->HasMagicEffect(item->effect->baseEffect) ? "Dispel failed" : "Dispel success";
			Util::PrintToConsole(msg);
		}

		// OR

		if (auto item = *cur; item ** item->effect->baseEffect == effect0) {
			Util::PrintToConsole("Found effect0 in interation " + to_string(counter) + "! Dispelling...");
			item->Dispel(true);
			msg = act->HasMagicEffect(effect0) ? "Dispel failed" : "Dispel success";
			Util::PrintToConsole(msg);
			break;
		}

		++cur;
	}*/


	// AVs
	float GetHealthMod(RE::Actor* act, const RE::ACTOR_VALUE_MODIFIER mod) noexcept { return act->healthModifiers.modifiers[static_cast<u32>(mod)]; }
	float GetHealthMax(RE::Actor* act) noexcept { return act->healthModifiers.modifiers[RE::ACTOR_VALUE_MODIFIER::kTemporary] + act->GetPermanentActorValue(RE::ActorValue::kHealth); }
	float GetHealthRatio(RE::Actor* act) noexcept {
		// (base+perm+temp+dmg) / (base+perm+temp)
		return act->GetActorValue(RE::ActorValue::kHealth) / (act->healthModifiers.modifiers[RE::ACTOR_VALUE_MODIFIER::kTemporary] + act->GetPermanentActorValue(RE::ActorValue::kHealth));
	}
	void ModActorValue(RE::Actor* act, const RE::ActorValue av, const RE::ACTOR_VALUE_MODIFIER mod, const float val) noexcept { act->RestoreActorValue(mod, av, val); }



	// Plugin files
	bool GetPluginFormIDOffset(RE::FormID& formID_out, std::string_view pluginName, RE::TESDataHandler* dataHandler) noexcept {
		if (!dataHandler) {
			dataHandler = RE::TESDataHandler::GetSingleton();
		}
		if (!dataHandler) {
			return false;
		}
		if (const auto file = dataHandler->LookupModByName(pluginName); file and file->compileIndex != 0xff) {
			// Copied from TESDataHandler		LookupModByName() finds both regular and light plugins
			// compileIndex is the load order priority. For regular plugins this goes up to FD (254 limit). For light plugins it is always FE.
			// smallFileCompileIndex is the light plugin load order priority. For regular plugins it is always 0. For light plugins it goes up to FFF (4096 limit).
			// 	So, the max offset calcualted for regular plugins is FD00 0000, and for light plugins FEFF F000.
			// 		(each 4 binary position shifts is 1 hex position shift)
			formID_out = (file->compileIndex << 24) + (file->smallFileCompileIndex << 12);
			return true;
		}
		return false;
	}

	
}

