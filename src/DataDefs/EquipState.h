#pragma once
#include "Types/LazyVector.h"
#include "Forms/FearForms.h"	// Keywords
#include "Forms/HurdlesForms.h"	// Keywords

#include <intrin0.inl.h>
#pragma intrinsic(_BitScanReverse) // ~13 cycles to go over 32bits (bsr and mask-unset, on ~16 set bits). Same for tzcnt. lzcnt is ~10% slower. bsr is guaranteed to be available.
// #pragma intrinsic(_BitScanForward) // Same as bsr.

namespace Data {

	struct EquipState {
	public:
		// Kind of a copy of BipedObjectSlot, except in arithmetic sequence from 0 instead of powers of 2, to use as indexes in ArmorEquipFlags
		enum class Slot : u8 {
			// kNone = 0,
			kHead = 0,
			kHair,
			kBody,
			kHands,
			kForearms,
			kAmulet,
			kRing,
			kFeet,
			kCalves,
			kShield,
			kTail,
			kLongHair,
			kCirclet,
			kEars,
			kModMouth,
			kModNeck,
			kModChestPrimary,
			kModBack,
			kModMisc1,
			kModPelvisPrimary,
			kDecapitateHead,
			kDecapitate,
			kModPelvisSecondary,
			kModLegRight,
			kModLegLeft,
			kModFaceJewelry,
			kModChestSecondary,
			kModShoulder,
			kModArmLeft,
			kModArmRight,
			kModMisc2,
			kFX01
		};
		using FRKwd = ::Fear::KWD;
		using HRKwd = Hurdles::KWD;
		using ArmorType = RE::BIPED_MODEL::ArmorType;
		using WeaponType = RE::WeaponTypes::WEAPON_TYPE;
		using AV = RE::ActorValue;

		// Type and FEAR keywords of the armor in a specific slot
		struct SlotFlags {
		public:
			/*
			First FRKwd enumerators except FRKwd::Total, then RE::BIPED_MODEL::ArmorType enumerators. So RE::BIPED_MODEL::ArmorType enumerators start with FRKwd::Total.
			MidQual = 0,
			LowQual = 1,
			Torn = 2,
			NonArmor = 3,
			NoSoles = 4,
			LightArmor = 5,
			HeavyArmor = 6,
			Clothing = 7,
			*/
			enum : u8 {
				ArmorTypeOffset = static_cast<u8>(FRKwd::Total),
				ArmorTypeIndexOffset = ArmorTypeOffset + 1,

				LightMask = 1ui8 << (static_cast<u8>(ArmorType::kLightArmor) + ArmorTypeOffset),
				HeavyMask = 1ui8 << (static_cast<u8>(ArmorType::kHeavyArmor) + ArmorTypeOffset),
				ClothingMask = 1ui8 << (static_cast<u8>(ArmorType::kClothing) + ArmorTypeOffset),

				TypeMask = 0b1110'0000ui8,
				KwdsMask = 0b0001'1111ui8,
				SoleMask = 0b0001'0000ui8,
				TierKwdMask = static_cast<u8>(KwdsMask & ~SoleMask),
			};

			__forceinline constexpr u8 raw() const noexcept { return f; }

			__forceinline constexpr bool is(const FRKwd flag) const noexcept { return f & static_cast<u8>(1ui8 << static_cast<u8>(flag)); }
			__forceinline constexpr bool is(const ArmorType type) const noexcept { return f & static_cast<u8>(1ui8 << (static_cast<u8>(type) + ArmorTypeOffset)); }

			__forceinline constexpr bool worn() const noexcept { return f & TypeMask; }
			__forceinline constexpr bool not_worn() const noexcept { return (f & TypeMask) == 0; }

			__forceinline constexpr u8 type() const noexcept { return f & TypeMask; }
			__forceinline constexpr u8 type_index() const noexcept { return f >> ArmorTypeIndexOffset; }
			__forceinline constexpr bool type_valid() const noexcept { return is_valid_type(f & TypeMask); }
			__forceinline constexpr u8 tiers() const noexcept { return f & TierKwdMask; }

			__forceinline constexpr bool light() const noexcept { return f & LightMask; }
			__forceinline constexpr bool heavy() const noexcept { return f & HeavyMask; }
			__forceinline constexpr bool clothing() const noexcept { return f & ClothingMask; }
			__forceinline constexpr bool not_light() const noexcept { return (f & LightMask) == 0; }
			__forceinline constexpr bool not_heavy() const noexcept { return (f & HeavyMask) == 0; }
			__forceinline constexpr bool not_clothing() const noexcept { return (f & ClothingMask) == 0; }

			template<ArmorType type>
			__forceinline constexpr bool not_type() const noexcept { return (f & static_cast<u8>(1ui8 << (static_cast<u8>(type) + ArmorTypeOffset))) == 0; }

			__forceinline constexpr bool nosoles() const noexcept { return f & SoleMask; }
			__forceinline constexpr bool soles() const noexcept { return (f & SoleMask) == 0; }

			__forceinline constexpr void set(const FRKwd flag) noexcept { f |= static_cast<u8>(1ui8 << static_cast<u8>(flag)); }
			__forceinline constexpr void set(const ArmorType type) noexcept {
				f &= KwdsMask; // Unset all armor types
				f |= static_cast<u8>(1 << (static_cast<u8>(type) + ArmorTypeOffset)); // Set the new one
			}

			__forceinline constexpr void unset(const FRKwd flag) noexcept { f &= ~static_cast<u8>(1ui8 << static_cast<u8>(flag)); }

			__forceinline constexpr void accumulate(const SlotFlags other) noexcept { f |= other.f; }

			__forceinline constexpr void clear() noexcept { f = 0; }

			__forceinline static constexpr bool is_valid_type(const u8 type) noexcept {
				const u32 x1 = type - 1;	//  This unsets the lowest set bit and sets all the ones lower than it. eg.	1100->1011,		0001->0000,		1000->0111,			0000->1111 (underflow)
				const u32 x2 = type ^ x1;//  This keeps the bit unset before, the bits set before, and no else. eg.	1100^1011=0111,	0001^0000=0001,	1000->0111=1111,	0000^1111=1111
				return x1 < x2;				//  If power of 2, ^ result will always be greater than -1 result because bits are never lost. 0 won't pass because it won't provide bits to ^ to break the ==.
			}

			u8 f{};
		};
		static_assert(std::is_trivially_copyable_v<SlotFlags> and sizeof(SlotFlags) == 1);
		// Count of each Hurdle keyword of equipped armors
		struct HRCounts {
		public:
			__forceinline constexpr bool has(const HRKwd id) const noexcept { return counts[static_cast<size_t>(id)] != 0; }
			__forceinline constexpr bool count(const HRKwd id) const noexcept { return counts[static_cast<size_t>(id)]; }
			__forceinline constexpr void add(const HRKwd id) noexcept { counts[static_cast<size_t>(id)]++; }
			__forceinline constexpr void remove(const HRKwd id) noexcept { counts[static_cast<size_t>(id)]--; }
			__forceinline constexpr bool has_any_10_13() const noexcept { return counts[static_cast<size_t>(HRKwd::Hurdle_10)] bitor counts[static_cast<size_t>(HRKwd::Hurdle_13)]; }
			__forceinline constexpr bool empty() const noexcept { return empty_flag; }

			void update_empty_flag() noexcept {
				//  These all get inlined to 7 instructions (movzx, or, sete). The only cost is getting the array in a cacheline, which duh.
				u64 n1, n2, n3;
				u16 n4;
				std::memcpy(&n1, &counts[0], 8);
				std::memcpy(&n2, &counts[8], 8);
				std::memcpy(&n3, &counts[16], 8);
				std::memcpy(&n4, &counts[24], 2);
				empty_flag = (n1 | n2 | n3 | n4 | counts[26]) == 0;
			}

			constexpr void clear() noexcept { counts = array<satu8, static_cast<size_t>(HRKwd::Total)>{}; }
			
			array<satu8, static_cast<size_t>(HRKwd::Total)> counts{};
			bool empty_flag{ true };
		};
		static_assert(std::is_trivially_copyable_v<HRCounts> and sizeof(HRCounts) == 28);
		// Weapon type/Spell school of left and right hand
		struct HandsFlags {
		public:
			/*
				First RE::WeaponTypes::WEAPON_TYPE which is 0 - 9
				Then RE::ActorValue which is -1 - 164
			*/
			enum : u8 {
				AVOffset = static_cast<u8>(WeaponType::kTotal) + 1 // Just add 1 to bring ActorValue::kNone to 10. Unsigned overflow is defined.
			};

			__forceinline constexpr bool left_is(const WeaponType type) const noexcept { return left == static_cast<u8>(type); }
			__forceinline constexpr bool left_is(const AV av) const noexcept { return left == (static_cast<u8>(av) + AVOffset); }

			__forceinline constexpr bool left_is_weapon() const noexcept { return (left != 0) & (left < AVOffset); }
			__forceinline constexpr bool left_is_spell() const noexcept { return left >= AVOffset; }

			__forceinline constexpr void left_set(const WeaponType type) noexcept { left = static_cast<u8>(type); }
			__forceinline constexpr void left_set(const AV av) noexcept { left = static_cast<u8>(av) + AVOffset; }

			__forceinline constexpr void left_unset() noexcept { left = 0; }


			__forceinline constexpr bool right_is(const WeaponType type) const noexcept { return right == static_cast<u8>(type); }
			__forceinline constexpr bool right_is(const AV av) const noexcept { return right == (static_cast<u8>(av) + AVOffset); }

			__forceinline constexpr bool right_is_weapon() const noexcept { return (right != 0) & (right < AVOffset); }
			__forceinline constexpr bool right_is_spell() const noexcept { return right >= AVOffset; }

			__forceinline constexpr void right_set(const WeaponType type) noexcept { right = static_cast<u8>(type); }
			__forceinline constexpr void right_set(const AV av) noexcept { right = static_cast<u8>(av) + AVOffset; }

			__forceinline constexpr void right_unset() noexcept { right = 0; }

			__forceinline constexpr bool any_weapon() const noexcept { return left_is_weapon() | right_is_weapon(); }
			__forceinline constexpr bool any_spell() const noexcept { return (left | right) >= AVOffset; }


			__forceinline constexpr void clear() noexcept { left = 0;  right = 0; }

			u8 left{};
			u8 right{};
		};
		static_assert(std::is_trivially_copyable_v<HandsFlags> and sizeof(HandsFlags) == 2);

		// Processed version of base slot (head, chest, hands, feet) SlotFlags
		struct AnalyzedBasicSlots {
			struct WornFlags {
				constexpr void AccumulateType(const SlotFlags slot) noexcept {
					//  type_index:	light:0, heavy:1,  clothing:2,	fucked:whatever
					//  shiftindex:	light:8, heavy:16, clothing:24,	fucked:whatever
					const u32 shiftindex = CHAR_BIT * (1 + slot.type_index());
					f |= (static_cast<u32>(slot.raw() * slot.type_valid()) << shiftindex); //  * type_valid() makes it do nothing if the piece has no valid type (not exactly 1 type)
				}
				constexpr void AccumulateTypeFeet(const SlotFlags slot) noexcept {
					//  See above for notes
					const bool type_valid = slot.type_valid();
					const u32 shiftindex = CHAR_BIT * (1 + slot.type_index());
					f |= (static_cast<u32>(slot.raw() * type_valid) << shiftindex);
					f |= static_cast<u32>(type_valid);
					f |= static_cast<u32>((slot.nosoles() & type_valid) << 1); //  type_valid acts as worn flag. If true, we may turn the soleless flag on as needed.
				}

				//  bits 31-24: clothing SlotFlags,  bits 23-16: heavy SlotFlags,  bits 15-8: light SlotFlags,  bit 1: nosoles worn,  bit 0: feet worn
				//  0000 0000	0000 0000	0000 0000	0000 00hf
				//  Clothing		Heavy		Light		NoSoles and Feet flags
				u32 f{ 0 };
			};

			constexpr AnalyzedBasicSlots() noexcept = default;
			constexpr AnalyzedBasicSlots(const AnalyzedBasicSlots&) noexcept = default;
			constexpr AnalyzedBasicSlots(AnalyzedBasicSlots&&) noexcept = default;
			constexpr AnalyzedBasicSlots& operator=(const AnalyzedBasicSlots&) noexcept = default;
			constexpr AnalyzedBasicSlots& operator=(AnalyzedBasicSlots&&) noexcept = default;
			constexpr ~AnalyzedBasicSlots() noexcept = default;

			constexpr AnalyzedBasicSlots(const EquipState& state) noexcept : flags{ Parse(state) } {}

			constexpr void Analyze(const EquipState& state) noexcept { flags = Parse(state); }

			enum : u32 {
				LowByteMask = 0xff,				//  & to keep only the low byte
				HighBytesMask = ~LowByteMask,	//  & to keep only the 3 high bytes

				LightShift = sizeof(SlotFlags) * CHAR_BIT,
				HeavyShift = 2 * LightShift,
				ClothingShift = 3 * LightShift,

				FeetMask = 0b01,
				NoSolesMask = 0b10,
			};
			constexpr SlotFlags light() const noexcept { return { .f = static_cast<u8>((flags.f >> LightShift) & LowByteMask) }; }
			constexpr SlotFlags heavy() const noexcept { return { .f = static_cast<u8>((flags.f >> HeavyShift) & LowByteMask) }; }
			constexpr SlotFlags clothing() const noexcept { return { .f = static_cast<u8>(flags.f >> ClothingShift) }; }
			constexpr bool feet_worn() const noexcept { return flags.f & FeetMask; }
			constexpr bool no_soles_worn() const noexcept { return flags.f & NoSolesMask; }
			constexpr bool feet_only() const noexcept { return (flags.f & LowByteMask) == FeetMask; }
			constexpr bool soleless_or_barefoot() const noexcept { return (flags.f & LowByteMask) != FeetMask; }

		private:
			static constexpr WornFlags Parse(const EquipState& state) noexcept {
				// Accumulate all FEAR kwd flags for each type. We care only about the presence of a kwd on any one basic piece of armor type, so no need to keep count or check all slots.
				WornFlags flags{};
				flags.AccumulateType(state.slots[static_cast<u64>(Slot::kCirclet)]);
				flags.AccumulateType(state.slots[static_cast<u64>(Slot::kBody)]);
				flags.AccumulateType(state.slots[static_cast<u64>(Slot::kHands)]);
				flags.AccumulateTypeFeet(state.slots[static_cast<u64>(Slot::kFeet)]);
				return flags;
			}
			WornFlags flags{};
		};
		static_assert(std::is_trivially_copyable_v<AnalyzedBasicSlots> and sizeof(AnalyzedBasicSlots) == 4);
		// Flags the new armor set by being equipped
		struct ArmorEquippedFlags {
			struct HRFlags {
			public:
				__forceinline constexpr bool has(const HRKwd kwd) const noexcept { return f & (1ui32 << static_cast<u8>(kwd)); }
				__forceinline constexpr void set(const HRKwd kwd) noexcept { f |= (1ui32 << static_cast<u8>(kwd)); }
				__forceinline constexpr void unset(const HRKwd kwd) noexcept { f &= ~(1ui32 << static_cast<u8>(kwd)); }

				__forceinline constexpr bool any() const noexcept { return f != 0; }

				template<HRKwd kwd> requires (kwd < HRKwd::Total)
				__forceinline constexpr bool has() const noexcept {
					enum : u32 { hrmask = 1ui32 << static_cast<u8>(kwd) };
					return f & hrmask;
				}

				__forceinline constexpr void clear() noexcept { f = 0; }

				u32 f{};
			};
			static_assert(std::is_trivially_copyable_v<HRFlags> and sizeof(HRFlags) == 4);

			struct CoverFlags {
			public:
				static constexpr u32 BodyMask = (1ui32 << static_cast<u8>(Slot::kBody));
				static constexpr u32 HeadMask = (1ui32 << static_cast<u8>(Slot::kCirclet));
				static constexpr u32 HandsMask = (1ui32 << static_cast<u8>(Slot::kHands));
				static constexpr u32 FeetMask = (1ui32 << static_cast<u8>(Slot::kFeet));
				static constexpr u32 AnyBasicMask = (BodyMask | HeadMask | HandsMask | FeetMask);
				static constexpr u32 NotBasicMask = ~AnyBasicMask;

				__forceinline constexpr void set(const u32 slots) noexcept { f = slots; }
				__forceinline constexpr bool has(const Slot s) const noexcept { return f & (1ui32 << static_cast<u32>(s)); }
				__forceinline constexpr void set(const Slot s) noexcept { f |= (1ui32 << static_cast<u32>(s)); }
				__forceinline constexpr void unset(const Slot s) noexcept { f &= ~(1ui32 << static_cast<u32>(s)); }

				__forceinline constexpr bool any_basic() const noexcept { return f & AnyBasicMask; }
				__forceinline constexpr bool not_basic() const noexcept { return (f & AnyBasicMask) == 0; }
				__forceinline constexpr bool any() const noexcept { return f > 0; }

				__forceinline constexpr bool feet() const noexcept { return f & FeetMask; }
				__forceinline constexpr bool not_feet() const noexcept { return (f & FeetMask) == 0; }

				__forceinline constexpr void clear() noexcept { f = 0; }

				u32 f{};
			};
			static_assert(std::is_trivially_copyable_v<CoverFlags> and sizeof(CoverFlags) == 4);

			__forceinline constexpr bool empty() const noexcept { return (hr.f == 0) bitand (slots.f == 0); }

			HRFlags hr{};
			CoverFlags slots{};
			SlotFlags flags{};
			char pad[3];
		};
		static_assert(std::is_trivially_copyable_v<ArmorEquippedFlags> and sizeof(ArmorEquippedFlags) == 12);
		// Flags the new hand set by being equipped
		struct HandEquippedFlags {
			/*
				First RE::WeaponTypes::WEAPON_TYPE which is 0 - 9
				Then RE::ActorValue which is -1 - 164
			*/
			static constexpr u8 AVOffset = static_cast<u8>(WeaponType::kTotal) + 1; // Just add 1 to bring ActorValue::kNone to 10. Unsigned overflow is defined.

			constexpr HandEquippedFlags() noexcept = default;
			constexpr HandEquippedFlags(HandEquippedFlags&&) noexcept = default;
			constexpr HandEquippedFlags(const HandEquippedFlags&) noexcept = default;
			constexpr HandEquippedFlags& operator=(HandEquippedFlags&&) noexcept = default;
			constexpr HandEquippedFlags& operator=(const HandEquippedFlags&) noexcept = default;
			constexpr ~HandEquippedFlags() noexcept = default;

			constexpr HandEquippedFlags(const WeaponType type) noexcept : f(static_cast<u8>(type)) {}
			constexpr HandEquippedFlags(const AV av) noexcept : f(static_cast<u8>(av) + AVOffset) {}

			__forceinline constexpr bool is(const WeaponType type) const noexcept { return f == static_cast<u8>(type); }
			__forceinline constexpr bool is(const AV av) const noexcept { return f == (static_cast<u8>(av) + AVOffset); }

			__forceinline constexpr bool is_weapon() const noexcept { return (f != 0) & (f < AVOffset); }
			__forceinline constexpr bool is_spell() const noexcept { return f >= AVOffset; }

			__forceinline constexpr bool empty() const noexcept { return f == 0; }

			__forceinline constexpr void set(const WeaponType type) noexcept { f = static_cast<u8>(type); }
			__forceinline constexpr void set(const AV av) noexcept { f = static_cast<u8>(av) + AVOffset; }

			__forceinline constexpr void clear() noexcept { f = 0; }

			u8 f{};
		};
		static_assert(std::is_trivially_copyable_v<HandEquippedFlags> and sizeof(HandEquippedFlags) == 1);


		explicit constexpr EquipState() noexcept = default;
		constexpr EquipState(EquipState&&) noexcept = default;
		constexpr EquipState(const EquipState&) noexcept = default;
		constexpr EquipState& operator=(EquipState&&) noexcept = default;
		constexpr EquipState& operator=(const EquipState&) noexcept = default;
		constexpr ~EquipState() noexcept = default;

		EquipState(RE::Actor* act) { InitData(act); }

		void InitData(RE::Actor* act) noexcept {
			// RE::TESObjectREFR::GetInventory() searches 2 things: the "inventory changes" and the "container"
			// 	Inventory changes are InventoryChanges* objects which TESObjectREFR's "extraList" (ExtraDataList) member may have one of
			// 	Containers are TESContainer* objects which TESObjectREFR's "data" (OBJ_REFR) member's objectReference (TESBoundObject*) member is attempted to be cast to
			// The result is a list of InventoryEntryData*, which has an "object" (TESBoundObject*), an int held count, and an "extraLists" (BSSimpleList<ExtraDataList*>*) as relevant members
			// 	The BoundObjet* is used as the map key and the numeric held count is paired with the InventoryEntryData* to make the map value, but all the info comes from the InventoryEntryData*
			// 	InventoryChanges has an "entryList" (BSSimpleList<InventoryEntryData*>*) member, which is what is added to the returned list
			// 	TESContainer can only provide a TESBoundObject* and a numeric held count, leaving the resulting InventoryEntryData's ExtraDataList* list as nullptr
			// The InventoryEntryData::IsWorn() check is done using the "extraLists" member. It always returns false if "extraLists" is nullptr
			// Consequently, no InventoryEntryData element of the resulting list will never will ever be worn if it was created via a TESContainer, because its "extraLists" was left as nullptr
			// This means we can ignore the TESContainer, and filter only the InventoryChanges to find our worn items.
			// I don't know  Actor objects even have/can be cast to TESContainer, and if that TESContainer even contains anything if yes, but it is irrelevant in this case so I won't bother checking

			// Process armors
			if (const auto changes = act->GetInventoryChanges(false); changes and changes->entryList) { // Force noinit to false in case the default ever changes
				for (const auto entry : *changes->entryList) {
					if (const auto obj = entry->object; obj and obj->Is(RE::FormType::Armor) and entry->IsWorn()) { // CHECK HOW OFTEN IT IS NOT WORN
						(void)ProcessArmorEquip(static_cast<RE::TESObjectARMO*>(obj)); // Unnecessary ArmorEquippedFlags creation. Compiler smart enough to cull it?
					}
				}
			}
			// Process hands
			UpdateHands(act); // Unnecessary unsetting if nothing/something irrelevant equipped. Compiler smart enough to know they were 0 already?
		}

		float GetExposure() const noexcept {
			const u8 slot32 = slots[static_cast<u64>(RE::BIPED_MODEL::BipedObjectSlot::kBody)].f;

			static constexpr u8 TYPEMASK = 0b1110'0000ui8;
			static constexpr u8 KYWDMASK = 0b0000'1111ui8; // Ignore nosoles, if somehow set.

			unsigned long kwdindex; // NonArmor:3   Torn:2   LowQual:1   MidQual:0   None:unspecified
			const bool anykwd = _BitScanReverse(&kwdindex, (slot32 & KYWDMASK)); // false if all 0

			/*	if not worn: ((0 * ...) + (1 * 4)) * 0.25f = 1.0f
				if worn and
					NonArmor: ((1 * 1 * (3 + 1)) + (0 * ...)) * 0.25f = 1.0f
					Torn: ((1 * 1 * (2 + 1)) + (0 * ...)) * 0.25f = 0.75f
					LowQual:   ((1 * 1 * (1 + 1)) + (0 * ...)) * 0.25f = 0.5f
					MidQual: ((1 * 1 * (0 + 1)) + (0 * ...)) * 0.25f = 0.25f
					None:   ((1 * 0 * ...) + (0 * ...)) * ... = 0.0f
				When anykwd is false, kwdindex being unspecified doesn't matter. Any integer * 0 == 0.*/

			const bool worn = (slot32 & TYPEMASK);								// 2  cycles: and*1, cmp*1
			return (((worn * anykwd * (kwdindex + 1)) + (!worn * 4)) * 0.25f);	// 23 cycles: mul*4, cvt*1, add*2, not*1

			/* // Verification
			constexpr bool Worn = true;
			constexpr bool Anykwd = true;
			constexpr unsigned long Kwdindex = 0; // NonArmor:3   Torn:2   LowQual:1   MidQual:0   None:unspecified
			constexpr float result = (((Worn * Anykwd * (Kwdindex + 1)) + (!Worn * 4)) * 0.25f); */
		}
		constexpr bool IsNaked() const noexcept {
			SlotFlags slot32 = slots[static_cast<u64>(RE::BIPED_MODEL::BipedObjectSlot::kBody)];
			return !slot32.worn() or slot32.is(FRKwd::NonArmor);
		}


		ArmorEquippedFlags ProcessArmorEquip(const RE::TESObjectARMO* armor) noexcept {
			// Copy data, first thing
			const lazy_vector<RE::BGSKeyword*> copiedKwds{ armor->keywords, armor->numKeywords };
			const auto biped = armor->bipedModelData;

			ArmorEquippedFlags changes{};

			if (u32 occupiedslots = biped.bipedObjectSlots.underlying(); occupiedslots > 0) {
				SlotFlags newflags{};

				newflags.set(biped.armorType.get()); // Set armor type

				for (const auto match : ::Fear::KwdsToIdx(copiedKwds)) {
					newflags.set(match); // Set Fear keyword flags
				}

				changes.slots.set(occupiedslots);
				changes.flags = newflags;

				unsigned long index;
				while (_BitScanReverse(&index, occupiedslots)) {
					occupiedslots ^= (1 << index); // Unset it. Can use ^ instead of &= ~... because we know occupiedslots has that bit set so ^ will unset it.
					slots[index] = newflags; // Update flags on that slot
				}
			}

			for (const auto match : Hurdles::KwdsToIdx(copiedKwds)) {
				hr.add(match); // Set Hurdles keyword flags
				changes.hr.set(match);
			}

			return changes;
		}
		void ProcessArmorUnequip(const RE::TESObjectARMO* armor) noexcept {
			// Copy data, first thing
			const lazy_vector<RE::BGSKeyword*> copiedKwds{ armor->keywords, armor->numKeywords };
			const auto biped = armor->bipedModelData;

			if (u32 occupiedslots = biped.bipedObjectSlots.underlying(); occupiedslots > 0) {
				unsigned long index;
				while (_BitScanReverse(&index, occupiedslots)) {
					occupiedslots ^= (1 << index); // Unset it. Can use ^ instead of &= ~... because we know occupiedslots has that bit set so ^ will unset it.
					slots[index].clear(); // Clear flags on that slot
				}
			}

			for (const auto match : Hurdles::KwdsToIdx(copiedKwds)) {
				hr.remove(match); // Set Hurdles keyword flags
			}
		}
		void UpdateHands(RE::Actor* act) noexcept {
			if (const RE::AIProcess* proc = act->currentProcess; proc) {
				const RE::TESForm* const* equips = proc->equippedObjects;

				if (const RE::TESForm* left = equips[RE::AIProcess::Hands::Hand::kLeft]; left) {
					switch (left->GetFormType()) {
					case RE::TESObjectWEAP::FORMTYPE: {
						hands.left_set(static_cast<const RE::TESObjectWEAP*>(left)->GetWeaponType());
						break;
					}
					case RE::SpellItem::FORMTYPE: {
						hands.left_set(static_cast<const RE::SpellItem*>(left)->GetAssociatedSkill());;
						break;
					}
					default: { hands.left_unset(); }
					}
				}
				else {
					hands.left_unset();
				}

				if (const RE::TESForm* right = equips[RE::AIProcess::Hands::Hand::kRight]; right) {
					switch (right->GetFormType()) {
					case RE::TESObjectWEAP::FORMTYPE: {
						hands.right_set(static_cast<const RE::TESObjectWEAP*>(right)->GetWeaponType());
						break;
					}
					case RE::SpellItem::FORMTYPE: {
						hands.right_set(static_cast<const RE::SpellItem*>(right)->GetAssociatedSkill());;
						break;
					}
					default: { hands.right_unset(); }
					}
				}
				else {
					hands.right_unset();
				}
			}
		}


		void Swap(EquipState& other) noexcept {
			enum : size_t { swapsize = sizeof(EquipState) };
			static_assert(std::is_trivially_copyable_v<EquipState> and swapsize == 64);
			if (this != &other) {
				// When the thing is trivially copyable, and no members need to be ignored (like padding), this generates the same code as std::swap();
				// When padding needs to be ignored, this is obviously faster.
				// When the thing has non-trivially copyable members which are fine to swap by raw byteswapping (like std::vector/lazy_vector) this is also faster on MSVC.
				//   MSVC generates a (not inlined) call to std::swap() which uses probably move constructors accordingly. GCC sees through that and always enerates raw byteswapping code.
				char temp[swapsize];
				std::memcpy(&temp, this, swapsize);
				std::memcpy(this, &other, swapsize);
				std::memcpy(&other, &temp, swapsize);
			}
		}

		bool Save(SKSE::SerializationInterface& intfc) const {
			enum : u32 {
				SerializableSize = static_cast<u32>(sizeof(std::remove_pointer_t<decltype(this)>) - sizeof(decltype(pad)))
			};
			static_assert(SerializableSize == 62, "EquipState size/layout has changed. Revisit Save()/Load() code!");
			return intfc.WriteRecordData(this, SerializableSize);
		}
		bool Load(SKSE::SerializationInterface& intfc) {
			enum : u32 {
				SerializableSize = static_cast<u32>(sizeof(std::remove_pointer_t<decltype(this)>) -sizeof(decltype(pad)))
			};
			static_assert(SerializableSize == 62, "EquipState size/layout has changed. Revisit Save()/Load() code!");
			return intfc.ReadRecordData(this, SerializableSize);
		}

		array<SlotFlags, 32> slots{};
		HRCounts hr{};
		HandsFlags hands{};
		char pad[2]; // To align in cacheline
	};
	static_assert(std::is_trivially_copyable_v<EquipState> and sizeof(EquipState) == 64);

	using HandsFlags = EquipState::HandsFlags;
	using AnalyzedBasicSlots = EquipState::AnalyzedBasicSlots;
	using ArmorEquippedFlags = EquipState::ArmorEquippedFlags;
	using HandEquippedFlags = EquipState::HandEquippedFlags;




}
