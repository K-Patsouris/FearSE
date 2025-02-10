#pragma once
#include "Common.h"
#include "Logger.h"
#include <chrono>


namespace Data {
	using std::chrono::steady_clock;
	using std::chrono::nanoseconds;
	using std::chrono::microseconds;
	using std::chrono::milliseconds;
	using std::chrono::seconds;
	using std::chrono::duration;
	using std::chrono::duration_cast;


	namespace Fear {

		void PlayerChangedLocation(const RE::BGSLocation* newloc) noexcept;

	}

	namespace PlayerRules {

		void PlayerAbsorbedDragonSoul() noexcept;
		void PlayerKillDraugr() noexcept;
		void PlayerKillDaedra() noexcept;
		void PlayerKillDragonPriest() noexcept;

		void IdleTimeStart() noexcept;
		void IdleTimeEnd() noexcept;

	}

	namespace Shared {
		static constexpr u32 SerializationType		{ 'FRDT' };
		static constexpr u32 SerializationVersion	{ 1 };

		bool SetFearActive(const bool new_active) noexcept;

		void BrawlEvent(RE::TESForm* controller, const bool starting) noexcept;

		void ProcessEquip(RE::Actor* act, const RE::TESBoundObject* obj) noexcept;
		void ProcessUnequip(RE::Actor* act, const RE::TESBoundObject* obj) noexcept;

		void FastTravelEnd(const float hours) noexcept;

		struct UpdateKinds {
			enum Kind {
				ShortUpdate,
				DefaultUpdate,
				LongUpdate
			};

			constexpr void mark(const Kind t) noexcept {
				switch (t) {
				case ShortUpdate: Short = true; return;
				case DefaultUpdate: Default = true; return;
				case LongUpdate: Long = true; return;
				default: Log::Error("Data::Shared::UpdateKinds::mark() called with unexpected argument {}"sv, std::bit_cast<int>(t));
				}
			}
			constexpr bool is_marked(const Kind t) const noexcept {
				switch (t) {
				case ShortUpdate: return Short;
				case DefaultUpdate: return Default;
				case LongUpdate: return Long;
				default: Log::Error("Data::Shared::UpdateKinds::is_marked() called with unexpected argument {}"sv, std::bit_cast<int>(t)); return false;
				}
			}
			constexpr bool any_marked() const noexcept { return Short or Default or Long; }

			constexpr void unmark_all() noexcept {
				Short = false;
				Default = false;
				Long = false;
			}


		private:
			bool Short{ false };
			bool Default{ false };
			bool Long{ false };
		};
		struct UpdateDeltas {
			constexpr UpdateDeltas& operator+=(const milliseconds ms) noexcept {
				delta_short += ms;
				delta_default += ms;
				delta_long += ms;
				return *this;
			}
			constexpr void reset() noexcept {
				delta_short = 0ms;
				delta_default = 0ms;
				delta_long = 0ms;
			}
			milliseconds delta_short{ 0ms };
			milliseconds delta_default{ 0ms };
			milliseconds delta_long{ 0ms };
		};
		void Update(const UpdateKinds kinds, const UpdateDeltas deltas) noexcept;


		void InstallHooks();


		bool Save(SKSE::SerializationInterface& intfc) noexcept;
		bool Load(SKSE::SerializationInterface& intfc, u32 version) noexcept;
		void Revert() noexcept;

	}


	namespace Functions {

		bool RegisterBasic(IVM* vm);
		bool RegisterFear(IVM* vm);

	}

}

