#include "Data.h"

#include "DataDefs/FearOps.h"
#include "DataDefs/RulesOps.h"

#include "DataDefs/Multivector.h"

#include "RulesMenu.h"			// Player PlayerRules management menu
#include "ExtraKeywords.h"		// Add keywords from MCM Papyrus functions

#include "Types/SLHelpers.h"
#include "Types/SyncTypes.h"

namespace Data {

	using GameDataUtils::SetEffectsMagnitude;
	using GameDataUtils::SetNthEffectDescription;
	using GameDataUtils::SetNthEffectMagnitude;
	using PrimitiveUtils::to_s32;

	SyncTypes::LockProtectedResource<multivector> locker{};
  


	// Shared interface
	namespace Shared {

		constexpr bool FearEnabled = true; // Replace with MCM check
		constexpr bool DebuffsEnabled = true; // Replace with MCM check

		void BrawlEvent(RE::TESForm* controller, const bool starting) noexcept {
			using SLHelpers::sslThreadController_interface::MaxActors;
			const SLHelpers::sslThreadController_interface analyzer{ controller };
			if (!analyzer.IsValid()) {
				return;
			}

			const auto actives = analyzer.ActiveActors();

			const auto locked = locker.GetExclusive();

			// Just be dumb and simply do this if scene is ending, and live with the slight code duplication
			if (!starting) {
				if (const auto idx = locked->find_index(analyzer.PassiveActor()); locked->is_valid(idx)) {
					locked->fear(idx).in_brawl = false;
				}
				for (u32 i = 0, actor_count = analyzer.ActorCount(); i < actor_count; ++i) {
					if (const size_t idx = locked->find_index(actives[i].get()); locked->is_valid(idx)) {
						locked->fear(idx).in_brawl = false;
					}
				}
				return;
			}

			// Scene is starting from here on
			struct idx_pair {
				u32 active_idx;
				u32 data_idx;
			};
			array<idx_pair, MaxActors> found_idxs{};
			const u32 found_count = [&locked, &actives, &found_idxs, actor_count = analyzer.ActorCount()]() {
				// Find FearInfo pointers of actives, and update their in_brawl. Do it in here because I like const counts.
				u32 found_count = 0;
				for (u32 i = 0; i < actor_count; ++i) {
					if (const size_t prospective = locked->find_index(actives[i].get()); locked->is_valid(prospective)) {
						found_idxs[found_count].active_idx = i;
						found_idxs[found_count].data_idx = static_cast<u32>(prospective);
						++found_count;
						locked->fear(prospective).in_brawl = true;
					}
				}
				return found_count;
			}();

			FearInfo* passive_fear;
			if (const auto passive_index = locked->find_index(analyzer.PassiveActor()); locked->is_valid(passive_index)) {
				passive_fear = &locked->fear(passive_index);
			} else {
				return; // No data for passive. Can return.
			}
			passive_fear->in_brawl = true;
			if (found_count == 0) {
				return; // Everything below requires a passive actor and at least 1 active actor, and should only trigger on scene start (picked arbitrarily, just need triggering once)
			}

			// Fears adjustment. SKSE queues the modevent callbacks using Skyrim's Papyrus VM like all other events, so these should be safe to access game data from.
			const bool passive_female = passive_fear->is_female;
			u8 pres_flags = 0;
			if (passive_female) {
				for (u32 i = 0; i < found_count; ++i) {
					FearInfo& info = locked->fear(found_idxs[i].data_idx);
					pres_flags |= static_cast<u8>(1 << static_cast<u8>(info.is_female));
					info.fears_female *= 0.99f;
					actives[found_idxs[i].active_idx]->AddToFaction(::Fear::Faction(::Fear::FAC::FearsFemale), info.FearsFemaleRank());
				}
			} else {
				for (u32 i = 0; i < found_count; ++i) {
					FearInfo& info = locked->fear(found_idxs[i].data_idx);
					pres_flags |= static_cast<u8>(1 << static_cast<u8>(info.is_female));
					info.fears_male *= 0.99f;
					actives[found_idxs[i].active_idx]->AddToFaction(::Fear::Faction(::Fear::FAC::FearsMale), info.FearsMaleRank());
				}
			}
			if (pres_flags & 1) { // Male in actives (1 << false)
				passive_fear->fears_male *= 1.01f;
				analyzer.PassiveActor()->AddToFaction(::Fear::Faction(::Fear::FAC::FearsMale), passive_fear->FearsMaleRank());
			}
			if (pres_flags & 2) { // Female in actives (1 << true)
				passive_fear->fears_female *= 1.01f;
				analyzer.PassiveActor()->AddToFaction(::Fear::Faction(::Fear::FAC::FearsFemale), passive_fear->FearsFemaleRank());
			}

			// PlayerRules rules stuff for passive player
			if (analyzer.PassiveActor()->IsPlayerRef() bitand passive_fear->is_blocked) {
				rules_info& prules = locked->player_rules();
				prules.EnteredBrawl(analyzer.Tags());
				if (analyzer.PassiveIsVictim()) {
					prules.EnteredUnfairBrawl();
				}
				array<Vanilla::RCE, MaxActors> races{};
				for (size_t i = 0, max = actives.size(); i < max; ++i) {
					races[i] = Vanilla::RaceToID(actives[i]->GetRace());
				}
				prules.Brawled(races);
			}
		}


		void ProcessEquip(RE::Actor* act, const RE::TESBoundObject* obj) noexcept {
			if (IsValidAddable(act)) {
				switch (obj->GetFormType()) {
				case RE::TESObjectARMO::FORMTYPE: {
					const auto locked = locker.GetExclusive();
					if (auto idx = locked->has_or_add(act); idx < locked->size()) {
						auto flags = locked->equipstate(idx).ProcessArmorEquip(static_cast<const RE::TESObjectARMO*>(obj));
						if (act->IsPlayerRef() and locked->player_is_blocked()) {
							locked->player_rules().ArmorEquipped(flags);
						}
					}
					break;
				}
				case RE::TESObjectWEAP::FORMTYPE: {
					if (auto type = static_cast<const RE::TESObjectWEAP*>(obj)->GetWeaponType(); type != RE::WeaponTypes::WEAPON_TYPE::kHandToHandMelee) { // Ignore Fists
						const auto locked = locker.GetExclusive();
						if (auto idx = locked->has_or_add(act); idx < locked->size()) {
							locked->equipstate(idx).UpdateHands(act);
							if (act->IsPlayerRef() and locked->player_is_blocked()) {
								locked->player_rules().HandEquipped({ type });
							}
						}
					}
					break;
				}
				case RE::SpellItem::FORMTYPE: {
					const auto locked = locker.GetExclusive();
					if (auto idx = locked->has_or_add(act); idx < locked->size()) {
						locked->equipstate(idx).UpdateHands(act);
						if (act->IsPlayerRef() and locked->player_is_blocked()) {
							locked->player_rules().HandEquipped({ static_cast<const RE::SpellItem*>(obj)->GetAssociatedSkill() });
						}
					}
					break;
				}
				default: break;
				}
			}
		}
		void ProcessUnequip(RE::Actor* act, const RE::TESBoundObject* obj) noexcept {
			if (IsValidAddable(act)) {
				switch (obj->GetFormType()) {
				case RE::TESObjectARMO::FORMTYPE: {
					const auto locked = locker.GetExclusive();
					if (auto idx = locked->has_or_add(act); idx < locked->size()) {
						EquipState& state = locked->equipstate(idx);
						state.ProcessArmorUnequip(static_cast<const RE::TESObjectARMO*>(obj));
						if (act->IsPlayerRef() and locked->player_is_blocked()) {
							locked->player_rules().ArmorUnequipped(state);
						}
					}
					break;
				}
				case RE::TESObjectWEAP::FORMTYPE: {
					if (static_cast<const RE::TESObjectWEAP*>(obj)->GetWeaponType() == RE::WeaponTypes::WEAPON_TYPE::kHandToHandMelee) {
						return; // Ignore Fists
					}
					// Fallthrough because, besides the Fists check, Weapons and Spells do the exact same thing on unequip
				}
				case RE::SpellItem::FORMTYPE: {
					const auto locked = locker.GetExclusive();
					if (auto idx = locked->has_or_add(act); idx < locked->size()) {
						EquipState& state = locked->equipstate(idx);
						state.UpdateHands(act);
						if (act->IsPlayerRef() and locked->player_is_blocked()) {
							locked->player_rules().HandUnequipped(state.hands);
						}
					}
					break;
				}
				default: break;
				}
			}
		}

		void FastTravelEnd(const float hours) noexcept {
			PlayerRules::FastTravelEnd(hours);
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				locked->player_rules().FastTravelEnd();
			}
		}


		void Update(const UpdateKinds kinds, const UpdateDeltas deltas) noexcept {
			using namespace UpdateTypes;
			alignas(64) static array<trivial_handle, MaxUpdateCount> handles{};			// 128 - 2 cachelines
			alignas(64) static array<RE::ActorPtr, MaxUpdateCount> actptrs{};			// 256 - 4 cachelines
			alignas(64) static array<main_out, MaxUpdateCount> mains{};					// 256 - 4 cachelines	Main thread pulls, for Fear
			alignas(64) static ranks_and_oneofs pack{};									// 128 - 2 cachelines	ranks from Fear for main thread in first 1.5, misc in other 0.5
			// Total 768 bytes, in 12 cachelines. Arrays in the first 11.5, and then some bytes for one-ofs and bookkeeping
			
			struct {
				// Overwrites the oldest task to make room for the new one, even if that oldest task wasn't null.
				void AddTask(void(*task)()) noexcept {
					tq[2] = tq[1];
					tq[1] = tq[0];
					tq[0] = task;
				}
				// Nulls all tasks.
				void ClearTasks() noexcept {
					tq[0] = nullptr;
					tq[1] = nullptr;
					tq[2] = nullptr;
				}
				// Queues and waits for all non-null tasks to finish executing on the main thread.
				void RunTasks() noexcept {
					if (const u32 count = CountTasks(); count != 0) {
						const auto tasker = SKSE::GetTaskInterface();
						if (!tasker) {
							return;
						}

						std::atomic<bool> waiter{};
						waiter.store(false, std::memory_order_release); // Required? Not if construction syncs but does it? Also, maybe sequential?

						tasker->AddTask([&] {
							if (waiter.load(std::memory_order_acquire) != false) { // For sync
								return;
							}
							for (size_t i = 0; i < count; ++i) {
								tq[i]();
							}
							waiter.store(true, std::memory_order_release);
							waiter.notify_all();
						});

						waiter.wait(false, std::memory_order_acquire); // Wait until not false

						ClearTasks();
					}
				}
			private:
				u32 CountTasks() const noexcept { return static_cast<u32>((tq[0] != nullptr) + (tq[1] != nullptr) + (tq[2] != nullptr)); }
				array<void(*)(), 3> tq{}; // Only need 2 atm
			} static task_queue{}; // Holds a queue of up to 3 tasks to execute on the main thread.

			static multivector* data_ptr{ nullptr };
			
			static auto get_filtered_actors = [] {
				const RE::ProcessLists* lists{ RE::ProcessLists::GetSingleton() };
				const trivial_handle ph{ RE::PlayerCharacter::GetSingleton() };
				RE::ActorPtr pptr = ph.get();
				if ((pptr != nullptr) bitand (lists != nullptr)) {
					pack.now = GameDataUtils::DaysPassed();
					pack.follower = static_cast<RE::PlayerCharacter*>(pptr.get())->teammateCount != 0;
					handles[0] = ph;
					actptrs[0] = std::move(pptr);
					const u32 max_i = std::min<u32>(lists->highActorHandles.size(), MaxUpdateCount - 1);
					u32 next = 1;
					for (u32 i = 0; i < max_i; ++i) {
						const trivial_handle h = lists->highActorHandles[i];
						if (RE::ActorPtr ptr = h.get(); ptr and IsValidAddable(ptr.get())) {
							handles[next] = h;
							actptrs[next] = std::move(ptr);
							++next;
						}
					}
					pack.actor_count = next;
					// Log::Error("\tUpdating: get_filtered_actors grabbed {} valids out of {}"sv, pack.actor_count, lists->highActorHandles.size());
				}
			};
			static auto get_main = [] {
				if (data_ptr) {
					data_ptr->init_news(actptrs, pack, FearEnabled); // Not-nullptr means there are unregistered. Even if not, the func would do nothing
					data_ptr = nullptr;
				}
				const u32 end = pack.actor_count;
				for (u32 i = 0; i < end; ++i) {
					mains[i].ParseActor(actptrs[i].get());
				}
				for (u32 i = 0; i < end; ++i) {
					for (u32 j = 0; j < end; ++j) {
						mains[i].seeing.set_if_val(j, actptrs[i]->RequestDetectionLevel(actptrs[j].get()) > 0);
					}
				}
			};
			static auto set_desc = [] {
				const i32 mag = pack.mag;
				SetNthEffectDescription(::PlayerRules::Spell(::PlayerRules::SPL::RecentDodges), 0, "You have Dodged " + to_string(mag) + " time" + ((mag != 1) ? "s" : "") + " recently");
			};
			static auto set_ranks = [] {
				for (u32 i = 0, end = pack.actor_count; i < end; ++i) {
					actptrs[i]->AddToFaction(::Fear::Faction(::Fear::FAC::Fear), pack.ranks[i][0]);
					actptrs[i]->AddToFaction(::Fear::Faction(::Fear::FAC::Thrillseeking), pack.ranks[i][1]);
					actptrs[i]->AddToFaction(::Fear::Faction(::Fear::FAC::Thrillseeker), pack.ranks[i][2]);
				}
			};
			static auto set_debuffs = [] {
				RE::SpellItem* const mscw = ::PlayerRules::Spell(::PlayerRules::SPL::DebuffMSCW);
				RE::SpellItem* const craft = ::PlayerRules::Spell(::PlayerRules::SPL::DebuffCrafts);
				RE::SpellItem* const resist = ::PlayerRules::Spell(::PlayerRules::SPL::DebuffResist);
				RE::SpellItem* const power = ::PlayerRules::Spell(::PlayerRules::SPL::DebuffPower);

				RE::Actor* pptr = actptrs[0].get();

				pptr->RemoveSpell(mscw); // Always clear all debuffs first. Must remove anyway to reapply.
				pptr->RemoveSpell(craft);
				pptr->RemoveSpell(resist);
				pptr->RemoveSpell(power);
				
				const float wp_mod = 1.0f - pack.player_willpower;
				const float mag_base = (pack.player_tension - 1.0f) * wp_mod;

				if (mag_base > 0.0f) {
					const float scaled100 = std::min<float>(mag_base, 0.5f) * 100.0f; // Cap at 50 speed and 100 carry weight
					SetNthEffectMagnitude(mscw, 0, scaled100);
					pptr->AddSpell(mscw);
					const u32 trunc = static_cast<u32>(scaled100);
					SetNthEffectDescription(mscw, 0, "Movement Speed is reduced by <" + to_string(trunc) + ">, and Carry Weight by <" + to_string(trunc * 2) + '>');
				}
				else {
					return; // Below checks would fail
				}

				if (const float mag_craft = mag_base - 0.5f; mag_craft > 0.0f) { // Start applying after mscw gets capped
					const float scaled100 = std::min<float>(mag_craft, 0.75f) * 100.0f; // Cap at 75
					SetEffectsMagnitude(craft, 0, 1, scaled100);
					pptr->AddSpell(craft);
					SetNthEffectDescription(craft, 0, "Smithing, Alchemy, and Enchanting are <" + to_string(static_cast<u32>(scaled100)) + ">% weaker");
				}
				else {
					return; // Below checks would fail
				}

				if (const float mag_resist = mag_base - 1.0f; mag_resist > 0.0f) { // Start applying after craft reaches 50%
					const float scaled100 = std::min<float>(mag_resist, 0.75f) * 100.0f; // Cap at 75 magic resist and 225 armor
					SetNthEffectMagnitude(resist, 0, scaled100);
					pptr->AddSpell(resist);
					const u32 trunc = static_cast<u32>(scaled100);
					SetNthEffectDescription(resist, 0, "Magic Resistance is reduced by <" + to_string(trunc) + ">% and Armor by <" + to_string(trunc * 5) + '>');
				}
				else {
					return; // Below checks would fail
				}

				if (const float mag_power = mag_base - 2.0f; mag_power > 0.0f) { // Start applying a bit after resist gets capped
					const float scaled1 = std::min<float>(mag_power, 0.75f); // Cap at 75 weapon damage/spell power
					const float scaled100 = scaled1 * 100.0f;
					SetNthEffectMagnitude(power, 0, scaled1); // AttackDamageMult 1=>100%
					SetEffectsMagnitude(power, 1, 3, scaled100); // <skill>PowerMod 100=>100%
					pptr->AddSpell(power);
					const string trunc{ to_string(static_cast<u32>(scaled100)) };
					SetNthEffectDescription(power, 0, "Weapons deal <" + trunc + ">% less damage and Illusion, Conjuration, Destruction, Alteration, and Restoration are <" + trunc + ">% weaker");
				}

			};
			static auto zero_invalid_handles = [] {
				if (data_ptr) {
					auto begin = data_ptr->all_handles().begin();
					auto end = data_ptr->all_handles().end();
					for (; begin != end; ++begin) {
						if (RE::Actor* act = begin->get().get(); !act or !IsValidKeepable(act)) {
							begin->reset();
						}
					}
					data_ptr = nullptr;
				}
			};

			// Log::Info("Update!"sv);

			if (kinds.is_marked(UpdateKinds::LongUpdate)) { // Do the long first because it removes invalid elements
				const auto locked = locker.GetExclusive();

				data_ptr = locked.operator->(); // lol
				task_queue.AddTask(zero_invalid_handles);
				task_queue.RunTasks();

				locked->clear_zero_handles();
			}

			if (kinds.is_marked(UpdateKinds::DefaultUpdate)) {
				task_queue.AddTask(get_filtered_actors);
				task_queue.RunTasks();

				// Log::Info("Default update with {} actors"sv, pack.actor_count);

				if (pack.actor_count != 0) {
					const auto locked = locker.GetExclusive();

					if (locked->swap_allocate_move(handles, actptrs, pack)) { // Allocation for unregistereds needed and performed
						data_ptr = locked.operator->();
					}
					// Log::Info("Default update with {} unregistered actors and data size {}"sv, pack.unregistered_count, data_ptr->size());
					task_queue.AddTask(get_main);

					task_queue.RunTasks();

					pack.need_ranks = FearEnabled;
					Fear::Update(deltas.delta_default, locked->all_fears(), locked->all_equips(), mains, actptrs, pack);
					if (FearEnabled) { // Only affect factions if our Fear is used
						task_queue.AddTask(set_ranks);
					}

					if (locked->player_is_blocked()) {
						locked->player_rules().Update(locked->equipstate(0), PlayerRules::GetDayDelta(pack.now), pack.player_tension, pack.follower);
						pack.player_willpower = locked->player_rules().Exp().get();
						if (DebuffsEnabled) {
							task_queue.AddTask(set_debuffs);
						}
					}
				}

				task_queue.RunTasks(); // Don't need the lock for post-tasks
			}


			for (auto& ptr : actptrs) {
				ptr.reset(); // Let go of the shared pointers
			}

		}



		class ChangeCellHook {
		public:
			static void Install() {
				REL::Relocation<std::uintptr_t> VTable{RE::VTABLE_PlayerCharacter[0]};
				original = VTable.write_vfunc(0x98, SetCell); // Actor.h SetParentCell(RE::TESObjectCELL*)
			}
		private:
			static void SetCell(RE::PlayerCharacter* this_, RE::TESObjectCELL* cell) { // Don't forget the this pointer retard
				original(this_, cell);
				if (cell) {
					Fear::PlayerChangedCell(cell);
				}
			}
			static inline REL::Relocation<decltype(SetCell)> original;
		};
		void InstallHooks() {
			ChangeCellHook::Install();
			Log::Info("Hooked PlayerCharacter::SetParentCell(TESObjectCELL*)"sv);
		}



		bool Save(SKSE::SerializationInterface& intfc) noexcept {
			if (!intfc.OpenRecord(SerializationType, SerializationVersion)) {
				return false;
			}
			return locker.GetExclusive()->Save(intfc) and Fear::Save(intfc) and PlayerRules::Save(intfc);
		}

		bool Load(SKSE::SerializationInterface& intfc, u32 version) noexcept {
			if (version != SerializationVersion) {
				Log::Error("Serialization version out of date. Read <{}>, expected <{}>. Deserialization aborted."sv, version, SerializationVersion);
				return false;
			}
			return locker.GetExclusive()->Load(intfc) and Fear::Load(intfc) and PlayerRules::Load(intfc);
		}

		void Revert() noexcept { locker.GetExclusive()->clear(); Fear::Revert(); PlayerRules::Revert(); }

	}

	// PlayerRules interface
	namespace PlayerRules {

		static void AdvancePlayerRule1(const GranterID id) {
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				locked->player_rules().AdvanceGranter(id, 1);
			}
		}
		void PlayerAbsorbedDragonSoul() noexcept { AdvancePlayerRule1(GranterID::AbsorbDragonSouls); }
		void PlayerKillDraugr() noexcept { AdvancePlayerRule1(GranterID::KillDraugr); }
		void PlayerKillDaedra() noexcept { AdvancePlayerRule1(GranterID::KillDaedra); }
		void PlayerKillDragonPriest() noexcept { AdvancePlayerRule1(GranterID::KillDragonPriests); }


		namespace Messaging {
			using MenuUtils::WaitEVMMBV;
			using MenuUtils::WaitEVSM_F;

			enum Next {
				Base,

				DodgeRecords,

				RulesBase,

				RulesDetailsBase,
				RuleDetails,

				RulesBuyBase,
				RuleBuyPreview,
				RuleBuyCustomize,
				RuleBuyGold,

				RulesRemoveBase,
				RuleRemove,

				DebugRules,

				Exit,

				FatalError
			};

			/*
			// For debug
			static string NextStr(Next id) {
				switch (id) {
				case Base: return "<Home>";
				case DodgeRecords: return "<Dodges>";
				case RulesBase: return "<RulesBase>";
				case RulesDetailsBase: return "<DetailsBase>";
				case RuleDetails: return "<Details>";
				case RulesBuyBase: return "<BuyBase>";
				case RuleBuyPreview: return "<BuyPreview>";
				case RuleBuyCustomize: return "<BuyCustomize>";
				case RuleBuyGold: return "<BuyGold>";
				case RulesRemoveBase: return "<RemoveBase>";
				case RuleRemove: return "<Remove>";
				case DebugRules: return "<Show Punishes>";
				case Exit: return "<Exit>";
				case FatalError: return "<FatalError>";
				default: return "<INVALID>";
				}
			}// */

			struct Buttons {
				Buttons() {}
				Buttons(const lazy_vector<Next>& ids) : nexts(ids) { MatchTextsToNexts(); }
				Buttons(lazy_vector<Next>&& ids) : nexts(std::move(ids)) { MatchTextsToNexts(); }

				operator lazy_vector<string>() const { return texts; }

				Buttons& add(Next id) { nexts.try_append(id); texts.try_append(NextToString(id)); return *this; }

				Buttons& add(Next id, const string& t) { nexts.try_append(id); texts.try_append(t); return *this; }
				Buttons& add(Next id, string&& t) { nexts.try_append(id); texts.try_append(std::move(t)); return *this; }

				Buttons& add(Next id, const lazy_vector<string>& ts) {
					if (!ts.empty()) {
						nexts.try_append_n(ts.size(), id);
						texts.try_append_range_copy(ts.begin(), ts.end());
					}
					return *this;
				}
				Buttons& add(Next id, lazy_vector<string>&& ts) {
					if (!ts.empty()) {
						nexts.try_append_n(ts.size(), id);
						texts.try_append_range_move(ts.begin(), ts.end());
					}
					return *this;
				}

				Next get_next(i32 i) const noexcept {
					if (i >= 0 and i < nexts.size()) {
						return nexts[static_cast<size_t>(i)];
					}
					return FatalError;
				}
				size_t size() const noexcept { return nexts.size(); }
				void clear() noexcept { nexts.clear(); texts.clear(); }

				lazy_vector<Next> nexts{};
				lazy_vector<string> texts{};
			private:
				static string NextToString(Next id) {
					switch (id) {
					case Base: return "Home";
					case DodgeRecords: return "Dodges";
					case RulesBase: return "Rules";
					case RulesDetailsBase: return "Details";
					case RulesBuyBase: return "Buy";
					case RuleBuyCustomize: return "Customize";
					case RuleBuyGold: return "Buy with Gold";
					case RulesRemoveBase: return "Remove";
					case RuleRemove: return "Remove";
					case DebugRules: return "Show Punishes";
					case Exit: return "Exit";
					default: return "INVALID";
					}
				}
				void MatchTextsToNexts() {
					texts.resize(nexts.size());
					for (u64 i = 0, end = texts.size(); i < end; ++i) {
						texts[i] = NextToString(nexts[i]);
					}
				}
			};

			static Next OpenRuleRemove(const GranterID id) {
				string msg{};
				static const Buttons buttons{ [] {
					Buttons btns{};
					btns.add(RuleRemove).add(RulesRemoveBase, "Back").add(Exit);
					return btns;
				}() };
				if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
					msg = locked->player_rules().Overview(id) + "\n\nRemoving a rule does not refund it, only enables buying a new version of it.";
				} else {
					return Exit;
				}
				Next next = buttons.get_next(WaitEVMMBV(msg, buttons));
				if (next == RuleRemove) {
					next = RulesRemoveBase;
					if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
						locked->player_rules().RemoveGranter(id);
					}
				}
				return next;
			}
			static Next OpenRulesRemoveBase() {
				string msg{};
				Buttons buttons{};
				lazy_vector<GranterID> rule_ids{};
				auto refresh = [&] {
					if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
						const rules_info& prules = locked->player_rules();
						msg = prules.GetGranterStrBase(0);
						rule_ids = prules.GetGranterIDs(true);
						buttons.clear();
						buttons.add(RuleRemove, rules_info::GranterStaticNames(rule_ids));
						buttons.add(RulesBase, "Back").add(Exit);
						return true;
					} else {
						return false;
					}
				};
				if (!refresh()) {
					return Exit;
				}
				i32 res = WaitEVMMBV(msg, buttons);
				Next next = buttons.get_next(res);
				while (next == RuleRemove) {
					if (Next r = OpenRuleRemove(rule_ids[static_cast<size_t>(res)]); r == RulesRemoveBase) { // Aka "Back"
						if (!refresh()) {
							return Exit;
						}
						res = WaitEVMMBV(msg, buttons);
						next = buttons.get_next(res);
					}
					else {
						return r;
					}
				}
				return next;
			}
			static Next OpenRuleBuyCustomize(GranterMaker& maker) {
				const array<string, 3> tac{ rules_info::GranterStaticName(maker.ID()), "Set", "Cancel" };
				auto sliders{ maker.GetParams() };
				if (sliders.empty()) {
					return Exit;
				}
				maker.SetParams(WaitEVSM_F(tac, sliders));
				return RulesDetailsBase;
			}
			static Next OpenRuleBuyPreview(const GranterID id) {
				static auto get_gold = [](RE::Actor* act) -> i32 {
					if (!act) {
						return 0;
					}
					if (const auto tasker = SKSE::GetTaskInterface(); tasker) {
						std::atomic<i32> amount{ -1 };
						tasker->AddTask([&] {
							i32 gold = act->GetGoldAmount();
							if (gold < 0) {
								amount.store(0);
							}
							else {
								amount.store(gold);
							}
							amount.notify_all();
						});
						amount.wait(-1);
						return amount.load();
					}
					return 0;
				};
				static auto remove_gold = [](RE::Actor* act, i32 amount) -> bool {
					if (!act) {
						return false;
					}
					if (amount == 0) {
						return true;
					}
					else if (const auto tasker = SKSE::GetTaskInterface(); tasker and amount > 0) {
						std::atomic<bool> success{ false };
						tasker->AddTask([&] {
							i32 gold = act->GetGoldAmount();
							if (gold < amount) {
								success.store(false);
							}
							else {
								success.store(RemoveGold(act, amount)); // Will always be true if we got here
							}
							success.notify_all();
						});
						success.wait(false);
						return success.load();
					}
					return false;
				};

				GranterMaker maker{};
				maker.SetID(id);

				string msg{};
				Buttons buttons{};
				auto refresh = [&] {
					msg = maker.Overview(true);
					buttons.clear();
					buttons.add(RuleBuyCustomize);
					if (get_gold(Vanilla::Player()) >= maker.Cost()) {
						buttons.add(RuleBuyGold);
					}
					buttons.add(RulesBuyBase, "Back").add(Exit);
					return true;
				};
				if (!refresh()) {
					return Exit;
				}
				i32 res = WaitEVMMBV(msg, buttons);
				Next next = buttons.get_next(res);
				while (next == RuleBuyCustomize) {
					if (Next r = OpenRuleBuyCustomize(maker); r == RulesDetailsBase) { // Aka "Back"
						if (!refresh()) {
							return Exit;
						}
						res = WaitEVMMBV(msg, buttons);
						next = buttons.get_next(res);
					}
					else {
						return r;
					}
				}
				if (next == RuleBuyGold) {
					next = RulesBase;
					if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
						rules_info& prules = locked->player_rules();
						if (prules.AddGranter(maker)) {
							if (!remove_gold(Vanilla::Player(), maker.Cost())) {
								prules.RemoveGranter(maker.ID());
							}
						}
					}
				}
				return next;
			}
			static Next OpenRulesBuyBase() {
				string msg{};
				Buttons buttons{};
				lazy_vector<GranterID> rule_ids{};
				if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
					const rules_info& prules = locked->player_rules();
					msg = prules.GetGranterStrBase(0);
					rule_ids = prules.GetGranterIDs(true);
					buttons.add(RuleBuyPreview, rules_info::GranterStaticNames(rule_ids));
					buttons.add(RulesBase, "Back").add(Exit);
				} else {
					return Exit;
				}
				i32 res = WaitEVMMBV(msg, buttons);
				Next next = buttons.get_next(res);
				if (next == RuleBuyPreview) {
					next = OpenRuleBuyPreview(rule_ids[static_cast<size_t>(res)]);
				}
				return next;
			}
			static Next OpenRuleDetails(const GranterID id) {
				string msg{};
				static const Buttons buttons{ [] {
					Buttons btns{};
					btns.add(RulesDetailsBase, "Back").add(Exit);
					return btns;
				}() };
				if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
					msg = locked->player_rules().Overview(id);
				} else {
					return Exit;
				}
				Next n = buttons.get_next(WaitEVMMBV(msg, buttons));
				return n;
			}
			static Next OpenRulesDetailsBase() {
				string msg{};
				Buttons buttons{};
				lazy_vector<GranterID> rule_ids{};
				auto refresh = [&] {
					if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
						const rules_info& prules = locked->player_rules();
						msg = prules.GetGranterStrBase(0);
						rule_ids = prules.GetGranterIDs(true);
						buttons.clear();
						buttons.add(RuleDetails, rules_info::GranterStaticNames(rule_ids));
						buttons.add(RulesBase, "Back").add(Exit);
						return true;
					} else {
						return false;
					}
				};
				if (!refresh()) {
					return Exit;
				}
				i32 res = WaitEVMMBV(msg, buttons);
				Next next = buttons.get_next(res);
				while (next == RuleDetails) {
					if (Next r = OpenRuleDetails(rule_ids[static_cast<size_t>(res)]); r == RulesDetailsBase) { // Aka "Back"
						if (!refresh()) {
							return Exit;
						}
						res = WaitEVMMBV(msg, buttons);
						next = buttons.get_next(res);
					}
					else {
						return r;
					}
				}
				return next;
			}
			static Next OpenRulesBase() {
				string msg{};
				Buttons buttons{};
				if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
					const rules_info& prules = locked->player_rules();
					msg = prules.GetGranterStrBase(0);
					const bool has_granters = prules.HasGranters();
					if (has_granters) { buttons.add(RulesDetailsBase); }
					if (prules.CanAddGranter()) { buttons.add(RulesBuyBase); }
					if (has_granters) { buttons.add(RulesRemoveBase); }
					buttons.add(Base).add(Exit);
				} else {
					return Exit;
				}
				Next next = buttons.get_next(WaitEVMMBV(msg, buttons));
				switch (next) {
				case RulesDetailsBase: { return OpenRulesDetailsBase(); }
				case RulesBuyBase: { return OpenRulesBuyBase(); }
				case RulesRemoveBase: { return OpenRulesRemoveBase(); }
				default: { return next; }
				}
			}
			static Next OpenDodgeRecords() {
				string msg{};
				if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
					msg = locked->player_rules().RecordsString();
				} else {
					return Exit;
				}
				static const Buttons buttons{ { Base, Exit } };
				return buttons.get_next(WaitEVMMBV(msg, buttons));
			}
			static Next OpenDebugRules() {
				string msg{};
				Buttons buttons{};
				buttons.add(Base).add(Exit);
				if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
					msg = locked->player_rules().RulesBreakdown();
				} else {
					msg = "Player not blocked";
				}
				Next next = buttons.get_next(WaitEVMMBV(msg, buttons));
				return next;
			}
			static void OpenBase() {
				const string text{ [] {
					if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
						return locked->player_rules().RulesOverview();
					} else {
						return "Player not blocked"s;
					}
				}()};
				static const Buttons buttons{ { DebugRules, DodgeRecords, RulesBase, Exit } };
				Next next = Base;
				while (true) {
					switch (next) {
					case Base: { next = buttons.get_next(WaitEVMMBV(text, buttons)); break; }
					case DebugRules: { next = OpenDebugRules(); break; }
					case DodgeRecords: { next = OpenDodgeRecords(); break; }
					case RulesBase: { next = OpenRulesBase(); break; }
					case Exit: { return; }
					default: { Log::Error("MessageManager::OpenBase: Received invalid messagebox choice"sv); return; }
					}
				}
			}


			static void Start() {
				if (!locker.GetShared()->player_is_blocked()) {
					return;
				}
				std::thread(OpenBase).detach();
			}

		};

	}


	// Fear Papyrus functions
	namespace Functions {

		static float GetFear(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					// Log::Info("GetFear returning {} fear for {:08X}"sv, locked->fear(idx).fear.get(), act->formID);
					return locked->fear(idx).fear * 100.0f;
				}
				// Log::Warning("GetFear did not find {:08X}"sv, act->formID);
				// return -1.0f;
			}
			// Log::Warning("GetFear actor null"sv);
			return -1.0f;
		}
		static float GetThrillseeking(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->fear(idx).thrillseeking;
				}
			}
			return -1.0f;
		}
		static bool GetIsThrillseeker(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->fear(idx).thrillseeking >= 0.5f;
				}
			}
			return false;
		}
		static float GetFearsFemale(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->fear(idx).fears_female;
				}
			}
			return -1.0f;
		}
		static float GetFearsMale(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->fear(idx).fears_male;
				}
			}
			return -1.0f;
		}
		static float GetLastRestDay(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->fear(idx).last_rest_day.value_or(-1.0f);
				}
			}
			return -1.0f;
		}
		static string GetLastRestDayStr(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx) and locked->fear(idx).last_rest_day.has_value()) {
					return StringUtils::GetTimeString(locked->fear(idx).last_rest_day.value());
				}
			}
			return "NaN";
		}
		static float GetDaysSinceRest(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx) and locked->fear(idx).last_rest_day.has_value()) {
					return GameDataUtils::DaysPassed() - locked->fear(idx).last_rest_day.value();
				}
			}
			return -1.0f;
		}

		static float SetFear(StaticFunc, RE::Actor* act, const float value) {
			if (act) {
				const auto locked = locker.GetExclusive();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					locked->fear(idx).fear = (value * 0.01f);
					if (::Fear::FormsFilled()) {
						act->AddToFaction(::Fear::Faction(::Fear::FAC::Fear), locked->fear(idx).FearRank());
					}
					return locked->fear(idx).fear * 100.0f;
				}
			}
			return -1.0f;
		}
		static float SetThrillseeking(StaticFunc, RE::Actor* act, const float value) {
			if (act) {
				const auto locked = locker.GetExclusive();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					locked->fear(idx).thrillseeking = value;
					if (::Fear::FormsFilled()) {
						act->AddToFaction(::Fear::Faction(::Fear::FAC::Thrillseeking), locked->fear(idx).ThrillseekingRank());
						act->AddToFaction(::Fear::Faction(::Fear::FAC::Thrillseeker), locked->fear(idx).ThrillseekerRank());
					}
					return locked->fear(idx).thrillseeking;
				}
			}
			return -1.0f;
		}
		static float SetFearsFemale(StaticFunc, RE::Actor* act, const float value) {
			if (act) {
				const auto locked = locker.GetExclusive();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					locked->fear(idx).fears_female = value;
					if (::Fear::FormsFilled()) {
						act->AddToFaction(::Fear::Faction(::Fear::FAC::FearsFemale), locked->fear(idx).FearsFemaleRank());
					}
					return locked->fear(idx).fears_female;
				}
			}
			return -1.0f;
		}
		static float SetFearsMale(StaticFunc, RE::Actor* act, const float value) {
			if (act) {
				const auto locked = locker.GetExclusive();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					locked->fear(idx).fears_male = value;
					if (::Fear::FormsFilled()) {
						act->AddToFaction(::Fear::Faction(::Fear::FAC::FearsMale), locked->fear(idx).FearsMaleRank());
					}
					return locked->fear(idx).fears_male;
				}
			}
			return -1.0f;
		}

		static RE::Actor* GetMostAfraidActorInLocation(StaticFunc) { return Fear::GetMostAfraidActorInLocation(); }

		static constexpr string_view fear_script_name = "FunctionsFear"sv;
		static bool RegisterFearFunctions(IVM* vm) {
			vm->RegisterFunction("GetFear", fear_script_name, GetFear, true);
			vm->RegisterFunction("GetThrillseeking", fear_script_name, GetThrillseeking, true);
			vm->RegisterFunction("GetIsThrillseeker", fear_script_name, GetIsThrillseeker, true);
			vm->RegisterFunction("GetFearsFemale", fear_script_name, GetFearsFemale, true);
			vm->RegisterFunction("GetFearsMale", fear_script_name, GetFearsMale, true);
			vm->RegisterFunction("GetLastRestDay", fear_script_name, GetLastRestDay, true);
			vm->RegisterFunction("GetLastRestDayStr", fear_script_name, GetLastRestDayStr, true);
			vm->RegisterFunction("GetDaysSinceRest", fear_script_name, GetDaysSinceRest);

			vm->RegisterFunction("SetFear", fear_script_name, SetFear);
			vm->RegisterFunction("SetThrillseeking", fear_script_name, SetThrillseeking);
			vm->RegisterFunction("SetFearsFemale", fear_script_name, SetFearsFemale);
			vm->RegisterFunction("SetFearsMale", fear_script_name, SetFearsMale);

			// Old impl support, probably to prune later
			vm->RegisterFunction("GetMostAfraidActorInLocation", fear_script_name, GetMostAfraidActorInLocation, true);

			return true;
		}
	}

	// ExtraKeywords Papyrus functions
	namespace Functions {

		static bool HasAddedKeyword(StaticFunc, RE::TESForm* form, RE::BGSKeyword* kwd)							{ return ExtraKeywords::Data::Has(form, kwd); }
		static deque<bool> HasAddedKeywordsArray(StaticFunc, RE::TESForm* form, vector<RE::BGSKeyword*> kwds)	{ return ExtraKeywords::Data::Has(form, kwds); }
		static deque<bool> HasAddedKeywordsList(StaticFunc, RE::TESForm* form, RE::BGSListForm* list)			{ return ExtraKeywords::Data::Has(form, list); }
		static deque<bool> CrossKeywords(StaticFunc, RE::TESForm* form, RE::BGSListForm* list)					{ return ExtraKeywords::Data::CrossKeywords(form, list); }
		static bool AddKeyword(StaticFunc, RE::TESForm* form, RE::BGSKeyword* kwd)								{ return ExtraKeywords::Data::Add(form, kwd); }
		static bool RemoveKeyword(StaticFunc, RE::TESForm* form, RE::BGSKeyword* kwd)							{ return ExtraKeywords::Data::Remove(form, kwd); }
		static i32 ClearInvalidKeywordForms(StaticFunc)														{ return PrimitiveUtils::to_s32(ExtraKeywords::Data::ClearInvalids()); }

		static bool RegisterExtraKeywordsFunctions(IVM* vm) {
			vm->RegisterFunction("HasAddedKeyword", fear_script_name, HasAddedKeyword);
			vm->RegisterFunction("HasAddedKeywordsArray", fear_script_name, HasAddedKeywordsArray);
			vm->RegisterFunction("HasAddedKeywordsList", fear_script_name, HasAddedKeywordsList);
			vm->RegisterFunction("CrossKeywords", fear_script_name, CrossKeywords);
			vm->RegisterFunction("AddKeyword", fear_script_name, AddKeyword);
			vm->RegisterFunction("RemoveKeyword", fear_script_name, RemoveKeyword);
			vm->RegisterFunction("ClearInvalidKeywordForms", fear_script_name, ClearInvalidKeywordForms);

			return true;
		}

	}

	// PlayerRules Papyrus functions
	namespace Functions {
		static bool IsBlocked(StaticFunc, RE::Actor* act) {
			return act and locker.GetShared()->is_blocked(act);
		}
		static bool MakeBlocked(StaticFunc, RE::Actor* act) {
			return act and locker.GetExclusive()->set_blocked(act);
		}
		static void UnmakeBlocked(StaticFunc, RE::Actor* act) {
			act ? locker.GetExclusive()->unset_blocked(act) : void();
		}

		static bool GetCanRelax(StaticFunc, RE::Actor* act) {
			if (act) {
				if (const auto locked = locker.GetShared(); act->IsPlayerRef()) {
					return !locked->player_is_blocked() or locked->player_rules().CanRelax();
				} else {
					auto idx = locked->find_index(act);
					return !locked->is_valid(idx) or !locked->fear(idx).is_blocked;
				}
			}
			return true;
		}

		static float GetWillpower(StaticFunc) {
			if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
				return locked->player_rules().Exp().get();
			}
			return -1.0f;
		}

		static i32 GetEarnedReliefs(StaticFunc) {
			if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
				return locked->player_rules().EarnedReliefs().get();
			}
			return -1;
		}
		static bool SetEarnedReliefs(StaticFunc, i32 val) {
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				locked->player_rules().SetReliefs(val);
				return true;
			}
			return false;
		}
		static i32 AddEarnedReliefs(StaticFunc, i32 val) {
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				rules_info& prules = locked->player_rules();
				prules.AddReliefs(val);
				return prules.EarnedReliefs().get();
			}
			return -1;
		}

		static i32 GetDodgesLifetime(StaticFunc) {
			if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
				return locked->player_rules().Total().i32();
			}
			return -1;
		}
		static bool SetDodgesLifetime(StaticFunc, i32 val) {
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				locked->player_rules().Total(val);
				return true;
			}
			return false;
		}

		static i32 GetDodgesStreak(StaticFunc) {
			if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
				return locked->player_rules().Streak().i32();
			}
			return -1;
		}
		static bool SetDodgesStreak(StaticFunc, i32 val) {
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				locked->player_rules().Streak(val);
				return true;
			}
			return false;
		}

		static i32 GetDodgesBestStreak(StaticFunc) {
			if (const auto locked = locker.GetShared(); locked->player_is_blocked()) {
				return locked->player_rules().BestStreak().i32();
			}
			return -1;
		}
		static bool SetDodgesBestStreak(StaticFunc, i32 val) {
			if (const auto locked = locker.GetExclusive(); locked->player_is_blocked()) {
				locked->player_rules().BestStreak(val);
				return true;
			}
			return false;
		}


		static void OpenCrestLinkMsg(StaticFunc) { PlayerRules::Messaging::Start(); }



		static constexpr string_view rules_script_name = "FunctionsRules"sv;
		static bool RegisterRulesFunctions(IVM* vm) {

			vm->RegisterFunction("IsBlocked"sv, rules_script_name, IsBlocked, true);
			vm->RegisterFunction("MakeBlocked"sv, rules_script_name, MakeBlocked);
			vm->RegisterFunction("UnmakeBlocked"sv, rules_script_name, UnmakeBlocked);

			vm->RegisterFunction("GetCanRelax", rules_script_name, GetCanRelax, true);

			vm->RegisterFunction("GetWillpower"sv, rules_script_name, GetWillpower, true);

			vm->RegisterFunction("GetEarnedReliefs", rules_script_name, GetEarnedReliefs, true);
			vm->RegisterFunction("SetEarnedReliefs", rules_script_name, SetEarnedReliefs, true);
			vm->RegisterFunction("AddEarnedReliefs", rules_script_name, AddEarnedReliefs, true);

			vm->RegisterFunction("GetDodgesLifetime"sv, rules_script_name, GetDodgesLifetime, true);
			vm->RegisterFunction("SetDodgesLifetime"sv, rules_script_name, SetDodgesLifetime, true);

			vm->RegisterFunction("GetDodgesStreak"sv, rules_script_name, GetDodgesStreak, true);
			vm->RegisterFunction("SetDodgesStreak"sv, rules_script_name, SetDodgesStreak, true);

			vm->RegisterFunction("GetDodgesBestStreak"sv, rules_script_name, GetDodgesBestStreak, true);
			vm->RegisterFunction("SetDodgesBestStreak"sv, rules_script_name, SetDodgesBestStreak, true);

			vm->RegisterFunction("OpenCrestLinkMsg"sv, rules_script_name, OpenCrestLinkMsg, true);



			return true;
		}
	}

	// Shared Papyrus functions
	namespace Functions {

		static void NotifyDodge(StaticFunc, RE::Actor* act) {
			if (act and IsValidAddable(act)) {
				trivial_handle handle{ act };
				auto locked = locker.GetExclusive();
				if (size_t idx = locked->has_or_add(act); idx < locked->size()) {
					locked->fear(idx).buildup_mod += 1.0f;
					if (locked->player_is_blocked() and act->IsPlayerRef()) {
						locked->player_rules().Dodged();
					}
				}
			}
		}
		static void NotifyBrawl(StaticFunc, RE::Actor* act, RE::TESQuest* ctrl) {
			if (act and IsValidAddable(act)) {  // ctrl == nullptr is valid
				trivial_handle handle{ act };
				SLHelpers::sslThreadController_interface intfc{ ctrl };

				auto locked = locker.GetExclusive();

				if (size_t idx = locked->has_or_add(act); locked->is_valid(idx)) {
					const bool is_player = act->IsPlayerRef();
					const bool player_is_blocked = locked->player_is_blocked();
					if (is_player and player_is_blocked) {
						locked->player_rules().Rested();
					}
					FearInfo& fear = locked->fear(idx);
					fear.last_rest_day = GameDataUtils::DaysPassed();
					if (!fear.is_female and !is_player and player_is_blocked and intfc.IsValid() and act != intfc.PassiveActor()) { // Brawler is male non-player and non-passive
						PlayerRules::BrawlerHitDefender(locked->player_rules(), intfc.Tags(), fear.fear.get());
					}
				}
			}
		} 

		static float GetExposure(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->equipstate(idx).GetExposure();
				}
			}
			return -1.0f;
		}
		static bool GetIsNaked(StaticFunc, RE::Actor* act) {
			if (act) {
				const auto locked = locker.GetShared();
				if (auto idx = locked->find_index(act); locked->is_valid(idx)) {
					return locked->equipstate(idx).IsNaked();
				}
			}
			return false;
		}

		static constexpr string_view shared_script_name = "FunctionsShared"sv;
		static bool RegisterSharedFunctions(IVM* vm) {

			vm->RegisterFunction("NotifyDodge", shared_script_name, NotifyDodge);
			vm->RegisterFunction("NotifyBrawl", shared_script_name, NotifyBrawl);
			vm->RegisterFunction("GetExposure", fear_script_name, GetExposure, true);
			vm->RegisterFunction("GetIsNaked", fear_script_name, GetIsNaked, true);

			return true;
		}

		bool RegisterBasic(IVM* vm) { return RegisterSharedFunctions(vm) and RegisterRulesFunctions(vm); }
		bool RegisterFear(IVM* vm) { return RegisterFearFunctions(vm) and RegisterExtraKeywordsFunctions(vm); }
	}

}




		/*
															Notes on the Actor storage/serialization options

			There are several requirements:
				1:	It should be possible to store any reference Actor on game save, and retrieve it on game load as long as it still exists.
				2:	It should be possible to safely access any held Actor's data.
				3:	It should not enforce persistence of any held Actor for any longer than necessary.

			There are also several obstacles:
				1:	Like with all Forms, the only identifier of references that retains consistency between game loads is their FormID. It HAS to be what gets serialized as the identifier.
				2:	References can be deleted by the game. Storing Actor* to be able to access data on demand is unsafe because game CTDs if that's attempted after the game has deleted it.
					There are ways around it. TESFormDeleteEvent and SKSE Serialization's FormDeleteCallback provide means of notification when such deletion takes place.
					TESFormDeleteEvent seems to be firing only for references, while FormDeleteCallback fires for every VMHandle deletion. References only is sufficient.
					Listening to and synchronizing what is accessed with these apparently works, if with some alleged unreliability on FormDeleteCallback's side.
					Storing FormID and LookupByID() before every use, besides slow, would also technically not be entirely safe as a deletion could occur after the lookup but during use.
					Another way around it is ref counts. Each reference has an internal counter which will keep the game from deleting it while > 0 (apparently).
					Incrementing that counter, be it manually or via ActorPtr (the game's version of shared_ptr, complement to ActorHandle's being the weak_ptr), should also ensure safe access.
				3:	Storing a persistent VMHandle for each Actor to work with FormDeleteCallback would enforce persistence (right? or what if not persistent VMHandle?).
					The assosciative storing methods would have to store raw pointers (?) and listen to their respective event to discard deleted ones. Locks would have to ensure synchronicity.
					Incrementing ref count upon obtaining an Actor would enforce persistence as well. Using them should be done through ActorHandle, requesting ActorPtr before use.

			The Actors we hold come through 3 places:
				1:	Through high Process Lists via scanning during updates. This can produce ActorPtrs and ensure validity (or safely checkable validity) upon receipt.
				2:	Through Papyrus via mod events. Papyrus objects are persistent during their lifetime so simply filtering received nullptrs should suffice to ensure validity.
				3:	Through other parts of the plugin via data requests. That's up to me and my contracts. Enforcable by communicating with ActorPtrs, for example.
				4:	For EquipState, they can come through via TESEquipEvent in the form of NiPointer<TESObjectREFR> which is enough to check and ensure validity

			Before designing a safe model that satisfies the requirements, I have to make some assumptions:
				1:	The deletion of an actor will not take place until all the TESFormDeleteEvent/FormDeleteCallback listeners have returned.
					This should be a safe assumption to make, since it both makes sense and is apparently used and accepted by people over at SkyrimSE RE Discord, like PO3.
						Update: Probably wrong. At least for TESFormDeleteEvent, deletions do slip through and cause CTDs. FormDeleteCallback not thoroughly tested.
				2:	Obtaining a non-nullptr ActorPtr from an ActorHandle should ensure that the Actor will not be deleted while the ActorPtr is in scope. Aka, typical smart pointer behavior.
					This is also a safe assumption to make because why would Bethesda do it any other way? Working with ActorHandles during runtime is also generally accepted on the Discord.
						Update: This does actually seem to hold.

			So, the models I can come up with are the following:
				1:	Data map of <Actor*, FearInfo>, and a <FormID, Actor*> tracker map as an erase facilitator. Each map with its own lock. Listen to TESFormDeleteEvent.
					Every time an Actor is to be added, its <FormID, Actor*> gets inserted in the tracker map and its <Actor*, FearInfo> gets inserted in the data map.
						These should be synchronized: either both insertions happen or none.
					Every time TESFormDeleteEvent fires, the tracker map is searched the for the FormID and, if found, its Actor* is erased from the data map and it from the tracker map.
						The data map erasure must succeed. If it fails, report and throw. Leaving deleted actors in data will result in ungraceful CTD.
						It is not critical for the tracker map erasure to succeed. Simply skip data map erasures if Actor* not found when handling TESFormDeleteEvent callback.
					Advantages:
						Offers the fastest acquisition of a safely usable Actor* by virtue of all the Actor* keys on the data map being guaranteed not to have been deleted while under its lock.
						TESFormDeleteEvent is not that spammy. This reduces the odds of the tracker lock being taken when a new fire is received.
							This is assuming a new TESFormDeleteEvent can be sent before the old one returns. This is unlikely, but it's still good to minimize lock hammering.
					Disadvantages:
						Cannot recover from failure to erase a deleted element.
						Ugly to implement, with its multiple data structures/locks.
						There will still be a decent amount of callbacks happening. While that will only impact PlayerRules operations if an erasure must happen, it's still code running.
						While TESFormDeleteEvent seems to work and has been recommended for use whe handling references, I have yet to test it myself.
							Update: I can consistently get crashes with this method. Either I'm fucking something up, which I don't see how I am, or it is just unreliable.
				2:	Data map of <Actor*, FearInfo>, and a <VMHandle, Actor*> tracker map as an erase facilitator. Each map with its own lock. Listen to FormDeleteCallback.
					Every time an Actor is to be added, its <VMHandle, Actor*> gets inserted in the tracker map and its <Actor*, FearInfo> gets inserted in the data map.
						These should be synchronized: either both insertions happen or none.
					Every time FormDeleteCallback fires, the tracker map is searched the for the VMHandle and, if found, its Actor* is erased from the data map and it from the tracker map.
						The data map erasure must succeed. If it fails, report and throw. Leaving deleted actors in data will result in ungraceful CTD.
						It is not critical for the tracker map erasure to succeed. Simply skip data map erasures if Actor* not found when handling FormDeleteCallback callback.
					Advantages:
						Offers the fastest acquisition of a safely usable Actor* by virtue of all the Actor* keys on the data map being guaranteed not to have been deleted while under its lock.
					Disadvantages:
						Cannot recover from failure to erase a deleted element.
						Even uglier to implement, with its multiple data structures/locks and VMHandle persistence handling and stuff.
						When adding, VMHandles have to be obtained by an IObjectHandlePolicy, which has to be obtained by an IVirtualMachine.
							Looks like we're cosplaying Papyrus. SKSE Registration containers set the handle as persistent. AFAIU, I don't want to do this.
						FormDeleteCallback gets spammed to hell and back. It will only impact PlayerRules operations if an erasure must happen, it's still a lot of calls.
						As with TESFormDeleteEvent, I have yet to test it myself. I've heard of some unreliability on the Discord.
				3:	New struct K of holding an ActorHandle and an Actor* or a FormID. Data map of <K, FearInfo>. No other maps, so a single lock. No listeners.
					Every time an Actor is to be added and is not somehow sure to be valid, construct ActorHandle with the pointer and get ActorPtr from it. Check ActorPtr for validity.
					Every time an actor needs to be used, get the ActorPtr from the stored ActorHandle in the key. Check and use that.
					Advantages:
						Complete lack of reliance on getting usable data. Both Actor* and FormID can be safely converted to ActorPtr, which will ensure lifetime in its scope if not nullptr.
							Attempting to create an ActorHandle from a deleted Actor* will make a handle (possibly 0) which will produce nullptr ActorPtr. No crashes.
						No listeners. Saves a lot of code running, especially from FormDeleteCallback.
						Arguably the simplest to implement and reason with. No multiple locks to track or maps to synchronize.
					Disadvantages:
						A call to ActorHandle.get() and a check against nullptr has to be made in every scope an Actor needs to be used. This adds a slight overhead compared to the other methods.
							But it seems to be the only safe way. Calls to ActorHandle.get() seem consistently fast (<1us, likely a hashmap lookup?) and the nullptr check can be branch-predicted.
				4:	Flattened version of 3. Two index-matching vectors, vector<ActorHandle> and vector<FearInfo>, are sufficient and are faster for iterations.
					FormID used only for serialization. Validity is ensured, like in 3, by getting an ActorPtr fron the ActorHandle every time before use.
					Lookups are faster, for up to hundreds/thousands of elements, depending on machine. Can use some kind of "hot" system to make lookups always fast. Very optimizable overall.
						Further optimizable by adding an ActorHandle member in FearInfo, allowing for a single vector. Basically like a vector of pairs.
						Prefetcher will detect the stride, allowing lookups to be basically as fast as iterating over a vector<ActorHandle>.


									Measuring different lookup methods:
						The static Actor::LookupByHandle() takes <1us to run. steady_clock cannot measure it.
						ActorHandle.get() also seems to take <1us to run. steady_clock cannot measure it either.
						LookupByID<RE::Actor>() takes 1.5-2.5us to run, and only provides an Actor*, which is not guaranteed to be valid after the lookup returns.

									Fear loops every 2s and needs to procure a list of nearby actors. Getting ActorPtrs via high Process Lists is probably faster than scanning cell.
									PlayerRules loops every 9s and has to use its actors. Looking up ActorPtrs via the handles adds <1us per actor per 9s. Unlikely to make a difference.
									EquipState doesn't loop and also gets changed almost entirely through the TESEquipEvent, which provides a NiPointer itself.
										When queried by other parts of the plugin, it can expect to only be fed identifiers to valid actors (probably Actor*).
									ExtraKeywords never uses reference pointers.
		*/




