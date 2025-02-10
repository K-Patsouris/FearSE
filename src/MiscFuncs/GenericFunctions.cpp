#include "GenericFunctions.h"
#include "Common.h"
#include "Logger.h"
#include "Utils/PrimitiveUtils.h"
#include "Utils/RNG.h"
#include "Utils/StringUtils.h"


namespace Functions {
	using PrimitiveUtils::to_u64;


	bool WornHasKeywords(StaticFunc, RE::Actor* act, vector<RE::BGSKeyword*> kwds) {
		if (!act || kwds.empty())
			return false;
		const auto inv = act->GetInventory();
		for (const auto& [item, data] : inv) {
			if (item->Is(RE::FormType::LeveledItem)) // PO3 has this check. Probably for a reason.
				continue;
			const auto& [count, entry] = data;
			if (count > 0 && entry->IsWorn() && entry->GetObject()->HasKeywordInArray(kwds, false)) // Passing true will return false when it finds the first not-had kwd BUT will still return true if the FIRST kwd in the array is had
				return true;
		}
		return false;
	}

	deque<bool> HasKeywords(StaticFunc, RE::TESForm* form, RE::BGSListForm* formlist) {
		if (!form || !formlist)
			return deque<bool>{};
		const auto kwdForm = form->As<RE::BGSKeywordForm>();
		if (!kwdForm)
			return deque<bool>{};
		deque<bool> result(formlist->forms.size(), false);
		for (u32 i = 0; i < result.size(); ++i) {
			if (formlist->forms[i])
				result[i] = kwdForm->HasKeywordID(formlist->forms[i]->formID);
		}
		return result;
	}

	vector<RE::TESObjectARMO*> GetHeldArmorsOfSlot(StaticFunc, RE::TESObjectREFR* ref, i32 slot) {
		if (!ref)
			return vector<RE::TESObjectARMO*>{};
		vector<RE::TESObjectARMO*> result{};
		const auto inv = ref->GetInventory([](RE::TESBoundObject& obj) { return obj.Is(RE::FormType::Armor); });
		result.reserve(inv.size());
		u64 equippedIdx = 0, currentIdx = 0;
		for (const auto& [item, data] : inv) {
			if (data.first <= 0)
				continue;
			if (const auto armor = item->As<RE::TESObjectARMO>(); armor && armor->HasPartOf(static_cast<RE::BGSBipedObjectForm::BipedObjectSlot>(slot))) {
				result.push_back(armor);
				if (data.second->IsWorn())
					equippedIdx = currentIdx;
				++currentIdx;
			}
		}
		if (result.size() > 0 && equippedIdx > 0 && equippedIdx < result.size()) { // Equipped item not in index 0 (and also exists)
			const auto temp = result[0];
			result[0] = result[equippedIdx];
			result[equippedIdx] = temp;
		}
		return result;
	}

	vector<string> GetArmorNames(StaticFunc, vector<RE::TESObjectARMO*> armors) {
		vector<string> result{};
		result.reserve(armors.size());
		for (const auto arm : armors) {
			if (arm)
				result.push_back(arm->GetFullName());
			else
				result.push_back("");
		}
		return result;
	}

	vector<i32> GetActorFactionRanks(StaticFunc, RE::Actor* act, vector<RE::TESFaction*> facs) {
		vector<i32> result(facs.size(), -3);
		if (!act || facs.empty())
			return result;

		bool isPlayer = act->formID == 0x14;
		for (size_t i = 0; i < facs.size(); ++i) {
			if (facs[i]) {
				result[i] = act->GetFactionRank(facs[i], isPlayer);
			}
		}

		/*auto base = act->GetActorBase();
		if (!base)
			return result;

		auto factionChanges = act->extraList.GetByType<RE::ExtraFactionChanges>();
		bool foundCur = false;
		i32 counter = 0;
		for (auto& fac : facs) {
			if (!fac) {
				counter++;
				continue;
			}

			foundCur = false;
			for (auto& factionInfo : base->factions) {
				if (factionInfo.faction == fac) {
					result[counter] = factionInfo.rank;
					foundCur = true;
					break;
				}
			}
			if (!foundCur && factionChanges) {
				for (auto& change : factionChanges->factionChanges) {
					if (change.faction == fac) {
						result[counter] = change.rank;
						foundCur = true;
						break;
					}
				}
			}
			if (!foundCur)
				result[counter] = -2;

			counter++;
		}*/
		return result;
	}

	void SetActorFactionRanks(StaticFunc, RE::Actor* act, vector<RE::TESFaction*> facs, vector<i8> ranks) {
		if (!act || facs.empty() || facs.size() != ranks.size())
			return;

		for (u64 i = 0; i < facs.size(); i++) {
			act->AddToFaction(facs[i], ranks[i]);
		}
		return;

		/*
		if (auto factionChanges = act->extraList.GetByType<RE::ExtraFactionChanges>(); factionChanges) {  // Unique NPCs will not enter here
			i8 counter = 0;
			for (auto& fac : facs) {
				if (!fac) {
					counter++;
					continue;
				}
				bool setCur = false;
				for (auto& change : factionChanges->factionChanges) {
					if (change.faction == fac) {
						change.rank = ranks[counter];
						setCur = true;
						break;
					}
				}
				if (!setCur) {  // Non-unique not in the faction so assign it to them
					RE::FACTION_RANK newFac;
					newFac.faction = fac;
					newFac.rank = ranks[counter];
					factionChanges->factionChanges.push_back(newFac);
				}
				counter++;
			}
		} else if (auto base = act->GetActorBase(); base) {  // Non-unique NPCs will not enter here
			i8 counter = 0;
			for (auto& fac : facs) {
				if (!fac) {
					counter++;
					continue;
				}
				bool setCur = false;
				for (auto& factionInfo : base->factions) {
					if (factionInfo.faction == fac) {
						factionInfo.rank = ranks[counter];
						setCur = true;
						break;
					}
				}
				if (!setCur) {  // Unique not in faction so assign it to them
					RE::FACTION_RANK newFac;
					newFac.faction = fac;
					newFac.rank = ranks[counter];
					base->factions.push_back(newFac);
				}
				counter++;
			}
		}*/
	}

	
	vector<RE::TESObjectREFR*> GetSurroundingTypeRefs(StaticFunc, RE::TESObjectREFR* center, i32 type, float radius, bool useLOS, bool dead, bool childs, bool disabled, bool deleted) {
		vector<RE::TESObjectREFR*> result;
		if (!center || radius <= 0.0f || type < 0 || type >= static_cast<i32>(RE::FormType::Max))
			return result;

		RE::Actor* act = nullptr;
		if (useLOS) {
			act = center->As<RE::Actor>();
			if (!act)
				return result;
		}
		
		const auto formType = static_cast<RE::FormType>(type);
		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(center, radius, [&](RE::TESObjectREFR* ref) {
				bool discard = false;
				if (ref->Is(formType) && ref->Is3DLoaded() && (!useLOS || act->HasLineOfSight(ref, discard)))
					if ((dead || !ref->IsDead()) && (childs || !ref->IsChild()) && (disabled || !ref->IsDisabled()) && (deleted || !ref->IsDeleted()))
						result.push_back(ref);
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}
		return result;
	}
	vector<RE::Actor*> GetSurroundingActors(StaticFunc, RE::TESObjectREFR* center, float radius, bool useLOS, bool dead, bool childs, bool disabled, bool deleted) {
		vector<RE::Actor*> result;
		if (!center || radius <= 0.0f)
			return result;
		
		RE::Actor* act = nullptr;
		if (useLOS) {
			act = center->As<RE::Actor>();
			if (!act)
				return result;
		}
		
		bool b = false;
		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(center, radius, [&](RE::TESObjectREFR* ref){
				if (ref->Is(RE::FormType::ActorCharacter) && ref->Is3DLoaded() && (!useLOS || act->HasLineOfSight(ref, b)))
					if ((dead || !ref->IsDead()) && (childs || !ref->IsChild()) && (disabled || !ref->IsDisabled()) && (deleted || !ref->IsDeleted()))
						result.push_back(ref->As<RE::Actor>());
				return RE::BSContainer::ForEachResult::kContinue;
			});
			
		}
		return result;
	}
	vector<string> GetActorNames(StaticFunc, vector<RE::Actor*> actors) {
		vector<string> result{};
		result.reserve(actors.size());
		for (const auto act : actors) {
			if (act)
				result.push_back(act->GetDisplayFullName());
			else
				result.push_back("");
		}
		return result;
	}

	// Actor Values have 2 components:
	// 1:	The base value which is the unbuffed level of the AV												vanilla Actor.GetBaseActorValue returns and Actor.SetActorValue sets this
	// 2:	The modifiers which are amounts added to base to get the Actor's final value. There are 3 different modifiers:
	// 		a:	The permanent modifier which is the sum of Enchantments and Abilities affecting the AV			vanilla Actor.ModActorValue adds to this
	// 		b:	The temporary modifier which is the sum of temporary effects, like potions, affecting the AV	No vanilla Papyrus function interacts with this.
	// 			After testing, potions seem to go to the permanent modifier (at least for health). Temporary magic effects do go to temporary, though.
	// 		c:	The damage modifier which is the sum of the damage that AV has taken.							vanilla Actor.DamageActorValue adds to this, always negative
	// 				Normally, only Health/Magicka/Stamina can get damaged. Via RestoreActorValue below, any AV can be damaged and that damage will be counted when calculating the AV value.
	// 				However, only Health/Magicka/Stamina have natural regeneration (unless their rates are set to 0) so damaging any other AV sticks until you undamage it.
	// 				Unlike permanent and temporary ones, the damage modifier cannot be >0. Adding positive values to it will increase it up to 0, working like a heal, but never bring it above.
	// The final value of an Actor's AV is base + permanent + temporary + damage.	vanilla Actor.GetActorValue returns this
	// Vanilla Actor.GetActorValueMax returns base + permanent + temporary. I haven't tested if it counts damage too but it would be an oversight if it id, because Actor.GetActorValue already does that.
	// Regarding Commonlib, RE::ActorValueOwner has functions to interact with AVs:
	// 1:	GetActorValue			returns base + permanent + temporary + damage	equivalent to vanilla Actor.GetActorValue
	// 2:	GetPermanentActorValue	returns base + permanent						ignoring temporary and damage modifiers
	// 3:	GetBaseActorValue		returns base									equivalent to vanilla Actor.GetBaseActorValue
	// 4:	SetBaseActorValue		sets base										equivalent to vanilla Actor.SetActorValue
	// 5:	ModActorValue			adds to base									positive/negative values increase/reduce respectively
	// 6:	RestoreActorValue		adds to a modifier								a_modifier determines which		positive/negative values increase/reduce respectively
	// 			if a_modifier==RE::ACTOR_VALUE_MODIFIER::kPermanent then it is equivalent to vanilla Actor.ModActorValue
	// 7:	SetActorValue			sets base										equivalent to vanilla Actor.SetActorValue, the same as SetBaseActorValue	redundant?
	vector<float> GetValuesByID(StaticFunc, RE::Actor* act, i32 avID) {
		vector<float> result(3, 0.0f);
		if (!act || avID >= 164 || avID < 0)
			return result;
		const auto av = static_cast<RE::ActorValue>(avID);
		result[0] = act->GetActorValue(av);
		result[1] = act->GetBaseActorValue(av);
		result[2] = act->GetPermanentActorValue(av);
		return result;
	}

	vector<float> GetValuesByName(StaticFunc, RE::Actor* act, RE::BSFixedString avName) {
		vector<float> result(3, 0.0f);
		if (!act)
			return result;
		const auto avList = RE::ActorValueList::GetSingleton();
		if (!avList)
			return result;
		const auto av = avList->LookupActorValueByName(avName);
		if (av == RE::ActorValue::kNone)
			return result;
		result[0] = act->GetActorValue(av);
		result[1] = act->GetBaseActorValue(av);
		result[2] = act->GetPermanentActorValue(av);
		return result;
	}

	// Get actor values of the 18 skills (IDs 6 to 23). base=true returns base levels, while base=false returns current levels. Current level is identical to max level for skill actor values.
	vector<float> GetSkillLevels(StaticFunc, RE::Actor* act, bool base) {
		vector<float> result(18, 0.0f);
		if (!act)
			return result;
		if (base) {  // get base
			for (u64 avID = 6; avID < 24; avID++) {
				result[avID - 6] = act->GetBaseActorValue(static_cast<RE::ActorValue>(avID));
			}
		} else {  // get current
			for (u64 avID = 6; avID < 24; avID++) {
				result[avID - 6] = act->GetActorValue(static_cast<RE::ActorValue>(avID));
			}
		}
		return result;
	}

	// Change actor values of the 18 skills (IDs 6 to 23). mode=1 sets bases, mode=2 mods permanent modifier, mode=3 mods base
	void SetSkillLevels(StaticFunc, RE::Actor* act, vector<float> levels, i32 mode) {
		if (!act || levels.size() != 18)
			return;
		if (mode == 1) {  // set base (vanilla SetActorValue)
			for (u64 avID = 6; avID < 24; avID++) {
				act->SetBaseActorValue(static_cast<RE::ActorValue>(avID), levels[avID - 6]);
			}
		} else if (mode == 2) {                                              // mod permanent (vanilla ModActorValue)
			const auto modPerma = static_cast<RE::ACTOR_VALUE_MODIFIER>(0);  // RE::ACTOR_VALUE_MODIFIER::kPermanent
			for (u64 avID = 6; avID < 24; avID++) {
				act->RestoreActorValue(modPerma, static_cast<RE::ActorValue>(avID), levels[avID - 6]);
			}
		} else if (mode == 3) {  // mod base
			for (u64 avID = 6; avID < 24; avID++) {
				act->ModActorValue(static_cast<RE::ActorValue>(avID), levels[avID - 6]);
			}
		}
	}


	i32 GetActorSex(StaticFunc, RE::Actor* act) {
		if (!act)
			return -2;
		if (auto base = act->GetActorBase(); base) {
			return static_cast<std::underlying_type<RE::SEX>::type>(base->GetSex());
		}
		return -2;
	}

	RE::BSFixedString GetFullName(StaticFunc, RE::TESObjectREFR* ref) {
		if (!ref)
			return "";
		return ref->GetDisplayFullName();
	}

	bool ActorHasLOS(StaticFunc, RE::Actor* act, RE::TESObjectREFR* ref) {
		if (!act || !ref)
			return false;
		bool arg2 = false;
		return act->HasLineOfSight(ref, arg2);
	}

	float GetRefsDistance(StaticFunc, RE::TESObjectREFR* ref1, RE::TESObjectREFR* ref2) {
		if (!ref1 || !ref2)
			return 0.0f;
		return ref1->GetPosition().GetDistance(ref2->GetPosition());
	}

	bool StartDialogueWithPlayer(StaticFunc, RE::Actor* speaker, RE::TESTopicInfo* info) {
		if (!speaker || !info)
			return false;
		return speaker->SetDialogueWithPlayer(true, true, info);
	}

	
	template <typename ContOut, typename ContIn>
	ContOut ArrTypeToType(ContIn contIn) {
		if (contIn.size() <= 0)
			return ContOut{};
		ContOut result(contIn.size());
		for (u64 i = 0; i < result.size(); ++i)
			result[i] = static_cast<ContOut::value_type>(contIn[i]);
		return result;
	}
	vector<i32> ArrFtoI(StaticFunc, vector<float> arr) { return ArrTypeToType<vector<i32>>(arr); }
	vector<float> ArrItoF(StaticFunc, vector<i32> arr) { return ArrTypeToType<vector<float>>(arr); }
	deque<bool> ArrFtoB(StaticFunc, vector<float> arr) { return ArrTypeToType<deque<bool>>(arr); }
	deque<bool> ArrItoB(StaticFunc, vector<i32> arr) { return ArrTypeToType<deque<bool>>(arr); }
	
	template <typename T, typename U>
	vector<T*> ArrFormToType(const vector<U*>& formArr) {
		vector<T*> result(formArr.size(), nullptr);
		if (result.size() <= 0)
			return result;
		for (size_t i = 0; i < result.size(); ++i)
			result[i] = skyrim_cast<T*>(formArr[i]); // works like Papyrus's "as". static_cast will put empty Actor forms in result, not None.
		return result;
	}
	vector<RE::TESObjectREFR*> ArrFormToRef (StaticFunc, vector<RE::TESForm*> formArr) { return ArrFormToType<RE::TESObjectREFR>(formArr); }
	vector<RE::Actor*> ArrFormToActor (StaticFunc, vector<RE::TESForm*> formArr) { return ArrFormToType<RE::Actor>(formArr); }
	vector<RE::BGSKeyword*> ArrFormToKwd (StaticFunc, vector<RE::TESForm*> formArr) { return ArrFormToType<RE::BGSKeyword>(formArr); }
	vector<RE::SpellItem*> ArrFormToSpell (StaticFunc, vector<RE::TESForm*> formArr) { return ArrFormToType<RE::SpellItem>(formArr); }
	vector<RE::TESFaction*> ArrFormToFac (StaticFunc, vector<RE::TESForm*> formArr) { return ArrFormToType<RE::TESFaction>(formArr); }
	vector<RE::Actor*> ArrRefToActor (StaticFunc, vector<RE::TESObjectREFR*> formArr) { return ArrFormToType<RE::Actor>(formArr); }
	
	template <typename T>
	vector<T*> CreateArrayType(i32 size, T* fill) { return vector<T*>(to_u64(size), fill); }
	vector<RE::TESFaction*> CreateFactionArray(StaticFunc, i32 size, RE::TESFaction* fill) { return CreateArrayType(size, fill); }

	// SKSE CreateBoolArray is messed up. Could be because it fails to convert from vector<bool>'s bitset to bool. Anyway deque doesn't do this and CommonlibSSE allows it as a type and it works so yeah.
	deque<bool> BoolArray(StaticFunc, i32 size, bool fill) { if (size < 0) return deque<bool>{}; return deque<bool>(size, fill); }
	
	template <typename T>
	vector<T*> SliceArrayType(const vector<T*>& arr, i32 start, i32 end) {
		if (arr.size() <= 0u) {
			return vector<T*>{};
		}

		size_t eIdx{ (end >= 0) ? (std::clamp<size_t>(static_cast<size_t>(end), 0u, arr.size() - 1u)) : (arr.size() - 1u)};

		vector<T*> temp(arr.begin() + to_u64(start), arr.begin() + eIdx + 1u); // range constructor is [first, last)
		return temp;
	}
	vector<RE::TESFaction*> SliceFactionArray(StaticFunc, vector<RE::TESFaction*> arr, i32 start, i32 end) { SliceArrayType(arr, start, end); return arr; }
	
	template <typename T>
	void PushArrayType(vector<T*>& arr, T* val) { arr.push_back(val); }
	vector<RE::TESFaction*> PushFaction(StaticFunc, vector<RE::TESFaction*> arr, RE::TESFaction* fac) { PushArrayType(arr, fac); return arr; }
	
	template <typename T>
	void ShiftPushType(vector<T>& arr, T item, bool atEnd) {
		if (arr.size() <= 1u) [[unlikely]] {
			arr.assign(1u, item);
		}
		else if (atEnd) {
			std::shift_left(arr.begin(), arr.end(), 1);
			arr.back() = item;
		}
		else {
			std::shift_right(arr.begin(), arr.end(), 1);
			arr.front() = item;
		}
	}
	vector<float> ShiftPushFloat(StaticFunc, vector<float> arr, float val, bool atEnd) { ShiftPushType(arr, val, atEnd); return arr; }
	vector<i32> ShiftPushInt(StaticFunc, vector<i32> arr, i32 val, bool atEnd) { ShiftPushType(arr, val, atEnd); return arr; }
	vector<string> ShiftPushString(StaticFunc, vector<string> arr, string val, bool atEnd) { ShiftPushType(arr, val, atEnd); return arr; }
	vector<RE::TESForm*> ShiftPushForm(StaticFunc, vector<RE::TESForm*> arr, RE::TESForm* val, bool atEnd) { ShiftPushType(arr, val, atEnd); return arr; }
	
	i32 RandInt(StaticFunc, i32 min, i32 max) { return RNG::Random(min, max); }
	float RandFloat(StaticFunc, float min, float max) { return RNG::Random(min, max); }
	vector<int32_t> RandIntArr(StaticFunc, i32 size, i32 min, i32 max) { if (size <= 0) [[unlikely]] return vector<i32>{}; return RNG::RandomVec(size, min, max); }
	vector<float> RandFloatArr(StaticFunc, i32 size, float min, float max) { if (size <= 0) [[unlikely]] return vector<float>{}; return RNG::RandomVec(size, min, max); }


	string GameTimeToStr(StaticFunc, float time) { return StringUtils::GetTimeString(time); }

	string TStr(StaticFunc, bool flag, string trueStr, string falseStr) { return flag ? trueStr : falseStr; }

	float TrimF(StaticFunc, float number, i32 toDecimals) {
		if (toDecimals < 0)
			return number;
		else if(toDecimals == 0)
			return std::truncf(number);

		number *= (10.0f * static_cast<float>(toDecimals)); // so 36.1234567 targeting 3 decimals becomes 36123.4567
		number = std::truncf(number); // purge decimals, 36123.00
		number /= (10.0f * static_cast<float>(toDecimals)); // turn back to same magnitude, 36.123
		return number;
	}

	void Log(StaticFunc, string msg, i32 severity) {
		if (severity >= 0 and severity < static_cast<i32>(Log::Severity::prohibit)) {
			Log::Papyrus(std::move(msg), static_cast<Log::Severity>(severity));
		}
	}

	
	static constexpr string_view script{ "ODFunctionsMisc"sv };
	bool Register(RE::BSScript::IVirtualMachine* vm) {

		vm->RegisterFunction("WornHasKeywords"sv, script, WornHasKeywords);
		vm->RegisterFunction("HasKeywords"sv, script, HasKeywords);
		vm->RegisterFunction("GetHeldArmorsOfSlot"sv, script, GetHeldArmorsOfSlot);
		vm->RegisterFunction("GetArmorNames"sv, script, GetArmorNames);

		vm->RegisterFunction("GetActorFactionRanks"sv, script, GetActorFactionRanks);
		vm->RegisterFunction("SetActorFactionRanks"sv, script, SetActorFactionRanks);

		vm->RegisterFunction("GetSurroundingTypeRefs"sv, script, GetSurroundingTypeRefs);
		vm->RegisterFunction("GetSurroundingActors"sv, script, GetSurroundingActors);
		vm->RegisterFunction("GetActorNames"sv, script, GetActorNames);

		vm->RegisterFunction("GetValuesByID"sv, script, GetValuesByID);
		vm->RegisterFunction("GetValuesByName"sv, script, GetValuesByName);
		vm->RegisterFunction("GetSkillLevels"sv, script, GetSkillLevels);
		vm->RegisterFunction("SetSkillLevels"sv, script, SetSkillLevels);

		vm->RegisterFunction("GetActorSex"sv, script, GetActorSex);
		vm->RegisterFunction("GetFullName"sv, script, GetFullName);
		vm->RegisterFunction("ActorHasLOS"sv, script, ActorHasLOS);
		vm->RegisterFunction("GetRefsDistance"sv, script, GetRefsDistance);

		vm->RegisterFunction("StartDialogueWithPlayer"sv, script, StartDialogueWithPlayer);

		vm->RegisterFunction("ArrFtoI"sv, script, ArrFtoI, true);
		vm->RegisterFunction("ArrItoF"sv, script, ArrItoF, true);
		vm->RegisterFunction("ArrFtoB"sv, script, ArrFtoB, true);
		vm->RegisterFunction("ArrItoB"sv, script, ArrItoB, true);
		vm->RegisterFunction("ArrFormToRef"sv, script, ArrFormToRef, true);
		vm->RegisterFunction("ArrFormToActor"sv, script, ArrFormToActor, true);
		vm->RegisterFunction("ArrFormToKwd"sv, script, ArrFormToKwd, true);
		vm->RegisterFunction("ArrFormToSpell"sv, script, ArrFormToSpell, true);
		vm->RegisterFunction("ArrFormToFac"sv, script, ArrFormToFac, true);
		vm->RegisterFunction("ArrRefToActor"sv, script, ArrRefToActor, true);

		vm->RegisterFunction("CreateFactionArray"sv, script, CreateFactionArray, true);
		vm->RegisterFunction("SliceFactionArray"sv, script, SliceFactionArray, true);
		vm->RegisterFunction("PushFaction"sv, script, PushFaction, true);

		vm->RegisterFunction("BoolArray"sv, script, BoolArray, true);

		vm->RegisterFunction("ShiftPushFloat"sv, script, ShiftPushFloat, true);
		vm->RegisterFunction("ShiftPushInt"sv, script, ShiftPushInt, true);
		vm->RegisterFunction("ShiftPushString"sv, script, ShiftPushString, true);
		vm->RegisterFunction("ShiftPushForm"sv, script, ShiftPushForm, true);

		vm->RegisterFunction("RandInt"sv, script, RandInt, true);
		vm->RegisterFunction("RandFloat"sv, script, RandFloat, true);
		vm->RegisterFunction("RandIntArr"sv, script, RandIntArr, true);
		vm->RegisterFunction("RandFloatArr"sv, script, RandFloatArr, true);
		

		vm->RegisterFunction("GameTimeToStr"sv, script, GameTimeToStr, true);

		vm->RegisterFunction("TStr"sv, script, TStr, true);
		vm->RegisterFunction("TrimF"sv, script, TrimF, true);

		vm->RegisterFunction("Log"sv, script, Log, true);

		

		return true;
	}
}
