#pragma once
#include "BaseTypes.h"
#include "Forms/FearForms.h"
#include "Utils/RNG.h" // Randomization in InitData()

namespace Data {


	struct FearInfo {
	public:
		constexpr FearInfo() noexcept = default;
		constexpr FearInfo(const FearInfo&) = default;
		constexpr FearInfo(FearInfo&&) noexcept = default;
		constexpr FearInfo& operator=(const FearInfo&) = default;
		constexpr FearInfo& operator=(FearInfo&&) noexcept = default;
		constexpr ~FearInfo() noexcept = default;

		FearInfo(RE::Actor* act, const bool allow_writes) noexcept { InitData(act, allow_writes); }


		i8 FearRank() const noexcept { return scalef0100(fear); }
		i8 ThrillseekingRank() const noexcept { return scalef0100(thrillseeking); }
		i8 ThrillseekerRank() const noexcept { return static_cast<i8>((thrillseeking > 0.5f) * 3) - 2; }
		i8 FearsFemaleRank() const noexcept { return scalef0100(fears_female); }
		i8 FearsMaleRank() const noexcept { return scalef0100(fears_male); }


		void InitData(RE::Actor* act, const bool allow_writes) noexcept {
			static RNG::gamerand gen{ RNG::random_state() };
			const float seeds[6]{ gen.nextf01(), gen.nextf01(), gen.nextf01(), gen.nextf01(), gen.nextf01(), gen.nextf01() };

			// Log::Info("FearInfo::InitData initializing {:08X}"sv, act->formID);

			// Fears
			if (GameDataUtils::GetSex(act) == RE::SEX::kFemale) {  // Is female
				is_female = true;
				fears_male = seeds[0] > seeds[1] ? seeds[0] : seeds[1];
				fears_female = seeds[2];
			}
			else {  // Is male
				is_female = false;
				fears_male = seeds[2];
				fears_female = seeds[0] > seeds[1] ? seeds[0] : seeds[1];
			}

			// Last rest
			if (!act->IsPlayerRef()) {
				last_rest_day = GameDataUtils::DaysPassed() - (seeds[3] * -7.0f); // Up to a week before now
			}

			// Fear
			fear = 0.2f + (seeds[4] * 0.9f); // Caps at ~0.9

			// Thrillseeking
			thrillseeking = 0.3f + (seeds[5] * 0.7f);

			if (allow_writes) {
				act->AddToFaction(::Fear::Faction(::Fear::FAC::Fear), FearRank());
				act->AddToFaction(::Fear::Faction(::Fear::FAC::Thrillseeking), ThrillseekingRank());
				act->AddToFaction(::Fear::Faction(::Fear::FAC::Thrillseeker), ThrillseekerRank());
				act->AddToFaction(::Fear::Faction(::Fear::FAC::FearsFemale), FearsFemaleRank());
				act->AddToFaction(::Fear::Faction(::Fear::FAC::FearsMale), FearsMaleRank());
			}

		}


		void Swap(FearInfo& other) noexcept {
			enum : size_t { swapsize = sizeof(FearInfo) };
			static_assert(swapsize == 32);
			if (this != &other) {
				char temp[swapsize];
				std::memcpy(&temp, this, swapsize);
				std::memcpy(this, &other, swapsize);
				std::memcpy(&other, &temp, swapsize);
			}
		}

		bool Save(SKSE::SerializationInterface& intfc) const {
			enum : u32 {
				SerializableSize = static_cast<u32>(sizeof(std::remove_pointer_t<decltype(this)>) - sizeof(decltype(in_brawl)) - sizeof(decltype(pad)))
			};
			static_assert(SerializableSize == 26, "FearInfo size/layout has changed. Revisit Save()/Load() code!");
			return intfc.WriteRecordData(this, SerializableSize);
		}
		bool Load(SKSE::SerializationInterface& intfc) {
			enum : u32 {
				SerializableSize = static_cast<u32>(sizeof(std::remove_pointer_t<decltype(this)>) - sizeof(decltype(in_brawl)) - sizeof(decltype(pad)))
			};
			static_assert(SerializableSize == 26, "FearInfo size/layout has changed. Revisit Save()/Load() code!");
			in_brawl = false; // Set this manually. It was not serialized because brawls end on game load anyway.
			return intfc.ReadRecordData(this, SerializableSize);
		}

		sat01flt fear{};					// 4	How afraid the character is.
		sat01flt thrillseeking{};			// 4	How much the character likes the thrill.
		sat01flt fears_female{};			// 4	How much the character fears females.
		sat01flt fears_male{};				// 4	How much the character fears males.
		sat0flt buildup_mod{};				// 4
		optional_float last_rest_day{};		// 4	Days passed from game start to the character's last full rest. has_value() == false if never rested (newly spawned NPCs).
		bool is_female{};					// 1	True if character is female.
		bool is_blocked{};					// 1
		bool in_brawl{};					// 1
		char pad[5];						// 31

	private:
		static __forceinline constexpr i8 scalef0100(const sat01flt val) noexcept { return static_cast<i8>(val * 100.0f); }
	};
	static_assert(sizeof(FearInfo) == 32);





}
