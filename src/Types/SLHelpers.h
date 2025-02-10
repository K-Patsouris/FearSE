#pragma once
#include "Common.h"
#include "Types/LazyVector.h"

namespace SLHelpers {
	using namespace LazyVector;

	// !IsValid() signifies an invalid state. No guarantees about pointer validity or other function behavior. Assuming IsValid() :
	// 	* ActorCount() is count of Actors in the scene (count of Active Actors, +1 for the Passive). IsValid() simply checks if this is != 0.
	// 	* PassiveActor() will not be nullptr and represents the Passive Actor of the scene
	// 	* the pointers in ActiveActors() subrange (of size ActorCount() - 1) will not be nullptr and represent the Active Actor(s) in the scene
	// 	* if PassiveIsVictim() == true, then the PassiveActor() is a Victim in the scene
	// 	* for i in [0, ActorCount() - 1), if ActiveIsVictim(i) == true, ActiveActors()[i] is a Victim in the scene
	// 	* Tags() contains all the SL Animation tags, except potential "" tags which have been filtered
	struct sslThreadController_interface {
	private:
		struct flags32 {
			__forceinline constexpr void set(const u32 i) noexcept { f |= static_cast<u32>(1 << i); }
			[[nodiscard]] __forceinline constexpr bool operator[](const u32 i) const noexcept { return (f >> i) & 1; }
			__forceinline constexpr void clear() noexcept { f = 0; }
		private:
			u32 f{ 0 };
		};
	
	public:
		sslThreadController_interface(RE::TESForm* controller) noexcept;


		[[nodiscard]] bool IsValid() const noexcept { return actor_count != 0; }

		[[nodiscard]] u32 ActorCount() const noexcept { return actor_count; }

		[[nodiscard]] RE::Actor* PassiveActor() const noexcept { return actors[0].get(); }
		[[nodiscard]] auto ActiveActors() const noexcept { return std::ranges::subrange{ actors.begin() + 1, actors.begin() + actor_count }; }

		[[nodiscard]] bool PassiveIsVictim() const noexcept { return victim_flags[0]; }
		[[nodiscard]] bool ActiveIsVictim(const u32 i) const noexcept { return victim_flags[i + 1]; }

		[[nodiscard]] const lazy_vector<string>& Tags() const noexcept { return tags; }


		enum : size_t { MaxActors = 8 };
	private:
		array<RE::ActorPtr, MaxActors> actors{};
		u32 actor_count{};
		flags32 victim_flags{};
		lazy_vector<string> tags{};
	};
	static_assert(sizeof(sslThreadController_interface) == 96);

}
