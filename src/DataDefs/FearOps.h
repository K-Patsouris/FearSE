#pragma once
#include "FearInfo.h"
#include "EquipState.h"
#include "Utils/OutUtils.h"		// SendModEvent after update
#include "Forms/VanillaForms.h"


namespace Data::Fear {
	using OutUtils::SendModEvent;
	using PrimitiveUtils::bool_extend;
	using PrimitiveUtils::mask_float;


	std::atomic<float> last_update_day{ 0.0f };

	std::atomic<RE::Actor*> most_afraid{ nullptr };	// For old FEAR compatibility.

	std::atomic<bool> hostile_location{ false };		// Gets set to true when player enters hostile location, and to false when player enters any other location.
	std::atomic<bool> interior_cell{ false };			// Gets set to true when player enters an interior cell, and to false when player enters any other cell.
	static_assert(std::atomic<float>::is_always_lock_free and std::atomic<RE::Actor*>::is_always_lock_free and std::atomic<bool>::is_always_lock_free);



	void PlayerChangedCell(const RE::TESObjectCELL* newcell) noexcept { interior_cell.store(newcell->IsInteriorCell(), std::memory_order_relaxed); }
	void PlayerChangedLocation(const RE::BGSLocation* newloc) noexcept {
		if (newloc) { // This can expect nullptr. Many cells, like many exteriors, have no location. Treat those as non-hostile.
			if (const auto kwd_form = newloc->As<RE::BGSKeywordForm>(); kwd_form) {
				for (const auto& kwd : Vanilla::HostileLocKeywordss()) {
					if (kwd_form->HasKeyword(kwd)) {
						hostile_location.store(true, std::memory_order_relaxed);
						return;
					}
				}
			}
		}
		hostile_location.store(false, std::memory_order_relaxed); // If not for sure hostile, assume not hostile.
	}

	using UpdateTypes::main_out;
	using UpdateTypes::ranks_and_oneofs;
	enum : size_t { MaxUpdateCount = UpdateTypes::MaxUpdateCount };
	// Treats [0] as Player's stuff
	void Update(const milliseconds delta,
				lazy_vector<FearInfo>& infos,
				const lazy_vector<EquipState>& equips,
				const array<main_out, MaxUpdateCount>& mains,
				const array<RE::ActorPtr, MaxUpdateCount>& actptrs,
				ranks_and_oneofs& pack) noexcept
	{
		static RNG::gamerand rnd{ std::random_device{}() };

		const u32 actor_count = pack.actor_count;

		if ((actor_count == 0) or (delta <= 0ms)) {
			return;
		}
		float day_delta{};
		if (const float now = pack.now; now > 0.0f) {
			day_delta = now - last_update_day.exchange(now, std::memory_order_relaxed);
		}

		const array<float, 32> exposures{ [&] {
			array<float, 32> ret{};
			for (u32 i = 0; i < actor_count; ++i) {
				ret[i] = equips[i].GetExposure();
			}
			return ret;
		}()};
		
		const bool need_ranks = pack.need_ranks;
		const float interior = 1.0f + interior_cell.load(std::memory_order_relaxed);			// 1.0f or 2.0f
		const float hostile = 1.0f + (0.5f * hostile_location.load(std::memory_order_relaxed));	// 1.0f or 1.5f
		const float now = pack.now;
		float highest_fear = 0.0f, player_tension = 0.0f;
		u32 most_afraid_idx = 0;
		for (u32 i = 0; i < actor_count; ++i) {
			auto& info = infos[i]; // Convenience
			const float days_since_rest = now - info.last_rest_day.value_or(0.0f); // Treat rest-less as 0.0f

			const float buildup = [isplayer = i == 0, buildup_mod = std::sqrt(info.buildup_mod.get()), days_since_rest, &player_tension] {
				float ret = std::max(days_since_rest, isplayer ? days_since_rest : 7.0f); // NPC cap at a week, because they most go without for long times
				if (ret > 1.0f) {
					ret = std::sqrt(ret); // Introduce diminishing growth above 1. Let before be linear.
				}
				ret *= buildup_mod;
				player_tension += mask_float(ret, isplayer); // Add non-zero only for the player. Avoids branching. Put in here to ditch isplayer variable
				return ret;
			}();
			info.buildup_mod -= 1.0f; // Remove 1 every however often the updates are. For reference, dodging adds 1.

			float gain = 0.0f;
			float loss = 0.0f;

			// Factor in HP ratio (-)
			const auto hppcnt = mains[i].hppcnt; // Ensure single read
			loss += (4.0f - (0.0004f * hppcnt * hppcnt)) * hostile;	// 0 to 4 exponentially as HP decreases, increased by 50% if in hostile location
			// Factor in combat state (-)
			loss += mains[i].combat * interior;			// 0 not in combat, 1 in combat outside, 2 in combat inside

			// Calculate seen by and seeing stuff
			u8 seen_by_m = 0;
			u8 seen_by_f = 0;
			float seeing_exposure_m = 0.0f;
			float seeing_exposure_f = 0.0f;
			const auto seeing = mains[i].seeing; // Ensure single read
			for (u32 j = 0; j < actor_count; ++j) {
				if ((i != j) bitand seeing[j]) { // Only affected by those one sees
					const bool male = !infos[j].is_female;
					const float exp_base = exposures[j] + infos[j].in_brawl; // Seeing someone in scene just adds 1-2 instead of 0-1, based on exposure.
					const float exp_m = mask_float(exp_base, male);
					seeing_exposure_m += exp_m;
					seeing_exposure_f += exp_base - exp_m; // If exp_m is 0, then female, so add base. If exp_m is base, then male, so add 0. Trades a mask calculation for a subtraction.
					if (mains[j].seeing[i]) { // Mutual seeing
						seen_by_m += male;
						seen_by_f += !male;
					}
				}
			}
			// Factor in seen by (+)
			const u8 inscene = info.in_brawl; // Ensure single read
			const float thrill_mod = exposures[i] * info.thrillseeking, fears_male = info.fears_male, fears_female = info.fears_female; // Ensure single read
			gain += thrill_mod * fears_male * std::sqrtf(seen_by_m <<= inscene); // Consider being seen twice as effective if in scene
			gain += thrill_mod * fears_female * std::sqrtf(seen_by_f <<= inscene);
			// Factor in seeing (+/-)
			const float seeing_m_scaled = std::sqrt(seeing_exposure_m);
			const float seeing_f_scaled = std::sqrt(seeing_exposure_f);
			const bool dislike_m = fears_male < 0.5f;
			const bool dislike_f = fears_female < 0.5f;
			gain += mask_float(seeing_m_scaled * fears_male, !dislike_m);
			gain += mask_float(seeing_f_scaled * fears_female, !dislike_f);
			loss += mask_float(seeing_m_scaled * (1.0f - fears_male), dislike_m bitand static_cast<bool>(dislike_f bitor (seeing_f_scaled == 0.0f)));
			loss += mask_float(seeing_f_scaled * (1.0f - fears_female), dislike_f bitand static_cast<bool>(dislike_m bitor (seeing_m_scaled == 0.0f)));

			/*
				Like?		Always gain.
				Dislike?	Loss if not seeing anyone liked.
				If not seeing anything of a category, that should result in 0 gain or loss.

				seeF	seeM	likeF	likeM	resF/resM
				0		0		0		0		0/0 (no presence is noop)
				0		0		0		1		0/0
				0		0		1		0		0/0
				0		0		1		1		0/0

				1		0		0		0		L/0
				0		1		0		0		0/L
				1		1		0		0		L/L

				1		0		0		1		L/0
				1		0		1		0		G/0
				1		0		1		1		G/0
				
				0		1		0		1		0/G
				0		1		1		0		0/L
				0		1		1		1		0/G

				1		1		0		1		0/G
				1		1		1		0		G/0
				1		1		1		1		G/G
			*/
			
			// Maybe increase thrillseeking, AFTER the old value has been used for calculations
			const bool inc_thrill = rnd.nextf01() < (exposures[i] * ((seen_by_m + seen_by_f) >> 4)); // So, seen count / 16 (or * 0.0625), scaled from 0 to self depending on exposure
			info.thrillseeking += mask_float(0.01f, inc_thrill); // Add a fixed amount of thrillseeking. Prevents rapid growth for excessive streakers but behaves just like a curve in the long run.

			// Pick base value based on buildup, scale gains and losses according to buildup, and calculate target (unclamped) fear
			const float base = 0.5f - (0.5f / std::sqrt(days_since_rest + 1.0f)); // ~.15 at 1 day, ~.21 at 2, ~.32 at a week, ~.37 at 2, ~.41 at a month, ~.43 at 6
			const float gainmod = gain * buildup; // Increae with buildup
			const float lossmod = loss / (buildup + FLT_MIN); // Decrease with buildup. Avoid division by 0 by flat-out adding the min positive float value.
			const float target_fear = base + gainmod + lossmod;

			// Move halfway towards new fear
			const float old_fear = info.fear; // Ensure single read
			const float raw_new_fear = old_fear + (0.5f * (target_fear - old_fear)); // 0.5 step:	1:50%,  2:75%,  3:87.5%,  4:93.75%,  5:96.875%
			info.fear = raw_new_fear; // info.fear saturates in [0,1]

			if (need_ranks) { // Maybe will change once or twice per playthough. It's basically asking if the user uses OD's FEAR reqork. So, basically always predicted.
				auto& ranks = pack.ranks[i];
				ranks[0] = info.FearRank();
				ranks[1] = info.ThrillseekingRank();
				ranks[2] = info.ThrillseekerRank();
			}

			most_afraid_idx += static_cast<u32>((i - most_afraid_idx) bitand bool_extend<u32>(raw_new_fear >= highest_fear)); // Use unclamped for some potential tie-breaking
		}

		pack.player_tension = player_tension;

		most_afraid.store(actptrs[most_afraid_idx].get(), std::memory_order_relaxed);
		OutUtils::SendModEvent("fear_UpdateComplete", "", 0.0f, nullptr);
	}



	RE::Actor* GetMostAfraidActorInLocation() noexcept { return most_afraid.load(std::memory_order_relaxed); }
	

	bool Save(SKSE::SerializationInterface& intfc) noexcept {
		if (!intfc.WriteRecordData(last_update_day.load(std::memory_order_relaxed))) {
			Log::Critical("Failed to serialize last_update_day for Fear!"sv);
			return false;
		}
		return true;
	}

	bool Load(SKSE::SerializationInterface& intfc) noexcept {
		float temp;
		if (!intfc.ReadRecordData(temp)) {
			Log::Critical("Failed to deserialize last_update_day for Fear!"sv);
			return false;
		}
		last_update_day.store(temp, std::memory_order_relaxed);
		return true;
	}

	void Revert() noexcept { last_update_day.store(0.0f, std::memory_order_relaxed); }

	

}
