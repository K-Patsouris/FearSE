#pragma once
#include "Forms/VanillaForms.h"	// IsNPCRace
#include "Utils/GameDataUtils.h"
#include "Types/LazyVector.h"
#include "Types/NumericTypes.h"
#include "Types/TrivialHandle.h"

namespace Data {
	

	// Expects dereferencable act
	__forceinline bool IsValidKeepable(RE::Actor* act) noexcept {
		bool ret = !act->IsDead();
		const auto flags = act->formFlags;
		ret &= (flags & RE::TESObjectREFR::RecordFlags::kInitiallyDisabled) == 0;
		ret &= (flags & RE::TESForm::RecordFlags::kDeleted) == 0;
		return ret;
		
	}
	// Expects dereferencable act
	__forceinline bool IsValidAddable(RE::Actor* act) noexcept {
		bool ret = act->race and Vanilla::IsNPCRace(act->race) and !act->IsChild();
		return ret and IsValidKeepable(act);
	}

	// Does not dereference
	__forceinline bool IsPlayer(RE::Actor* act) noexcept { return act == Vanilla::Player(); }


	using namespace LazyVector;
	using namespace NumericTypes;
	using namespace TrivialHandle;

	using sat0flt = saturating<float, 0.0f, FLT_MAX>;		// A finite float in [0.0f, FLT_MAX]
	using sat1flt = saturating<float, 1.0f, FLT_MAX>;		// A finite float in [1.0f, FLT_MAX]
	using sat0100flt = saturating<float, 0.0f, 100.0f>;		// A finite float in [0.0f, 100.0f]
	using satpos100flt = saturating<float, 0.1f, 100.0f>;	// A finite float in [0.1f, 100.0f]
	using sat01flt = saturating<float, 0.0f, 1.0f>;			// A finite float in [0.0f, 1.0f]
	using satu8 = saturating<u8>;						// A non-overflowing u8 in [0, UINT8_MAX]
	using sat1u8 = saturating<u8, 1, UINT8_MAX>;			// A non-overflowing u8 in [1, UINT8_MAX]
	using sat07u8 = saturating<u8, 0, 7>;				// A non-overflowing u8 in [0, 7]
	using sat17u8 = saturating<u8, 1, 7>;				// A non-overflowing u8 in [1, 7]
	using sat014u8 = saturating<u8, 0, 14>;				// A non-overflowing u8 in [0, 14]
	using sat114u8 = saturating<u8, 1, 14>;				// A non-overflowing u8 in [1, 14]
	using sat0100u8 = saturating<u8, 0, 100>;			// A non-overflowing u8 in [0, 100]
	using sat1100u8 = saturating<u8, 1, 100>;			// A non-overflowing u8 in [1, 100]
	using sat0200u8 = saturating<u8, 0, 200>;			// A non-overflowing u8 in [0, 200]
	using sat0500u16 = saturating<u16, 0, 500>;			// A non-overflowing u16 in [0, 500]
	using satu32 = saturating<u32>;						// A non-overflowing u32 in [0, UINT_MAX]
	using sat1u32 = saturating<u32, 1, UINT_MAX>;		// A non-overflowing u8 in [1, UINT_MAX]
	using sati32 = saturating<i32>;						// A non-overflowing i32 in [INT_MIN, INT_MAX]





	namespace UpdateTypes {

		enum : size_t { MaxUpdateCount = 32 };

		struct flags32 {
			__forceinline constexpr void set_if_val(const u32 i, const bool val) noexcept { f |= static_cast<u32>(val << i); }
			[[nodiscard]] __forceinline constexpr bool operator[](const u32 i) const noexcept { return (f >> i) & 1; }
			__forceinline constexpr void clear() noexcept { f = 0; }
		private:
			u32 f{ 0 };
		};

		struct main_out { // For fear
			void ParseActor(RE::Actor* act) {
				hppcnt = static_cast<u8>(GameDataUtils::GetHealthRatio(act) * 100.0f);
				combat = act->IsInCombat();
			}

			flags32 seeing{};
			u8 hppcnt{};
			bool combat{};
			char pad[2];
		};
		static_assert(std::is_trivially_copyable_v<main_out> and sizeof(main_out) == 8);

		struct ranks_and_oneofs { // Put these in a struct to ensure the one-of stuff all end up in the remaining half cacheline after ranks' 1.5
			constexpr void reset() noexcept {
				actor_count = 0;
				unregistered_count = 0;
			}

			array<array<i8, 3>, MaxUpdateCount> ranks{};
			u32 actor_count{};			// Count of valid actors updating
			u32 unregistered_count{};	// 
			float player_tension{};		// 
			float player_willpower{};	// 
			float now{};				// Current GameDaysPassed global value
			i32 mag{};					// "Magnitude" to set Recent Dodges Spell description to
			bool follower{};			// True iff player has follower
			bool need_ranks{};			// 
			char pad[6];
		};
		static_assert(std::is_trivially_copyable_v<ranks_and_oneofs> and sizeof(ranks_and_oneofs) == 128);

	}




}
