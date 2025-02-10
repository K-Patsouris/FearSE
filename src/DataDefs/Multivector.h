#pragma once
#include "FearInfo.h"
#include "PlayerRules.h"
#include "EquipState.h"
#include "Forms/VanillaForms.h"
#include "Forms/RulesForms.h"

#include <ranges>


namespace Data {

	class multivector {
		enum : size_t { MaxUpdateCount = UpdateTypes::MaxUpdateCount };
		using ranks_and_oneofs = UpdateTypes::ranks_and_oneofs;
	public:
		explicit constexpr multivector() noexcept = default;
		constexpr multivector(const multivector&) = default;
		constexpr multivector(multivector&&) noexcept = default;
		constexpr multivector& operator=(const multivector&) = default;
		constexpr multivector& operator=(multivector&&) noexcept = default;
		constexpr ~multivector() noexcept = default;


		constexpr size_t find_index(const trivial_handle hnd) const noexcept { return static_cast<size_t>(handles.find(hnd) - handles.begin()); }
		constexpr bool is_valid(const size_t idx) const noexcept { return idx < size(); }

		constexpr decltype(auto) fear(this auto& self, const size_t idx) noexcept { return self.fears[idx]; }
		constexpr decltype(auto) equipstate(this auto& self, const size_t idx) noexcept { return self.equips[idx]; }

		constexpr lazy_vector<trivial_handle>& all_handles() noexcept { return handles; }
		constexpr lazy_vector<FearInfo>& all_fears() noexcept { return fears; }
		constexpr const lazy_vector<EquipState>& all_equips() const noexcept { return equips; }

		constexpr bool is_blocked(const trivial_handle hnd) const noexcept {
			if (const size_t idx = find_index(hnd); idx < handles.size()) {
				return fears[idx].is_blocked;
			}
			return false;
		}
		bool set_blocked(RE::Actor* act) noexcept {
			const bool isplayer = act->IsPlayerRef();
			const trivial_handle handle{ isplayer ? Vanilla::PlayerHandle() : act };
			const size_t idx = find_index(handle);
			if (idx == size()) { // Not registered
				if (!reserve_all(1)) {
					return false; // Couldn't allocate to register
				}
				handles.append(handle);
				fears.append(act, true);
				equips.append(act);
			}
			return fears[idx].is_blocked or (fears[idx].is_blocked = isplayer ? add_rulesplayer_spells(act) : add_rulesnpc_spells(act));
		}
		void unset_blocked(RE::Actor* act) noexcept {
			const bool isplayer = act->IsPlayerRef();
			if (const size_t idx = find_index(isplayer ? Vanilla::PlayerHandle() : act); idx < size()) {
				fears[idx].is_blocked = false;
				if (isplayer) {
					remove_rules_player_spells(act);
				} else {
					remove_rules_npc_spells(act);
				}
			}
		}
		constexpr bool player_is_blocked() const noexcept {
			if (const auto idx = find_index(Vanilla::PlayerHandle()); idx < size()) {
				return fears[idx].is_blocked;
			}
			return false;
		}
		constexpr auto& player_rules(this auto& self) noexcept { return self.prules; }


		size_t has_or_add(RE::Actor* act) noexcept {
			trivial_handle handle{ act };
			const size_t old_size = size();
			if (const size_t idx = find_index(handle); idx < old_size) {
				return idx; // Existing
			}
			if (reserve_all(1)) {
				handles.append(handle);
				fears.append(act, true);
				equips.append(act);
			}
			return old_size; // Always return this here. If added, it is the index to it. If not, it is handles.size().
		}

		void erase(const size_t idx) noexcept {
			handles.erase(idx);
			fears.erase(idx);
			equips.erase(idx);
		}

		bool swap_allocate_move(array<trivial_handle, MaxUpdateCount>& handles_buffer, array<RE::ActorPtr, MaxUpdateCount>& ptrs_buffer, ranks_and_oneofs& pack) noexcept {
			// Abomination of a function that implements a very specific MRU sorting. Probably should be simplified at some point.
			
			using std::ranges::subrange;
			using std::ranges::sort;
			using std::ranges::views::zip;

			const u32 actor_count = pack.actor_count;

			const subrange hnds{ handles_buffer.begin(), handles_buffer.begin() + actor_count };
			const subrange ptrs{ ptrs_buffer.begin(), ptrs_buffer.begin() + actor_count };

			auto tuple_comp = [](const auto& a, const auto& b) -> bool { return std::get<0>(a) < std::get<0>(b); };

			// Find indexes and sort them with input
			array<u32, 32> index_buffer{ [&]() {
				array<u32, 32> idxs{};
				subrange ir{ idxs.begin(), idxs.begin() + actor_count };
				std::transform(hnds.begin(), hnds.end(), ir.begin(), [this](const trivial_handle h) { return static_cast<u32>(find_index(h)); });	// Find indexes of all input handles
				sort(zip(ir, hnds, ptrs), tuple_comp);	// Sort indexes and ptrs based on indexes only
				return idxs;
			}()};
			const subrange idxs{ index_buffer.begin(), index_buffer.begin() + actor_count };

			// Find registered and unregistered in sorted input (by just bounds-checking sorted idxs)
			const u32 registered_count = [&idxs, max = static_cast<u32>(handles.size())] {
				u32 idx = 0;
				for (; (idx < idxs.size()) and (idxs[idx] < max); ++idx) { ; }
				return idx;
			}();
			const u32 unregistered_count = actor_count - registered_count;
			pack.unregistered_count = unregistered_count;

			// Swap registered to start, if there are any, and if at least 1 of them needs to be moved
			if (const subrange reg_idxs{ idxs.begin(), idxs.begin() + registered_count }; (registered_count != 0) and (reg_idxs.back() >= registered_count)) {
				const u32 i_max = registered_count;
				
				array<u32, MaxUpdateCount> swaps( [=]() { // Find missing idxs to fill
					array<u32, MaxUpdateCount> swaps{};
					auto reg_it = reg_idxs.begin(), swap_it = swaps.begin();
					for (u32 elem = 0; elem < i_max; ++elem) {
						if (elem != *reg_it) {
							*(swap_it++) = elem;
						} else {
							++reg_it;
						}
					}
					return swaps;
				}() );

				const auto smaller_end = [=] { // With input length at most 32, binary search will never beat this
					auto it = reg_idxs.begin();
					while (*it < i_max) { // Guaranteed to break before it == end() because here we know at least the last element is not < i_max, so don't check
						++it;
					}
					return it; // it now points to the first element not < i_max
				}();

				for (auto next_swap = swaps.begin(), next_bigger = smaller_end, reg_idxs_end = reg_idxs.end(); next_bigger != reg_idxs_end; ++next_bigger, ++next_swap) {
					this->swap(*next_bigger, *next_swap); // Swap a bigger to its new index
					*next_bigger = *next_swap;	// And update its old recorded index to that new index
				}
				sort(zip(reg_idxs, subrange{ hnds.begin(), hnds.begin() + registered_count }, subrange{ ptrs.begin(), ptrs.begin() + registered_count }), tuple_comp);
			}

			// Let swaps always happen. Even if this fails, chances are most of the current actors will be in the next one, so the ordering isn't wasted.
			if (unregistered_count != 0) {
				const u32 oldsize = static_cast<u32>(handles.size());
				const u32 remaining = oldsize - registered_count; // Count of elements existing but not in update
				const u32 newsize = oldsize + unregistered_count;
				if (!resize_all(newsize)) {
					pack.actor_count = registered_count; // Ignore unregistereds
					return false; // No need to init in main
				}
				const u32 count_to_move = std::min(remaining, unregistered_count);
				const u32 src_offset = registered_count;
				if (count_to_move != 0) {
					const u32 dst_offset = newsize - count_to_move;
					std::memcpy(handles.begin() + dst_offset, handles.begin() + src_offset, count_to_move * sizeof(trivial_handle));
					std::memcpy(fears.begin() + dst_offset, fears.begin() + src_offset, count_to_move * sizeof(FearInfo));
					std::memcpy(equips.begin() + dst_offset, equips.begin() + src_offset, count_to_move * sizeof(EquipState));
				}
				auto out_it = handles.begin() + src_offset;
				for (auto in_it = (hnds.begin() + registered_count); in_it != hnds.end(); ++in_it, ++out_it) {
					*out_it = *in_it; // Can initialize handles now. No need for main thread.
				}
				return true; // Need to init in main
			} else {
				return false; // No need to init in main
			}
		}
		void init_news(array<RE::ActorPtr, MaxUpdateCount>& ptrs_buffer, const ranks_and_oneofs& pack, const bool allow_fear_writes) noexcept {
			const u32 actor_count = pack.actor_count;
			const u32 unregistered_count = pack.unregistered_count;
			const u32 registered_count = actor_count - unregistered_count;

			const std::ranges::subrange ptrs{ ptrs_buffer.begin() + registered_count, ptrs_buffer.begin() + actor_count};

			// Log::Info("init_news initializing {} actors"sv, ptrs.size());

			auto ars_it = fears.begin() + registered_count;
			for (auto& ptr : ptrs) {
				(ars_it++)->InitData(ptr.get(), allow_fear_writes);
			}
			auto eqp_it = equips.begin() + registered_count;
			for (auto& ptr : ptrs) {
				(eqp_it++)->InitData(ptr.get());
			}
		}
		void get_exposures(array<float, MaxUpdateCount>& exposures, const ranks_and_oneofs& pack) const noexcept {
			for (u32 i = 0, end = pack.actor_count; i < end; ++i) {
				exposures[i] = equips[i].GetExposure();
			}
		}


		void clear_zero_handles() noexcept {
			const size_t max = handles.size();
			if (max == 0) {
				return; // No handles to begin with. First retreat_to_next_valid() call would actually catch this (and needlessly clear()) but this is more obvious.
			}

			auto advance_to_next_invalid = [this, max](size_t& idx) {
				if (idx < max) {
					while ((++idx < max) and (handles[idx].is_valid())) { ; }
				}
				return idx < max;
			};
			auto retreat_to_next_valid = [this](size_t& idx) {
				if (idx == 0) {
					return false;
				}
				while (!handles[--idx].is_valid()) {
					if (idx == 0) {
						return false;
					}
				}
				return true;
			};

			size_t idx_inv = 0;
			size_t idx_val = max;

			if (handles[idx_inv].is_valid() and !advance_to_next_invalid(idx_inv)) {
				return; // All handles are valid
			}
			if (!retreat_to_next_valid(idx_val)) {
				clear();
				return; // All handles are invalid
			}

			// From here on, since not all handles are valid and not all handles are invalid, there must exist at least 2 elements.

			while (idx_inv < idx_val) {
				handles[idx_inv] = handles[idx_val];
				fears[idx_inv] = std::move(fears[idx_val]);
				equips[idx_inv] = std::move(equips[idx_val]);
				advance_to_next_invalid(idx_inv);
				retreat_to_next_valid(idx_val);
			}

			// Since everything below idx_inv has been filled, idx_val has retreated to the next highest valid, and idx_inv >= idx_val (loop broke), idx_val is now the index of the last element.
			resize_all(idx_val + 1); // So add 1 to get the vectors size.
		}


		constexpr size_t size() const noexcept { return handles.size(); }
		
		constexpr void clear() noexcept {
			handles.clear();
			fears.clear();
			equips.clear();
		}

		void swap(const size_t idx1, const size_t idx2) noexcept {
			if (idx1 != idx2) {
				handles[idx1].swap(handles[idx2]);
				fears[idx1].Swap(fears[idx2]);
				equips[idx1].Swap(equips[idx2]);
			}
		}


		bool Save(SKSE::SerializationInterface& intfc) const {
			enum : RE::FormID { InvalidFormID = 0x0 };
			const size_t cursize = handles.size();

			lazy_vector<RE::FormID> formIDs{};
			if (!formIDs.reserve(cursize)) {
				Log::Critical("Failed to initialize serialization bookkeeping! (out of memory?)"sv);
				return false;
			}
			size_t valid_count = 0;
			for (size_t i = 0; i < cursize; ++i) {
				if (const RE::ActorPtr ptr = handles[i].get(); ptr and IsValidKeepable(ptr.get())) {
					++valid_count;
					formIDs.append(ptr->GetFormID());
				} else {
					formIDs.append(InvalidFormID);
				}
			}

			if (!intfc.WriteRecordData(valid_count)) {
				Log::Critical("Failed to serialize actor count!"sv);
				return false;
			}

			for (size_t i = 0; i < cursize; ++i) {
				if (const RE::FormID formID = formIDs[i]; formID != InvalidFormID) {
					if (!intfc.WriteRecordData(formID) or !fears[i].Save(intfc) or !equips[i].Save(intfc)) {
						Log::Critical("Failed to serialize data for formID <{:08X}>!"sv, formID);
						return false;
					}
				}
			}

			if (!intfc.WriteRecordData(prules)) {
				Log::Critical("Failed to serialize player rules data!"sv);
				return false;
			}

			Log::Info("Serialized actor data"sv);
			return true;
		}
		bool Load(SKSE::SerializationInterface& intfc) {
			clear();

			size_t data_size;
			if (!intfc.ReadRecordData(data_size)) {
				Log::Critical("Failed to deserialize actor count!"sv);
				return false;
			}
			lazy_vector<RE::FormID> formIDs{};
			if (!handles.reserve(data_size) or !fears.reserve(data_size) or !equips.reserve(data_size)) {
				Log::Critical("Not enough memory to deserialize data!"sv);
				return false;
			}

			for (size_t i = 0; i < data_size; ++i) {
				RE::FormID formID;
				FearInfo ars;
				EquipState eqs;
				if (!intfc.ReadRecordData(formID) or !ars.Load(intfc) or !eqs.Load(intfc)) {
					Log::Critical("Failed to deserialize actor data (potential unresolved formID <{:08X}>)!"sv, formID);
					return false;
				}
				if (intfc.ResolveFormID(formID, formID)) { // Buffer always progressed, even if the formID is invalid in this load.
					const RE::ActorHandle handle{ RE::TESForm::LookupByID<RE::Actor>(formID) }; // nullptr lookup just makes a 0 handle, which produces nullptr through .get(). No crashes, just check.
					if (RE::ActorPtr ptr = handle.get(); ptr and IsValidAddable(ptr.get())) {
						handles.append(handle);
						fears.append(std::move(ars)); // Move unnecessary atm, since these are both trivially copyable, but in case it changes...
						equips.append(std::move(eqs));
					}
				}
			}

			if (!intfc.ReadRecordData(prules)) {
				Log::Critical("Failed to deserialize player rules data!"sv);
				return false;
			}

			Log::Info("Deserialized data for {} actors. The remaining {}, serialized previously, are no longer valid."sv, handles.size(), (data_size - handles.size()));
			return true;
		}

	private:
		constexpr bool reserve_all(const size_t extra_count) noexcept {
			const size_t newcap = handles.size() + extra_count;
			return (newcap <= handles.capacity()) or (handles.reserve(newcap) and fears.reserve(newcap) and equips.reserve(newcap));
		}
		constexpr bool resize_all(const size_t newsize) noexcept {
			return (newsize == size()) or (handles.resize(newsize) and fears.resize(newsize) and equips.resize(newsize));
		}

		lazy_vector<trivial_handle> handles{};
		lazy_vector<FearInfo> fears{};
		lazy_vector<EquipState> equips{};
		rules_info prules{};

		static __forceinline bool add_rulesnpc_spells(RE::Actor* act) noexcept { return GameDataUtils::AddSpell(act, ::PlayerRules::Spell(::PlayerRules::SPL::PlayerRules), false); };
		static __forceinline void remove_rules_npc_spells(RE::Actor* act) noexcept { act->RemoveSpell(::PlayerRules::Spell(::PlayerRules::SPL::PlayerRules)); };
		static __forceinline bool add_rulesplayer_spells(RE::Actor* act) noexcept {
			if (add_rulesnpc_spells(act)) {
				if (!GameDataUtils::AddSpell(act, ::PlayerRules::Spell(::PlayerRules::SPL::CrestLink), true)) {
					remove_rules_npc_spells(act);
					return false;
				}
			}
			return true;
		};
		static __forceinline void remove_rules_player_spells(RE::Actor* act) noexcept {
			remove_rules_npc_spells(act);
			act->RemoveSpell(::PlayerRules::Spell(::PlayerRules::SPL::PlayerRules));
		};

	};

}
