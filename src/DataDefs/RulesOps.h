#pragma once
#include "PlayerRules.h"
#include "Forms/RulesForms.h"

namespace Data::PlayerRules {

	using namespace GameDataUtils;
	using PrimitiveUtils::to_s32;
	using StringUtils::ObjectivePronouns;

	std::atomic<float> last_update_day{ 0.0f };

	std::atomic<float> idling_start_day{ 0.0f }; // Not serialized. Can't save DURING sleep/wait/fast travel.
	std::atomic<float> days_to_skip{ 0.0f }; // But this is serialized. Save can happen after idling but before timed update.


	void BrawlerHitDefender(rules_info& passive, const lazy_vector<string>& tags, const float buildup) noexcept {
		for (const auto& tag : tags) {
			if (StrICmp(tag.c_str(), "raw") or StrICmp(tag.c_str(), "angry") or StrICmp(tag.c_str(), "merry")) {
				const float mod = std::sqrt(buildup); // Fast rise towards 1.0f, slower rise afterwards. Maybe some simple clamping instead would be better?
				const float rnd = RNG::Random<float>(0.6f, 1.4f); // Some variance
				const u32 hitcount = static_cast<u32>(5.0f * rnd * mod);
				passive.AdvanceGranter(GranterID::GetHit, hitcount); // passive got hit by act
				return;
			}
		}
		
	}


	void IdleTimeStart() noexcept { idling_start_day.store(DaysPassed(), std::memory_order_relaxed); }
	void IdleTimeEnd() noexcept {
		if (const float isd = idling_start_day.exchange(0.0f, std::memory_order_relaxed); isd > 0.0f) {
			days_to_skip.fetch_add(DaysPassed() - isd, std::memory_order_relaxed);
		}
	}
	void FastTravelEnd(const float hours) noexcept { days_to_skip.fetch_add(hours / 24.0f, std::memory_order_relaxed); }
	
	float GetDayDelta(const float now) noexcept {
		return now - last_update_day.exchange(now, std::memory_order_relaxed) - days_to_skip.exchange(0.0f, std::memory_order_relaxed);
	}

	i32 UpdateShort(lazy_vector<rules_info>& infos, const milliseconds delta, const size_t player_index) noexcept {
		if (delta > 0ms) {
			const float sec_delta = static_cast<float>(delta.count()) * 0.001f;
			for (auto& info : infos) {
				info.TickRecents(sec_delta);
			}
			if (player_index < infos.size()) {
				return to_s32(infos[player_index].RecentsCount());
			}
		}
		return -1;
	}
	

	bool Save(SKSE::SerializationInterface& intfc) noexcept {
		if (!intfc.WriteRecordData(last_update_day.load(std::memory_order_relaxed))) {
			Log::Critical("Failed to serialize last_update_day for PlayerRules!"sv);
			return false;
		}
		if (!intfc.WriteRecordData(days_to_skip.load(std::memory_order_relaxed))) {
			Log::Critical("Failed to serialize days_to_skip for PlayerRules!"sv);
			return false;
		}
		return true;
	}

	bool Load(SKSE::SerializationInterface& intfc) noexcept {
		float temp;
		if (!intfc.ReadRecordData(temp)) {
			Log::Critical("Failed to deserialize last_update_day for PlayerRules!"sv);
			return false;
		}
		last_update_day.store(temp, std::memory_order_relaxed);
		if (!intfc.ReadRecordData(temp)) {
			Log::Critical("Failed to deserialize days_to_skip for PlayerRules!"sv);
			return false;
		}
		days_to_skip.store(temp, std::memory_order_relaxed);
		return true;
	}

	void Revert() noexcept {
		last_update_day.store(0.0f, std::memory_order_relaxed);
		days_to_skip.store(0.0f, std::memory_order_relaxed);
	}

}




