#pragma once
#include "BaseTypes.h"
#include "EquipState.h"
#include "Forms/VanillaForms.h"

#include "Utils/MenuUtils.h"
#include "Utils/RNG.h"			// Randomize rule selections
#include "Utils/StringUtils.h"

#include <immintrin.h>
#pragma intrinsic(_pdep_u32)
#pragma intrinsic(_tzcnt_u32)
#pragma intrinsic(_BitScanForward)

namespace Data {

	using MenuUtils::slider_params;
	using PrimitiveUtils::bool_extend;
	using PrimitiveUtils::mask_float;
	using StringUtils::StrICmp;

	/*constexpr string_view C_Default	{ "<font color = '#FFFFFFFF'>"sv };
	constexpr string_view C_Purple	{ "<font color = '#FFFF419D'>"sv };
	constexpr string_view C_Green	{ "<font color = '#FF21A43F'>"sv };
	constexpr string_view C_Red		{ "<font color = '#FFFF0000'>"sv }; //*/
	
	// The following are Rule classes for the player. They are intentionally made as memory-compact and branchless as possible, as an excercise. They work, but are kind of unmaintainable.

	namespace Punishers {
		using Slot = EquipState::Slot;
		using SlotFlags = EquipState::SlotFlags;
		using FRKwd = EquipState::FRKwd;
		using HRKwd = EquipState::HRKwd;
		using ArmorType = EquipState::ArmorType;

		struct packed_po {
			enum : u8 {
				Periodic = 0b0000'0001,
				Obeying = 0b0000'0010,
				NotPeriodic = static_cast<u8>(~Periodic),
				NotObeying = static_cast<u8>(~Obeying),
			};

			__forceinline constexpr u8 raw() const noexcept { return f; }

			__forceinline constexpr bool periodic_active() const noexcept { return f & Periodic; }		// Bit 0
			__forceinline constexpr bool periodic_inactive() const noexcept { return f & NotPeriodic; }	// Bit 0
			__forceinline constexpr bool obeying() const noexcept { return f & Obeying; }				// Bit 1	slowest access
			__forceinline constexpr bool periodic_inactive_or_obeyed() const noexcept {
				return (f & Obeying) | (not static_cast<bool>(f & Periodic)); // Return true if obeying or periodic not active
			}

			__forceinline constexpr void set_periodic() noexcept { f |= Periodic; }						// Set bit 0
			__forceinline constexpr void set_obeying() noexcept { f |= Obeying; }						// Set bit 1
			__forceinline constexpr void set_obeying(const bool val) noexcept {					// Set bit 1 to val
				f &= NotObeying;
				f |= static_cast<u8>(val << 1);
			}

			__forceinline constexpr void unset_periodic() noexcept { f &= NotPeriodic; }				// Unset bit 0
			__forceinline constexpr void unset_obeying() noexcept { f &= NotObeying; }					// Unset bit 1

			__forceinline constexpr void andeq_obeying(const bool val) noexcept {
				f &= static_cast<u8>(NotObeying + val + val); // Same code as * or <<
				// false:	f &= 1111'1101, clearing bit 1
				// true:		f &= 1111'1111, leaving f unchanged
			}

			u8 f{};
			// 0000 00op
			// p: periodic rule active
			// o: obeying periodic rule
			// 0: unused
		};
		struct packed_pof : public packed_po {
			enum : u8 {
				Flat = 0b0000'0100,
				NotFlat = static_cast<u8>(~Flat),
				AnyActive = static_cast<u8>(Flat | Periodic),
			};
			__forceinline constexpr bool flat_active() const noexcept { return f & Flat; }		// Bit 2
			__forceinline constexpr bool flat_inactive() const noexcept { return f & NotFlat; }	// Not bit 2
			__forceinline constexpr void set_flat() noexcept { f |= Flat; }						// Set bit 2
			__forceinline constexpr void unset_flat() noexcept { f &= NotFlat; }				// Unset bit 2

			__forceinline constexpr u8 actives() const noexcept { return f & AnyActive; }
			__forceinline constexpr bool any_inactive() const noexcept { return (f & AnyActive) != AnyActive; }

			// u8 f{};
			// 0000 0fop
			// p: periodic rule active		inherited
			// o: obeying periodic rule		inherited
			// f: flat rule active			utilized bit 3 as flat rule active flag
			// 0: unused
		};
		struct packed_poftier : public packed_pof {
			enum : u8 {
				Tier = 0b1111'1000,
				NotTier = static_cast<u8>(~Tier),
				MaxTier = 0b1000'0000,
				FullyActiveValue = static_cast<u8>(static_cast<u8>(Periodic) | static_cast<u8>(Flat) | static_cast<u8>(MaxTier))
			};

			__forceinline constexpr u8 tier() const noexcept { return f >> 3; }
			__forceinline constexpr void set_tier(const u8 val) noexcept {
				f &= NotTier;						// Unset
				f |= static_cast<u8>(val << 3);	// Set new
			}
			__forceinline constexpr bool can_upgrade() const noexcept {
				const u8 cur = f & Tier;
				return (cur != 0) bitand (cur < MaxTier);
			}
			// Returns true if the tier after upgrading isn't maxed and can be upgraded again. Returns false otherwise.
			__forceinline constexpr bool upgrade() noexcept {
				const u8 newtier = static_cast<u8>(f & Tier) << 1; // Calc new
				f &= NotTier;	// Unset
				f |= newtier;	// Set new
				return newtier < MaxTier;
			}

			// tttt tfop
			// p: periodic rule active		inherited
			// o: obeying periodic rule		inherited
			// f: flat rule active			inherited
			// t: rule tier					utilized high 5 bits as rule tier
		};
		static_assert(sizeof(packed_poftier) == sizeof(packed_po) and sizeof(packed_pof) == sizeof(packed_po) and sizeof(packed_po) == 1);
		template<typename T> requires(std::is_enum_v<T> and T::Total <= 16)
		struct packed_aopairs {
			enum : u32 {
				ActiveMask = static_cast<u32>(1 << T::Total) - 1,
				ObeyingMask = static_cast<u32>(ActiveMask << 16),
			};
			__forceinline constexpr u32 raw() const noexcept { return f; }
			__forceinline constexpr u16 actives() const noexcept { return f & ActiveMask; }
			__forceinline constexpr u16 inactives() const noexcept { return static_cast<u32>(~f) & ActiveMask; }
			__forceinline constexpr u16 obeyeds() const noexcept { return f >> 16; }

			__forceinline constexpr u8 active_count() const noexcept { return static_cast<u8>(std::popcount(actives())); }
			__forceinline constexpr u8 inactive_count() const noexcept { return static_cast<u8>(std::popcount(inactives())); }

			__forceinline constexpr bool allowed() const noexcept { return actives() == obeyeds(); }
			__forceinline constexpr bool any_inactive() const noexcept { return actives() != ActiveMask; }

			__forceinline constexpr bool id_active(const T id) const noexcept { return f & static_cast<u32>(1 << id); }
			__forceinline constexpr bool id_obeyed(const T id) const noexcept { return f & static_cast<u32>(1 << (id + 16)); }

			constexpr bool activate_random_check_full(RNG::gamerand& gen) noexcept {
				const u32 availables = inactive_count();
				const u32 pick = gen.next(availables + (availables == 0));
				// _pdep_u32(n, val) returns val with all bits 0 except the (n % 32)th set bit. Returns 0 if popcount(val) <= n. Passing val as mask to pdep is what makes this work. It's not an error.
				// eg. val == 5 (0b0101), n == 0  =>  result == 1 (0b0001)
				// eg. val == 9 (0b1001), n == 1  =>  result == 8 (0b1000)
				// eg. val == 9 (0b1001), n == 2  =>  result == 0 (0b0000)
				f |= _pdep_u32(1ul << pick, inactives()); // 1ul invokes the 32bit version of shl, which masks n to 5 bits.
				return availables > 1;
			}
			__forceinline constexpr void set_id(const T id) noexcept { f |= static_cast<u32>(1 << id); }
			__forceinline constexpr void set_obeyed(const T id) noexcept { f |= static_cast<u32>(id_active(id) << (id + 16)); }
			__forceinline constexpr void set_obeyed(const T id, const bool val) noexcept {
				const u32 sanitized = (val & id_active(id)) << (id + 16);
				f |= sanitized;
			}

			__forceinline constexpr void unset_id(const T id) noexcept { f &= ~static_cast<u32>(1 << id); unset_obeyed(id); }
			__forceinline constexpr void unset_obeyed(const T id) noexcept { f &= ~static_cast<u32>(1 << (id + 16)); }

			u32 f{};
		};
		

		struct CostersEquip {
			enum ID : u8 {
				NoWeapons,	// Equip
				NoSpells,	// Equip
				NoSolesOnly,	// Equip

				Total
			};

			constexpr bool any_inactive() const noexcept {
				static_assert(Total == 3, "CostersEquip::any_inactive() needs updating because number of rules has changed");
				u8 accum = data[NoWeapons].raw();
				accum &= data[NoWeapons].raw();
				accum &= data[NoSolesOnly].raw();
				return (~accum) & packed_pof::AnyActive; // If any has 0 in an active bit then ~accum has 1 in that active bit and this returns true.
			}

			constexpr bool activate_random_check_full(RNG::gamerand& gen, const AnalyzedBasicSlots flagpack, const HandsFlags hands) noexcept {
				static_assert(Total == 3, "CostersEquip::activate_random_check_full() needs updating because number of rules has changed");
				enum Action : u8 {
					Invalid = 0,

					NoWpnP,
					NoWpnF,
					NoSplP,
					NoSplF,
					NoSolesP,
					NoSolesF,

					Total = NoSolesF
				};
				// Fill array with possible actions
				array<Action, Action::Total> acts{};
				u32 idx = 0;
				acts[idx] = static_cast<Action>(NoWpnP & bool_extend(data[NoWeapons].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(NoWpnF & bool_extend(data[NoWeapons].flat_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(NoSplP & bool_extend(data[NoSpells].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(NoSplF & bool_extend(data[NoSpells].flat_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(NoSolesP & bool_extend(data[NoSolesOnly].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(NoSolesF & bool_extend(data[NoSolesOnly].flat_inactive()));
				const u32 count = idx + (acts[idx] != Action::Invalid); // Count total actions placed in the array. Do the + again in case the last action wasn't Invalid.
				const u32 pick = gen.next(count + (count == 0)); // Pick an action index in [0, count) if count > 0, or in [0, 1) if count == 0 (aka pick 0).
				switch (acts[pick]) {
				case Invalid: { return false; } // Being here means no actions were available
				case NoWpnP: { data[NoWeapons].set_periodic(); data[NoWeapons].set_obeying(not hands.any_weapon()); break; }
				case NoWpnF: { data[NoWeapons].set_flat(); break; }
				case NoSplP: { data[NoSpells].set_periodic(); data[NoSpells].set_obeying(not hands.any_spell()); break; }
				case NoSplF: { data[NoSpells].set_flat(); break; }
				case NoSolesP: { data[NoSolesOnly].set_periodic(); data[NoSolesOnly].set_obeying(flagpack.soleless_or_barefoot()); break; }
				case NoSolesF: { data[NoSolesOnly].set_flat(); break; }
				default: { __assume(0); }
				}
				return count > 1; // Being here means at least 1 action was available
			}

			constexpr i32 handequipped(const HandEquippedFlags hef) noexcept {
				const size_t index = hef.is_spell(); // NoWeapons (0) if not spell, NoSpells (1) if spell
				data[index].unset_obeying();
				return data[index].flat_active();
			}
			constexpr i32 feetequipped(const SlotFlags slot) noexcept {
				const bool nosoles = slot.nosoles();
				data[NoSolesOnly].set_obeying(nosoles); // We know feet were equipped, so we can outright set here
				progress[NoSolesOnly] = mask_float(progress[NoSolesOnly], nosoles);
				return data[NoSolesOnly].flat_active() & (not nosoles); // Return 1 if not nosoles and flat active
			}
			constexpr void handunequipped(const HandsFlags newhands) noexcept {
				const bool wp = newhands.any_weapon();
				const bool sp = newhands.any_spell();
				data[NoWeapons].set_obeying(not wp);
				data[NoSpells].set_obeying(not sp);
				progress[NoWeapons] = mask_float(progress[NoWeapons], wp);
				progress[NoSpells] = mask_float(progress[NoSpells], sp);
			}
			constexpr void armorunequipped(const AnalyzedBasicSlots flagpack) noexcept {
				const bool newobeying = flagpack.soleless_or_barefoot();
				data[NoSolesOnly].set_obeying(newobeying);
				progress[NoSolesOnly] = mask_float(progress[NoSolesOnly], not newobeying);
			}

			constexpr i32 update(const float hour_delta) noexcept {
				progress[NoWeapons] += mask_float(hour_delta, data[NoWeapons].periodic_active());
				progress[NoSpells] += mask_float(hour_delta, data[NoSpells].periodic_active());
				progress[NoSolesOnly] += mask_float(hour_delta, data[NoSolesOnly].periodic_active());

				i32 result = 0;

				i32 intg = static_cast<i32>(progress[NoWeapons]); // Keep integer part only
				progress[NoWeapons] -= static_cast<float>(intg);
				result += intg;

				intg = static_cast<i32>(progress[NoSpells]);
				progress[NoSpells] -= static_cast<float>(intg);
				result += intg;

				intg = static_cast<i32>(progress[NoSolesOnly]);
				progress[NoSolesOnly] -= static_cast<float>(intg);
				result += intg;

				return result;
			}

			array<packed_pof, Total> data{};	// 3 bits for periodic-obeying-flat, 5 bits unused.
			char pad;
			array<float, Total> progress{};
		};
		static_assert(sizeof(CostersEquip) == 16);

		struct CostersEquipTiered {
			enum ID : u8 {
				LightRestricted,	// Equip
				HeavyRestricted,	// Equip
				ClothingRestricted,	// Equip

				Total
			};

			constexpr bool any_inactive() const noexcept {
				static_assert(Total == 3, "CostersEquipTiered::any_inactive() needs updating because number of rules has changed");
				u8 accum = data[LightRestricted].raw();
				accum &= data[HeavyRestricted].raw();
				accum &= data[ClothingRestricted].raw();
				return (accum & packed_poftier::FullyActiveValue) != packed_poftier::FullyActiveValue; // If !=, at least one rule is inactive and/or at least 1 rule is not at max tier.
			}

			constexpr bool activate_random_check_full(RNG::gamerand& gen, const AnalyzedBasicSlots flagpack) noexcept {
				static_assert(Total == 3, "CostersEquipTiered::activate_random_check_full() needs updating because number of rules has changed");
				enum Action : u8 {
					Invalid = 0,

					LightP,
					LightF,
					LightU,
					HeavyP,
					HeavyF,
					HeavyU,
					ClothP,
					ClothF,
					ClothU,

					Total = ClothU
				};
				// Fill array with possible actions
				array<Action, Action::Total> acts{};
				u32 idx = 0;
				acts[idx] = static_cast<Action>(LightP & bool_extend(data[LightRestricted].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(LightF & bool_extend(data[LightRestricted].flat_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(LightU & bool_extend(data[LightRestricted].can_upgrade()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(HeavyP & bool_extend(data[HeavyRestricted].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(HeavyF & bool_extend(data[HeavyRestricted].flat_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(HeavyU & bool_extend(data[HeavyRestricted].can_upgrade()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(ClothP & bool_extend(data[ClothingRestricted].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(ClothF & bool_extend(data[ClothingRestricted].flat_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(ClothU & bool_extend(data[ClothingRestricted].can_upgrade()));
				const u32 count = idx + (acts[idx] != Action::Invalid); // Count total actions placed in the array. Do the + again in case the last action wasn't Invalid.
				const u32 pick = gen.next(count + (count == 0)); // Pick an action index in [0, count) if count > 0, or in [0, 1) if count == 0 (aka pick 0).
				bool ret = count > 1; // Will be set to true if upgrading without reaching max tier
				switch (acts[pick]) {
				case Invalid: { return false; } // Being here means no action was available
				case LightP: {
					data[LightRestricted].set_periodic();
					data[LightRestricted].set_tier(1);
					data[LightRestricted].set_obeying(flagpack.light().not_worn() bitor (flagpack.light().tiers() != 0));
					break;
				}
				case LightF: { data[LightRestricted].set_flat(); break; }
				case LightU: {
					ret |= data[LightRestricted].upgrade();
					data[LightRestricted].set_obeying(flagpack.light().not_worn() bitor (data[LightRestricted].tier() <= flagpack.light().tiers()));
					break;
				}
				case HeavyP: {
					data[HeavyRestricted].set_periodic();
					data[HeavyRestricted].set_tier(1);
					data[HeavyRestricted].set_obeying(flagpack.heavy().not_worn() bitor (flagpack.heavy().tiers() != 0));
					break;
				}
				case HeavyF: { data[HeavyRestricted].set_flat(); break; }
				case HeavyU: {
					ret |= data[HeavyRestricted].upgrade();
					data[HeavyRestricted].set_obeying(flagpack.heavy().not_worn() bitor (data[HeavyRestricted].tier() <= flagpack.heavy().tiers()));
					break;
				}
				case ClothP: {
					data[ClothingRestricted].set_periodic();
					data[ClothingRestricted].set_tier(1);
					data[ClothingRestricted].set_obeying(flagpack.clothing().not_worn() bitor (flagpack.clothing().tiers() != 0));
					break;
				}
				case ClothF: { data[ClothingRestricted].set_flat(); break; }
				case ClothU: {
					ret |= data[ClothingRestricted].upgrade();
					data[ClothingRestricted].set_obeying(flagpack.clothing().not_worn() bitor (data[ClothingRestricted].tier() <= flagpack.clothing().tiers()));
					break;
				}
				default: { __assume(0); }
				}
				return ret; // Being here at means least 1 action was available
			}

			constexpr i32 armorequipped(const SlotFlags slot) noexcept {
				const u32 index = slot.type_index(); // 0:light 1:heavy 2:clothing
				const bool allowed = data[index].tier() <= slot.tiers(); // Required tier <= means piece satisfies the rule
				data[index].andeq_obeying(allowed); // Make obeying false if not allowed
				return data[index].flat_active() & (not allowed); // Return 1 if not allowed and flat active
			}
			constexpr void armorunequipped(const AnalyzedBasicSlots flagpack) noexcept {
				SlotFlags sf = flagpack.light();
				bool ob = sf.not_worn() bitor (data[LightRestricted].tier() <= sf.tiers());	// obeying = tier match or not worn
				data[LightRestricted].set_obeying(ob);			// Set it
				progress[LightRestricted] = mask_float(progress[LightRestricted], ob);		// Zero progress if obeying

				sf = flagpack.heavy();
				ob = sf.not_worn() bitor (data[HeavyRestricted].tier() <= sf.tiers());
				data[HeavyRestricted].set_obeying(ob);
				progress[HeavyRestricted] = mask_float(progress[HeavyRestricted], ob);

				sf = flagpack.clothing();
				ob = sf.not_worn() bitor (data[ClothingRestricted].tier() <= sf.tiers());
				data[ClothingRestricted].set_obeying(ob);
				progress[ClothingRestricted] = mask_float(progress[ClothingRestricted], ob);
			}

			constexpr i32 update(const float hour_delta) noexcept {
				progress[LightRestricted] += mask_float(hour_delta, data[LightRestricted].periodic_active());
				progress[HeavyRestricted] += mask_float(hour_delta, data[HeavyRestricted].periodic_active());
				progress[ClothingRestricted] += mask_float(hour_delta, data[ClothingRestricted].periodic_active());

				i32 result = 0;

				i32 intg = static_cast<i32>(progress[LightRestricted]); // Keep integer part only
				progress[LightRestricted] -= static_cast<float>(intg);
				result += intg;

				intg = static_cast<i32>(progress[HeavyRestricted]);
				progress[HeavyRestricted] -= static_cast<float>(intg);
				result += intg;

				intg = static_cast<i32>(progress[ClothingRestricted]);
				progress[ClothingRestricted] -= static_cast<float>(intg);
				result += intg;

				return result;
			}

			array<packed_poftier, Total> data{};	// 3 bits for periodic-obeying-flat, 5 bits for tier.
			char pad;
			array<float, Total> progress{};
		};
		static_assert(sizeof(CostersEquipTiered) == 16);

		struct PreventersEquip {
			enum ID : u8 {
				WearHR_1,		// Equip
				WearHR_2,		// Equip
				WearHR_3,		// Equip
				WearHR_9,			// Equip
				WearHR_4,			// Equip
				WearHR_5,			// Equip
				WearHR_6,			// Equip
				WearHR_7,	// Equip
				WearHR_8,	// Equip
				WearHR_10_13,			// Equip

				WearNoSoles,		// Equip

				Total
			};

			__forceinline constexpr bool any_inactive() const noexcept { return data.any_inactive(); }
			__forceinline constexpr bool allowed() const noexcept { return data.allowed(); }

			constexpr bool activate_random_check_full(RNG::gamerand& gen, const AnalyzedBasicSlots flagpack, const EquipState::HRCounts& hr) noexcept {
				const u16 olds = data.actives();
				const bool ret = data.activate_random_check_full(gen);
				unsigned long idx;
				const u8 scanmask = bool_extend(_BitScanForward(&idx, static_cast<u32>(olds ^ data.actives()))); // Up to 1 bit was set so XORing will isolate it.
				const ID newid = static_cast<ID>((static_cast<u8>(idx) & (scanmask)) | (static_cast<u8>(Total) & static_cast<u8>(~scanmask))); // If scanmask != 0, some rule was activated.
				switch (newid) {
				case WearHR_1:
				case WearHR_2:
				case WearHR_3:
				case WearHR_4:
				case WearHR_5:
				case WearHR_6:
				case WearHR_7:
				case WearHR_8:
				case WearHR_9:		{ data.set_obeyed(newid, hr.has(static_cast<HRKwd>(newid))); break; }
				case WearHR_10_13:	{ data.set_obeyed(newid, hr.has_any_10_13()); break; }
				case WearNoSoles:	{ data.set_obeyed(WearNoSoles, flagpack.no_soles_worn()); break; }
				case Total:			{ break; } // Nothing added
				default:			{ __assume(0); }
				}
				return ret;
			}

			constexpr void feetequipped(const SlotFlags slot) noexcept { data.set_obeyed(WearNoSoles, slot.nosoles()); }
			constexpr void hrequipped(const ArmorEquippedFlags::HRFlags hr) noexcept {
				// Hurdle keywords enum is sorted so that it matches the ID enum up to HR10. So the LSB in hr is true if the piece is HR1, etc.
				// HR10 is exceptional in that it can also be satisfied by the HR13 keyword which is some bit after the HR10.
				enum : u32 {
					HR13Mask = 1 << static_cast<u32>(HRKwd::Hurdle_13),
					To10Shift = static_cast<u32>(HRKwd::Hurdle_13) - static_cast<u32>(HRKwd::Hurdle_10),
					RelevantMask = 0b11'1111'1111, // 10 lowest bits
				};
				const u32 hr13 = (hr.f & HR13Mask) >> To10Shift; // Isolate HR13 bit and shift it to Gag position.

				const u32 relevant = ((hr.f & RelevantMask) | hr13) & data.f;	// Keep only 10 lowest keyword bits, then | the hr13 so HR13 satisfies HR10, then clear any bit whose rule is not active.
				data.f |= (relevant << 16);										// Set all obeying bits of the equipped keywords whose rule is active.
			}
			constexpr void armorunequipped(const AnalyzedBasicSlots flagpack, const EquipState::HRCounts& hr) noexcept {
				data.set_obeyed(WearNoSoles, flagpack.no_soles_worn()); // Must specifically have nosoles. Barefoot violates.

				enum : u32 {
					HRActiveMask = (1 << (WearHR_9 + 1)) - 1,	// bits 0-9   / 0b11'1111'1111
					HRObeyingMask = HRActiveMask << 16,			// bits 16-25 / 0b11'1111'1111'0000'0000'0000'0000
					NotHRObeyingMask = ~HRObeyingMask,
				};
				data.f &= NotHRObeyingMask;	// Always unset all HR obeyings in data.
				if ((data.raw() bitand HRActiveMask) bitand not hr.empty()) {
					u32 hrf = 0;
					hrf |= static_cast<u32>(hr.has(HRKwd::Hurdle_1)); // << WearHR_1 which is 0
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_2)) << WearHR_2);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_3)) << WearHR_3);
					hrf |= (static_cast<u32>(hr.has_any_10_13()) << WearHR_10_13);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_5)) << WearHR_4);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_6)) << WearHR_5);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_7)) << WearHR_6);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_8)) << WearHR_7);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_9)) << WearHR_8);
					hrf |= (static_cast<u32>(hr.has(HRKwd::Hurdle_4)) << WearHR_9);
					hrf &= data.f;				// Unset all inactives in new obeyings
					data.f |= (hrf << 16);		// Set new obeyings
				}
			}

			packed_aopairs<ID> data{};
		};
		static_assert(sizeof(PreventersEquip) == 4);

		struct PreventersEquipTiered {
			enum ID : u8 {
				LightRestricted,	// Equip
				HeavyRestricted,	// Equip
				ClothingRestricted,	// Equip

				Total
			};

			constexpr bool any_inactive() const noexcept {
				static_assert(Total == 3, "PreventersEquipTiered::any_inactive() needs updating because number of rules has changed");
				u8 accum = data[LightRestricted].raw();
				accum &= data[HeavyRestricted].raw();
				accum &= data[ClothingRestricted].raw();
				return (accum & packed_poftier::FullyActiveValue) != packed_poftier::FullyActiveValue; // If !=, at least one rule is inactive and/or at least 1 rule is not at max tier.
			}

			constexpr bool allowed() const noexcept {
				return	data[LightRestricted].periodic_inactive_or_obeyed() &
						data[HeavyRestricted].periodic_inactive_or_obeyed() &
						data[ClothingRestricted].periodic_inactive_or_obeyed();
			}

			constexpr bool activate_random_check_full(RNG::gamerand& gen, const AnalyzedBasicSlots flagpack) noexcept {
				static_assert(Total == 3, "PreventersEquipTiered::activate_random_check_full() needs updating because number of rules has changed");
				enum Action : u8 {
					Invalid = 0,

					LightP,
					LightU,
					HeavyP,
					HeavyU,
					ClothP,
					ClothU,

					Total = ClothU
				};
				// Fill array with possible actions
				array<Action, Action::Total> acts{};
				u32 idx = 0;
				acts[idx] = static_cast<Action>(LightP & bool_extend(data[LightRestricted].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(LightU & bool_extend(data[LightRestricted].can_upgrade()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(HeavyP & bool_extend(data[HeavyRestricted].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(HeavyU & bool_extend(data[HeavyRestricted].can_upgrade()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(ClothP & bool_extend(data[ClothingRestricted].periodic_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(ClothU & bool_extend(data[ClothingRestricted].can_upgrade()));
				const u32 count = idx + (acts[idx] != Action::Invalid); // Count total actions placed in the array. Do the + again in case the last action wasn't Invalid.
				const u32 pick = gen.next(count + (count == 0)); // Pick an action index in [0, count) if count > 0, or in [0, 1) if count == 0 (aka pick 0).
				bool ret = count > 1; // Will be set to true if upgrading without reaching max tier
				switch (acts[pick]) {
				case Invalid: { return false; } // Being here means no action was available
				case LightP: {
					data[LightRestricted].set_periodic();
					data[LightRestricted].set_tier(1);
					data[LightRestricted].set_obeying(flagpack.light().not_worn() bitor (flagpack.light().tiers() != 0));
					break;
				}
				case LightU: {
					ret |= data[LightRestricted].upgrade();
					data[LightRestricted].set_obeying(flagpack.light().not_worn() bitor (data[LightRestricted].tier() <= flagpack.light().tiers()));
					break;
				}
				case HeavyP: {
					data[HeavyRestricted].set_periodic();
					data[HeavyRestricted].set_tier(1);
					data[HeavyRestricted].set_obeying(flagpack.heavy().not_worn() bitor (flagpack.heavy().tiers() != 0));
					break;
				}
				case HeavyU: {
					ret |= data[HeavyRestricted].upgrade();
					data[HeavyRestricted].set_obeying(flagpack.heavy().not_worn() bitor (data[HeavyRestricted].tier() <= flagpack.heavy().tiers()));
					break;
				}
				case ClothP: {
					data[ClothingRestricted].set_periodic();
					data[ClothingRestricted].set_tier(1);
					data[ClothingRestricted].set_obeying(flagpack.clothing().not_worn() bitor (flagpack.clothing().tiers() != 0));
					break;
				}
				case ClothU: {
					ret |= data[ClothingRestricted].upgrade();
					data[ClothingRestricted].set_obeying(flagpack.clothing().not_worn() bitor (data[ClothingRestricted].tier() <= flagpack.clothing().tiers()));
					break;
				}
				default: { __assume(0); }
				}
				return ret; // Being here at means least 1 action was available
			}

			constexpr void armorequipped(const SlotFlags slot) noexcept {
				const u32 index = slot.type_index(); // 0:light 1:heavy 2:clothing
				const bool allowed = data[index].tier() <= slot.tiers(); // Required tier <= means piece satisfies the rule
				data[index].andeq_obeying(allowed); // Make obeying false if not allowed
			}
			constexpr void armorunequipped(const AnalyzedBasicSlots flagpack) noexcept {
				SlotFlags sf = flagpack.light();
				bool ob = sf.not_worn() bitor (data[LightRestricted].tier() <= sf.tiers());	// obeying = tier match or not worn
				data[LightRestricted].set_obeying(ob);

				sf = flagpack.heavy();
				ob = sf.not_worn() bitor (data[HeavyRestricted].tier() <= sf.tiers());
				data[HeavyRestricted].set_obeying(ob);

				sf = flagpack.clothing();
				ob = sf.not_worn() bitor (data[ClothingRestricted].tier() <= sf.tiers());
				data[ClothingRestricted].set_obeying(ob);
			}

			array<packed_poftier, Total> data{}; // p = rule active, o = obeying, f = unused, t = rule tier
		};
		static_assert(sizeof(PreventersEquipTiered) == 3);

		struct PreventerDodge {
			enum ID : u32 {
				Dodge,	// Papyrus
			};

			__forceinline constexpr bool active() const noexcept { return target; }
			__forceinline constexpr bool allowed() const noexcept { return remaining == 0; }

			constexpr bool activate_check_full() noexcept {
				target += u8{ 2 } & bool_extend(target < 200);
				return target < 200;
			}

			__forceinline constexpr void dodged() noexcept { --remaining; }
			__forceinline constexpr void update(const bool day_passed) noexcept { remaining += (target - remaining) * day_passed; }

			u8 target{};		// Active if != 0
			satu8 remaining{};
		};
		static_assert(sizeof(PreventerDodge) == 2);

		struct PreventersSL {
			enum ID : u32 {
				BrawlBreton,	// SL brawl modevent
				BrawlImperial,	// SL brawl modevent
				BrawlNord,		// SL brawl modevent
				BrawlRedguard,	// SL brawl modevent
				BrawlOrsimer,	// SL brawl modevent
				BrawlAltmer,	// SL brawl modevent
				BrawlDunmer,	// SL brawl modevent
				BrawlBosmer,	// SL brawl modevent
				BrawlKhajit,	// SL brawl modevent
				BrawlArgonian,	// SL brawl modevent
				DoUnfairBrawl,	// SL brawl modevent
				DoRawBrawl,		// SL brawl modevent
				DoAngryBrawl,	// SL brawl modevent
				DoMerryBrawl,	// SL brawl modevent

				Total
			};

			__forceinline constexpr bool any_inactive() const noexcept { return data.any_inactive(); }
			__forceinline constexpr bool allowed() const noexcept { return data.allowed(); }

			constexpr bool activate_random_check_full(RNG::gamerand& gen) noexcept { return data.activate_random_check_full(gen); }

			__forceinline constexpr void brawled(const ID id) noexcept { data.set_obeyed(id); }
			__forceinline constexpr void unfairbrawl() noexcept { data.set_obeyed(DoUnfairBrawl); }
			__forceinline constexpr void rawbrawl() noexcept { data.set_obeyed(DoRawBrawl); }
			__forceinline constexpr void angrybrawl() noexcept { data.set_obeyed(DoAngryBrawl); }
			__forceinline constexpr void merrybrawl() noexcept { data.set_obeyed(DoMerryBrawl); }

			__forceinline constexpr void update(const bool day_passed) noexcept { data.f &= (bool_extend(not day_passed) << 16); }

			packed_aopairs<ID> data;
		};
		static_assert(sizeof(PreventersSL) == 4);

		struct MiscPunishers {
			enum ID : u8 {
				NoFastTravel,	// Fast Travel event
				HaveFollower,	// Update

				Total
			};

			__forceinline constexpr bool any_inactive() const noexcept { return data.periodic_inactive() | data.flat_inactive(); }
			__forceinline constexpr bool allowed() const noexcept { return data.periodic_inactive() bitor data.obeying(); }

			constexpr bool activate_random_check_full(RNG::gamerand& gen, const bool follower) noexcept {
				static_assert(Total == 2, "MiscPunishers::activate_random_check_full() needs updating because number of rules has changed");
				enum Action : u8 {
					Invalid = 0,

					NoFastA,
					NoFastU,
					HaveFlA,

					Total = HaveFlA
				};

				array<Action, Action::Total> acts{};
				u32 idx = 0;
				acts[idx] = static_cast<Action>(NoFastA & bool_extend(data.flat_inactive()));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(NoFastU & bool_extend(data.flat_active() & (data.tier() < 10)));
				acts[idx += (acts[idx] != Action::Invalid)] = static_cast<Action>(HaveFlA & bool_extend(data.periodic_inactive()));
				const u32 count = idx + (acts[idx] != Action::Invalid); // Count total actions placed in the array. Do the + again in case the last action wasn't Invalid.
				const u32 pick = gen.next(count + (count == 0)); // Pick an action index in [0, count) if count > 0, or in [0, 1) if count == 0 (aka pick 0).
				bool ret = count > 1;
				switch (acts[pick]) {
				case Invalid: { return false; } // Being here means no actions were available
				case NoFastA: { data.set_flat(); data.set_tier(1); break; }
				case NoFastU: { const u8 oldtier = data.tier(); ret |= (oldtier < 9); data.set_tier(oldtier + 1); break; }
				case HaveFlA: { data.set_periodic(); data.set_obeying(follower); break; }
				default: { __assume(0); }
				}
				return ret; // Being here means at least 1 action was available
			}

			__forceinline constexpr bool nofasttravel_active() const noexcept { return data.flat_active(); }
			__forceinline constexpr i32 nofasttravel_penalty() const noexcept { return static_cast<i32>(data.tier() & bool_extend(nofasttravel_active())); }

			__forceinline constexpr void set_havefollower_inactive() noexcept { data.unset_periodic(); }
			__forceinline constexpr void set_havefollower_obeying(const bool val) noexcept { data.set_obeying(val); }

		private:
			packed_poftier data{};	// Periodic = HaveFollower, Obeying = HaveFollower obeying, Flat = NoFastTravel, Tier = NoFastTravel penalty
		};
		static_assert(sizeof(MiscPunishers) == 1);

		
	}
	using namespace Punishers;

	namespace Granters {

		struct GranterBig {
			enum ID : u32 {
				GetHit,
				KillDraugr,

				Total
			};
			enum : u8 {
				GoalMultiplier = 10,
				DifficultyMask = 0b1111'1110,
				DifficultyMaskedMax = 100 << 1,
			};

			__forceinline constexpr bool active() const noexcept { return fd & DifficultyMask; }
			__forceinline constexpr bool remove_on_completion() const noexcept { return (fd & DifficultyMask) > DifficultyMaskedMax; }
			__forceinline constexpr u8 difficulty() const noexcept { return fd >> 1; }
			__forceinline constexpr u16 remaining() const noexcept { return static_cast<u16>((fd & 1) << 8) | static_cast<u16>(fr); }
			__forceinline constexpr u16 goal() const noexcept { return static_cast<u16>(fd >> 1) * GoalMultiplier; }
			__forceinline constexpr u16 progress() const noexcept { return goal() - remaining(); }

			__forceinline constexpr void set_remove_on_completion(const bool val) noexcept {
				fd |= static_cast<u8>(DifficultyMask & bool_extend(val));
			}
			__forceinline constexpr void set_difficulty(const sat1100u8 val) noexcept {
				fd &= static_cast<u8>(~DifficultyMask);
				fd |= static_cast<u8>(static_cast<u8>(val.get()) << 1);
			}
			__forceinline constexpr void set_remaining(const sat0500u16 val) noexcept {
				fd &= DifficultyMask;
				fd |= static_cast<u8>((val.get() >> 8) & 1);
				fr = static_cast<u8>(val.get() & 0b1111'1111);
			}

			constexpr i32 advance(const u16 val) noexcept {
				if (!active()) {
					return 0;
				}
				if (const u16 oldrem = remaining(); val >= oldrem) {
					if (remove_on_completion()) {
						fd &= 1; // Set inactive
						return 1;
					}
					else {
						const u32 excess = val - oldrem; // Absolute overshoot
						const u16 goal_ = goal();

						const u16 newrem = goal_ - static_cast<u16>(excess % goal_); // Divide goal into it, and sub the remainder from goal for new remaining
						fd &= DifficultyMask; // Erase previous
						fd |= static_cast<u8>((newrem >> 8) & 1);
						fr = static_cast<u8>(newrem & 0b1111'1111);

						return static_cast<i32>(excess / goal_) + 1;
					}
				} else {
					const u16 newrem = static_cast<u16>(oldrem - val);
					fd &= DifficultyMask; // Erase previous
					fd |= static_cast<u8>((newrem >> 8) & 1);
					fr = static_cast<u8>(newrem & 0b1111'1111);
					return 0;
				}
			}

			constexpr bool init(const sat1100u8 difficulty, const bool roc) noexcept {
				if (not active()) {
					set_difficulty(difficulty);
					set_remaining(difficulty.u16() * GoalMultiplier);
					set_remove_on_completion(roc);
					return true;
				}
				return false;
			}
			__forceinline constexpr bool set_inactive() noexcept {
				const bool old = active();
				fd &= 1;
				return old;
			}

			// difficulty:	[1, 100]	7 bits, with values 0, 101-127 unused
			// remaining:	[0, 500]	9 bits, with values 501-511 unused
			// active:		bool		no storage, represented by difficulty != 0
			// roc:			bool		no storage, represented by difficulty > 100
			// 		Total 16 bits	(u8) (u8):	(dddd dddr) (rrrr rrrr)
			u8 fd{};
			u8 fr{};
		};
		static_assert(sizeof(GranterBig) == 2);

		struct GranterMed {
			enum ID : u32 {
				Dodge,
				KillDaedra,

				Total
			};
			enum : u8 {
				GoalMultiplier = 2,
				DifficultyMask = 0b0111'1111,
				RoCMask = 0b1000'0000,
			};

			__forceinline constexpr bool active() const noexcept { return fd & DifficultyMask; }
			__forceinline constexpr bool remove_on_completion() const noexcept { return fd >> 7; } // Better code than fd & RoCMask
			__forceinline constexpr u8 difficulty() const noexcept { return fd & DifficultyMask; }
			__forceinline constexpr u8 goal() const noexcept { return (fd & DifficultyMask) * GoalMultiplier; }
			__forceinline constexpr u8 remaining() const noexcept { return fr; }
			__forceinline constexpr u8 progress() const noexcept { return goal() - remaining(); }

			__forceinline constexpr void set_remove_on_completion(const bool val) noexcept {
				fd &= DifficultyMask;
				fd |= static_cast<u8>(static_cast<u8>(val) << 7);
			}
			__forceinline constexpr void set_difficulty(const sat1100u8 val) noexcept {
				fd &= RoCMask;
				fd |= val.get(); // No masking needed. Invariantly capped to 100 so MSB is never set.
			}
			__forceinline constexpr void set_remaining(const sat0200u8 val) noexcept {
				fr = val.get();
			}

			constexpr i32 advance(const u32 val) noexcept {
				if (!active()) {
					return 0;
				}
				if (const u8 oldrem = remaining(); val >= oldrem) {
					if (remove_on_completion()) {
						fd &= RoCMask; // Set inactive
						return 1;
					} else {
						const u32 excess = val - oldrem; // Absolute overshoot
						const u8 goal_ = goal();

						const u8 newrem = goal_ - static_cast<u8>(excess % goal_); // Divide goal into it, and sub the remainder from goal for new remaining
						fr = newrem;

						return static_cast<i32>(excess / goal_) + 1;
					}
				} else {
					const u8 newrem = static_cast<u8>(fr - val);
					fr = newrem;
					return 0;
				}
			}

			constexpr bool init(const sat1100u8 difficulty, const bool roc) noexcept {
				if (not active()) {
					set_difficulty(difficulty);
					set_remaining(difficulty.get() * GoalMultiplier);
					set_remove_on_completion(roc);
					return true;
				}
				return false;
			}
			__forceinline constexpr bool set_inactive() noexcept {
				const bool old = active();
				fd &= RoCMask; // Not sure if RoC value needs to be preserved but w/e
				return old;
			}

			// difficulty:	[1, 100]	7 bits, with values 0, 101-127 unused
			// remaining:	[0, 200]	8 bits, with values 201-255 unused
			// active:		bool		no storage, represented by difficulty != 0
			// roc:			bool		1 bit, stored in difficulty's MSB
			// 		Total 16 bits	(u8) (u8):	(cddd dddd) (rrrr rrrr)
			u8 fd{};
			u8 fr{};
		};
		static_assert(sizeof(GranterMed) == 2);

		struct GranterSmall {
			enum ID : u32 {
				MakeOthersFear,
				AbsorbDragonSouls,

				Total 
			};
			enum : u8 {
				ActiveMask = 0b1000'0000,
				GoalMask = static_cast<u8>(~ActiveMask),
				RoCMask = ActiveMask,
				RemainingMask = GoalMask,
			};

			__forceinline constexpr bool active() const noexcept { return fd & ActiveMask; }
			__forceinline constexpr bool remove_on_completion() const noexcept { return fr & RoCMask; }
			__forceinline constexpr u8 goal() const noexcept { return fd & GoalMask; }
			__forceinline constexpr u8 remaining() const noexcept { return fr & RemainingMask; }
			__forceinline constexpr u8 progress() const noexcept { return goal() - remaining(); }

			__forceinline constexpr void set_remove_on_completion(const bool val) noexcept {
				fr &= RemainingMask;
				fr |= static_cast<u8>(static_cast<u8>(val) << 7);
			}
			__forceinline constexpr void set_goal(const sat1100u8 val) noexcept {
				fd &= ActiveMask;
				fd |= val.get();
			}
			__forceinline constexpr void set_remaining(const sat0100u8 val) noexcept {
				fr &= RoCMask;
				fr |= val.get();
			}

			constexpr i32 advance(const u32 val) noexcept {
				if (!active()) {
					return 0;
				}
				if (const u8 oldrem = remaining(); val >= oldrem) {
					if (remove_on_completion()) {
						fd &= GoalMask; // Set inactive
						return 1;
					} else {
						const u32 excess = val - oldrem; // Absolute overshoot
						const u8 goal_ = goal();

						const u8 newrem = goal_ - static_cast<u8>(excess % goal_); // Divide goal into it, and sub the remainder from goal for new remaining
						fr &= RoCMask; // Erase previous
						fr |= newrem;

						return static_cast<i32>(excess / goal_) + 1;
					}
				} else {
					const u8 newrem = static_cast<u8>(oldrem - val);
					fr &= RoCMask; // Erase previous
					fr |= newrem;
					return 0;
				}
			}

			constexpr bool init(const sat1100u8 goal, const bool roc) noexcept {
				if (not active()) {
					set_goal(goal);
					set_remaining(goal.get());
					set_remove_on_completion(roc);
					fd |= ActiveMask;
					return true;
				}
				return false;
			}
			__forceinline constexpr bool set_inactive() noexcept {
				const bool old = active();
				fd &= GoalMask;
				return old;
			}

			// goal:		[1, 100]	7 bits, with values 0, 101-127 unused
			// remaining:	[0, 100]	7 bits, with values 101-127 unused
			// active:		bool		1 bit, stored in goal's MSB
			// roc:			bool		1 bit, stored in remaining's MSB
			// 		Total 16 bits	(u8) (u8):	(aggg gggg) (crrr rrrr)
			u8 fd{};
			u8 fr{};
		};
		static_assert(sizeof(GranterSmall) == 2);

		struct GranterTiny {
			enum ID : u32 {
				KillDragonPriests,

				Total 
			};
			enum : u8 {
				GoalMask = 0b1111'0000,
				RemainingMask = 0b0000'1111,
			};

			__forceinline constexpr bool active() const noexcept { return f & GoalMask; }
			__forceinline constexpr bool remove_on_completion() const noexcept { return (f & GoalMask) == GoalMask; }
			__forceinline constexpr u8 goal() const noexcept { return f >> 4; }
			__forceinline constexpr u8 remaining() const noexcept { return f & RemainingMask; }
			__forceinline constexpr u8 progress() const noexcept { return goal() - remaining(); }

			__forceinline constexpr void set_remove_on_completion(const bool val) noexcept {
				f |= static_cast<u8>(GoalMask & bool_extend(val));
			}
			__forceinline constexpr void set_goal(const sat114u8 val) noexcept {
				f &= RemainingMask;
				f |= static_cast<u8>(val.get() << 4);
			}
			__forceinline constexpr void set_remaining(const sat014u8 val) noexcept {
				f &= GoalMask;
				f |= static_cast<u8>(val.get() & RemainingMask);
			}

			constexpr i32 advance(const u32 val) noexcept {
				if (!active()) {
					return 0;
				}
				if (const u8 oldrem = remaining(); val >= oldrem) {
					if (remove_on_completion()) {
						f &= RemainingMask; // Set inactive
						return 1;
					} else {
						const u32 excess = val - oldrem; // Absolute overshoot
						const u8 goal_ = goal();

						const u8 newrem = goal_ - static_cast<u8>(excess % goal_); // Divide goal into it, and sub the remainder from goal for new remaining
						f &= GoalMask; // Erase previous
						f |= newrem;

						return static_cast<i32>(excess / goal_) + 1;
					}
				} else {
					const u8 newrem = static_cast<u8>(oldrem - val); // If val > oldrem, zero it. Else, subtract val.
					f &= GoalMask; // Erase previous
					f |= newrem;
					return 0;
				}
			}

			constexpr bool init(const sat114u8 goal, const bool roc) noexcept {
				if (not active()) {
					set_goal(goal);
					set_remaining(goal.get());
					set_remove_on_completion(roc);
					return true;
				}
				return false;
			}
			__forceinline constexpr bool set_inactive() noexcept {
				const bool old = active();
				f &= RemainingMask; // Not sure if remaining value needs to be preserved but w/e
				return old;
			}

			// goal:		[1, 14]		4 bits, with values 0, 15 unused
			// remaining:	[0, 14]		4 bits, with value 15 unused
			// active:		bool		1 bit, not stored, represented by goal being != 0
			// roc:			bool		1 bit, not stored, represented by goal being == 15
			// 		Total 8 bits	(u8):	(gggg rrrr)
			u8 f{};
		};
		static_assert(sizeof(GranterTiny) == 1);

		struct GranterTimed {
			enum ID : u32 {
				StayUnarmored,
				StayDemagicked,
				StayBurdened,

				Total 
			};
			enum : u8 {
				ActiveMask = 0b1000'0000,
				RoCMask = 0b0100'0000,

				GoalMask = 0b0011'1000,

				LowNumMask = 0b0000'0111,
				RemainingMask = LowNumMask,
			};

			__forceinline constexpr bool active() const noexcept { return f & ActiveMask; }
			__forceinline constexpr bool remove_on_completion() const noexcept { return f & RoCMask; }
			__forceinline constexpr u8 goal() const noexcept { return static_cast<u8>(f >> 3) & LowNumMask; }
			__forceinline constexpr u8 remaining() const noexcept { return f & LowNumMask; }
			__forceinline constexpr u8 progress() const noexcept { return goal() - remaining(); }

			__forceinline constexpr void set_remove_on_completion(const bool val) noexcept {
				f &= static_cast<u8>(~RoCMask);
				f |= static_cast<u8>(val << 6);
			}
			__forceinline constexpr void set_goal(const sat17u8 val) noexcept {
				f &= static_cast<u8>(~GoalMask);
				f |= static_cast<u8>(val.get() << 3);
			}
			__forceinline constexpr void set_remaining(const sat07u8 val) noexcept {
				f &= static_cast<u8>(~RemainingMask);
				f |= val.get();
			}

			constexpr i32 advance(const u32 val) noexcept {
				if (!active()) {
					return 0;
				}
				if (const u8 oldrem = remaining(); val >= oldrem) {
					if (remove_on_completion()) {
						f &= static_cast<u8>(~ActiveMask); // Set inactive
						return 1;
					} else {
						const u32 excess = val - oldrem; // Absolute overshoot
						const u8 goal_ = goal();

						const u8 newrem = goal_ - static_cast<u8>(excess % goal_); // Divide goal into it, and sub the remainder from goal for new remaining
						f &= static_cast<u8>(~RemainingMask); // Erase previous
						f |= newrem;

						return static_cast<i32>(excess / goal_) + 1;
					}
				} else {
					const u8 newrem = static_cast<u8>(oldrem - val);
					f &= static_cast<u8>(~RemainingMask); // Erase previous
					f |= newrem;
					return 0;
				}
			}

			constexpr bool init(const sat17u8 goal, const bool roc) noexcept {
				if (not active()) {
					set_goal(goal);
					set_remaining(goal.get());
					set_remove_on_completion(roc);
					f |= ActiveMask;
					return true;
				}
				return false;
			}
			__forceinline constexpr bool set_inactive() noexcept {
				const bool old = active();
				f &= static_cast<u8>(~ActiveMask);
				return old;
			}

			// goal:		[1, 7]		3 bits, with value 0 unused
			// remaining:	[0, 7]		3 bits
			// active:		bool		1 bit
			// roc:			bool		1 bit
			// 		Total 8 bits		(u8):	(acgg grrr)
			u8 f{};
		};
		static_assert(sizeof(GranterTimed) == 1);

		struct GranterID {
			enum Totals : u32 {
				BigOffset = 0,
				MedOffset = GranterBig::Total,
				SmallOffset = MedOffset + GranterMed::Total,
				TinyOffset = SmallOffset + GranterSmall::Total,
				TimedOffset = TinyOffset + GranterTiny::Total,
				Big = GranterBig::Total,
				Med = GranterMed::Total,
				Small = GranterSmall::Total,
				Tiny = GranterTiny::Total,
				Timed = GranterTimed::Total,
			};
			enum : u32 {
				GetHit				= GranterBig::GetHit,
				KillDraugr			= GranterBig::KillDraugr,
				Dodge				= GranterMed::Dodge + static_cast<u32>(Totals::MedOffset),
				KillDaedra			= GranterMed::KillDaedra + static_cast<u32>(Totals::MedOffset),
				MakeOthersFear		= GranterSmall::MakeOthersFear + static_cast<u32>(Totals::SmallOffset),
				AbsorbDragonSouls	= GranterSmall::AbsorbDragonSouls + static_cast<u32>(Totals::SmallOffset),
				KillDragonPriests	= GranterTiny::KillDragonPriests + static_cast<u32>(Totals::TinyOffset),
				StayUnarmored		= GranterTimed::StayUnarmored + static_cast<u32>(Totals::TimedOffset),
				StayDemagicked		= GranterTimed::StayDemagicked + static_cast<u32>(Totals::TimedOffset),
				StayBurdened		= GranterTimed::StayBurdened + static_cast<u32>(Totals::TimedOffset),

				Total
			};
			using IDType = saturating<u32, 0, GranterID::Total>;

			constexpr GranterID() noexcept = default;
			constexpr GranterID(const GranterID&) noexcept = default;
			constexpr GranterID(GranterID&&) noexcept = default;
			constexpr GranterID& operator=(const GranterID&) noexcept = default;
			constexpr GranterID& operator=(GranterID&&) noexcept = default;
			constexpr ~GranterID() noexcept = default;

			constexpr GranterID(const GranterBig::ID val) noexcept		: id{ (val == GranterBig::Total)	? GranterID::Total : val } {}
			constexpr GranterID(const GranterMed::ID val) noexcept		: id{ (val == GranterMed::Total)	? GranterID::Total : (static_cast<u32>(val) + Totals::MedOffset) } {}
			constexpr GranterID(const GranterSmall::ID val) noexcept	: id{ (val == GranterSmall::Total)	? GranterID::Total : (static_cast<u32>(val) + Totals::SmallOffset) } {}
			constexpr GranterID(const GranterTiny::ID val) noexcept		: id{ (val == GranterTiny::Total)	? GranterID::Total : (static_cast<u32>(val) + Totals::TinyOffset) } {}
			constexpr GranterID(const GranterTimed::ID val) noexcept	: id{ (val == GranterTimed::Total)	? GranterID::Total : (static_cast<u32>(val) + Totals::TimedOffset) } {}
			constexpr GranterID(const u32 val) noexcept				: id{ val } {}

			__forceinline constexpr bool IsBig() const noexcept		{ return id < Totals::Big; }
			__forceinline constexpr bool IsMed() const noexcept		{ return (id >= Totals::MedOffset)		bitand (id < (Totals::MedOffset + Totals::Med)); }
			__forceinline constexpr bool IsSmall() const noexcept	{ return (id >= Totals::SmallOffset)	bitand (id < (Totals::SmallOffset + Totals::Small)); }
			__forceinline constexpr bool IsTiny() const noexcept	{ return (id >= Totals::TinyOffset)		bitand (id < (Totals::TinyOffset + Totals::Tiny)); }
			__forceinline constexpr bool IsTimed() const noexcept	{ return (id >= Totals::TimedOffset)	bitand (id != GranterID::Total); }

			constexpr size_t ToIndex() const noexcept {
				if (IsBig()) { return Raw(); }
				if (IsMed()) { return static_cast<size_t>(Raw() - Totals::MedOffset); } // Cast cause intellisense complains about intermediate int potentially overflowing
				if (IsSmall()) { return static_cast<size_t>(Raw() - Totals::SmallOffset); }
				if (IsTiny()) { return static_cast<size_t>(Raw() - Totals::TinyOffset); }
				if (IsTimed()) { return static_cast<size_t>(Raw() - Totals::TimedOffset); }
				return GranterID::Total;
			}

			__forceinline constexpr operator u32() const noexcept { return id.get(); }
			__forceinline constexpr u32 Raw() const noexcept { return id.get(); }

			__forceinline constexpr GranterID& operator+=(const u32 rhs) noexcept { id += rhs; return *this; }
			__forceinline constexpr GranterID& operator-=(const u32 rhs) noexcept { id -= rhs; return *this; }
			__forceinline constexpr GranterID& operator*=(const u32 rhs) noexcept { id *= rhs; return *this; }
			__forceinline constexpr GranterID& operator/=(const u32 rhs) noexcept { id /= rhs; return *this; }
			__forceinline constexpr GranterID& operator++() noexcept { ++id; return *this; }
			__forceinline constexpr u32 operator++(int) noexcept { return id++; }
			__forceinline constexpr GranterID& operator--() noexcept { --id; return *this; }
			__forceinline constexpr u32 operator--(int) noexcept { return id--; }

		private:
			IDType id{ GranterID::Total };
		};
		static constexpr array<const char*, GranterID::Total> GranterNames {
			"Get Hit",
			"Kill Draugr",
			"Dodge",
			"Kill Daedra",
			"Make Others Fear",
			"Absorb Dragon Souls",
			"Kill Dragon Priests",
			"Stay Unarmored",
			"Stay Demagicked",
			"Stay Burdened",
		};
		static constexpr array<std::pair<const char*, const char*>, GranterID::Total> GranterOverviewTexts {
			std::pair<const char*, const char*>{ "You earn 1 relief after you get hit ", " times." },
			std::pair<const char*, const char*>{ "You earn 1 relief after you kill ", " Draugr." },
			std::pair<const char*, const char*>{ "You earn 1 relief after you Dodge ", " times." },
			std::pair<const char*, const char*>{ "You earn 1 relief after you kill ", " Daedra." },
			std::pair<const char*, const char*>{ "You earn 1 relief after you intimidate ", " humanoids." },
			std::pair<const char*, const char*>{ "You earn 1 relief after you absorb ", " Dragon Souls." },
			std::pair<const char*, const char*>{ "You earn 1 relief after you kill ", " Dragon Priests." },
			std::pair<const char*, const char*>{ "You earn 1 relief after the day changes (12am) ", " times in a row while you remain unarmored." },
			std::pair<const char*, const char*>{ "You earn 1 relief after the day changes (12am) ", " times in a row while you remain without magic." },
			std::pair<const char*, const char*>{ "You earn 1 relief after the day changes (12am) ", " times in a row while you remain burdened." },
		};
		static constexpr array<const char*, 2> GranterOverviewRoCTexts {
			"This rule resets after it grants its reward.",
			"This rule will be removed after it grants its reward."
		};
		struct GranterMaker {
			__forceinline constexpr GranterID ID() const noexcept { return id; }
			__forceinline constexpr u8 Difficulty() const noexcept { return difficulty; }
			__forceinline constexpr u16 Goal() const noexcept { return GoalMult[goal_index] * difficulty; }
			__forceinline constexpr bool RoC() const noexcept { return remove_on_completion; }
			__forceinline constexpr void SetID(const GranterID new_id) noexcept { id = new_id; SetGoalIndex(); accepts_params = false; }
			__forceinline constexpr void SetDifficulty(const u8 new_difficulty) noexcept { difficulty = new_difficulty; }
			__forceinline constexpr void SetRoC(const bool new_roc) noexcept { remove_on_completion = new_roc; }

			constexpr string Overview(const bool show_price = false) {
				string res{};
				if (id != GranterID::Total) {
					res += GranterOverviewTexts[id].first;
					res += to_string(Goal());
					res += GranterOverviewTexts[id].second;
					res += ("\n\n"s + GranterOverviewRoCTexts[remove_on_completion]);
					if (show_price) {
						res += "\n\nThis rule will cost " + to_string(Cost()) + " gold to add.";
					}
				}
				return res;
			}
			constexpr i32 Cost() {
				i32 res = 200'000;
				res -= (2000 * difficulty * (difficulty != 1));
				return res;
			}
			constexpr lazy_vector<slider_params> GetParams() const noexcept {
				lazy_vector<slider_params> res{};
				if ((id != GranterID::Total) and res.reserve(2)) {
					accepts_params = true;
					res.append(slider_params::values{
						.text = ParamText[id],
						// .info = "",
							   .min = GoalMultF[goal_index],
							   .max = GoalMaxF[goal_index],
							   .start = static_cast<float>(difficulty),
							   .step = GoalMultF[goal_index],
							   .decimals = 0 });
					res.append(slider_params::values{
						.text = "Remove on Task Completion",
						.info = "The rule will be erased when its task is completed if =1",
						.min = 0.0f, .max = 1.0f, .start = static_cast<float>(remove_on_completion), .step = 1.0f, .decimals = 0 });
				}
				return res;
			}
			constexpr void SetParams(lazy_vector<float> params) noexcept {
				if (accepts_params and (params.size() == 2)) {
					difficulty = static_cast<u8>(static_cast<u16>(params[0]) / GoalMult[goal_index]);
					remove_on_completion = static_cast<bool>(params[1]);
					// accepts_params = false; // Only needs to change if id changes so w/e
				}
			}

		private:
			GranterID id{ GranterID::Total };
			u8 difficulty{ 1 };
			bool remove_on_completion{ false };
			u8 goal_index{ 0 };
			mutable bool accepts_params{ false }; // Probably not needed if caller isn't monkey, but difficulty could overflow if GetParams() gave 1000 max and then SetID() set id to 0-1 before SetParams().

			constexpr void SetGoalIndex() noexcept {
				if (id.IsBig()) {
					goal_index = 2;
				}
				else if (id.IsMed()) {
					goal_index = 1;
				}
				else {
					goal_index = 0;
				}
			}
			static constexpr array<u16, 3> GoalMult{ 1, 2, 10 };
			static constexpr array<u16, 3> GoalMax{ GoalMult[0] * 100, GoalMult[1] * 100, GoalMult[2] * 100 };
			static constexpr array<float, 3> GoalMultF{ static_cast<float>(GoalMult[0]), static_cast<float>(GoalMult[1]), static_cast<float>(GoalMult[2]) };
			static constexpr array<float, 3> GoalMaxF{ static_cast<float>(GoalMax[0]), static_cast<float>(GoalMax[1]), static_cast<float>(GoalMax[2]) };
			static constexpr array<const char*, GranterID::Total> ParamText {
				"Times to get hit",
				"Amount of Draugr to kill",
				"Times to Dodge",
				"Amount of Daedra to kill",
				"Amount of people to intimidate",
				"Dragon Souls to absorb",
				"Amount of Dragon Priests to kill",
				"Consecutive day changes to pass while unarmored",
				"Consecutive day changes to pass while demagicked",
				"Consecutive day changes to pass while burdened",
			};
		};
		static_assert(sizeof(GranterMaker) == 8);
		using GranterBigs = array<GranterBig, GranterBig::Total>;
		using GranterMeds = array<GranterMed, GranterMed::Total>;
		using GranterSmalls = array<GranterSmall, GranterSmall::Total>;
		using GranterTinys = array<GranterTiny, GranterTiny::Total>;
		using GranterTimeds = array<GranterTimed, GranterTimed::Total>;

	}
	using namespace Granters;

	struct rules_info {

		// Base interface
		string RulesOverview() const {
			string ret{ "Earned reliefs: " + to_string(earned_reliefs.get()) };
			/*unimplemented, maybe uneeded*/
			ret += "/" + to_string(6969);
			return ret;
		}
		string RulesBreakdown() const {
			string ret{ "Periodic Costers active: \n" };
			/*unimplemented, maybe unneeded*/
			return ret;
		}

		constexpr bool CanRelax() const noexcept { return earned_reliefs > 0; }
		constexpr sati32 EarnedReliefs() const noexcept { return earned_reliefs; }
		constexpr sat01flt Exp() const noexcept { return hardened_exp; }
		constexpr void AddReliefs(const i32 amount) noexcept {
			const u32 masked = std::bit_cast<u32>(amount) & bool_extend<u32>((can_earn_reliefs) bitor (amount < 0)); // Allow if can earn, or amount negative. Zero otherwise.
			earned_reliefs += std::bit_cast<i32>(masked);
		}
		constexpr void AddReliefs(const i32 amount, const EquipState& equips, const bool follower) noexcept {
			AddReliefs(amount);
			if (const i32 earned = earned_reliefs.get(); earned < 0) {
				const u32 remaining = AddPunishes(static_cast<u32>(-earned), equips, follower);
				earned_reliefs = -static_cast<i32>(remaining);
			}
		}
		constexpr void SetReliefs(const i32 amount) noexcept { earned_reliefs = amount; }


		void Update(const EquipState& equips, const float day_delta, const float buildup, const bool follower)  noexcept {
			if (day_delta <= 0.0f) {
				Log::Info("Data::rules_info::Update(): Bad update, delta <{}> is under 0!"sv, day_delta);
				return;
			}

			hardened_exp += buildup;

			update_days_sum += day_delta;
			const u32 days_passed = update_days_sum.u32();
			update_days_sum -= static_cast<float>(days_passed); // Keep fractional only
			const float hour_delta = (day_delta * 24.0f);

			i32 earned = 0;
			// Progress timeds always, even if can't earn, because I want it punishing. 
			for (auto& r : granter_timeds) {
				earned += r.advance(days_passed);
			}
			// Zero whatever timeds may have earned if can't earn, effectively wasting it.
			earned = static_cast<i32>(static_cast<u32>(std::bit_cast<u32>(earned) & bool_extend<u32>(can_earn_reliefs)));

			earned -= cost_eq.update(hour_delta);
			earned -= cost_eq_tr.update(hour_delta);
			prev_sl.update(days_passed != 0);
			prev_dg.update(days_passed != 0);

			misc_pun.set_havefollower_obeying(follower);

			AddReliefs(earned, equips, follower);
			UpdateCanEarn(); // This at the end. Timed progress should be calculated under whatever can_earn_reliefs was before the update.
		}

		// Event triggers
		constexpr void Rested() noexcept {
			AddReliefs(-1);
			since_boost = 0;
			if (streak > best_streak) {
				best_streak = streak;
			}
			streak = 0;
		}
		
		constexpr void Dodged() noexcept {
			hardened_exp += 0.00005f; // 1.0f / 20'000.0f
			prev_dg.dodged();
			AdvanceGranter(GranterMed::Dodge, 1);
			AddRecent();
			++total;
			++streak;
			++since_boost;
			if (streak > best_streak) {
				best_streak = streak;
			}
		}

		constexpr void ArmorEquipped(const ArmorEquippedFlags aef) noexcept {
			i32 result = 0;

			// A basic slot (head chest hands feet) must be covered  and  the armor type must be set
			if (aef.slots.any_basic() bitand aef.flags.worn()) { // worn() returns true only if type is not 0
				// Send it to tiereds
				result -= cost_eq_tr.armorequipped(aef.flags);
				prev_eq_tr.armorequipped(aef.flags);

				// If feet, send to nosoles
				if (aef.slots.feet()) {
					result -= cost_eq.feetequipped(aef.flags);
					prev_eq.feetequipped(aef.flags);
				}
			}

			// Handle Hurdles, regardless of basic slot coverage or armor type
			if (aef.hr.any()) {
				prev_eq.hrequipped(aef.hr);
			}

			AddReliefs(result);
		}
		constexpr void ArmorUnequipped(const EquipState& newstate) noexcept {
			const AnalyzedBasicSlots flagpack{ newstate };
			// Call all tiereds
			cost_eq_tr.armorunequipped(flagpack);
			prev_eq_tr.armorunequipped(flagpack);
			// Call all nosoles
			cost_eq.armorunequipped(flagpack);
			prev_eq.armorunequipped(flagpack, newstate.hr); // and HR
		}
		constexpr void HandEquipped(const HandEquippedFlags hef) noexcept {
			if (!hef.empty()) {
				AddReliefs(-cost_eq.handequipped(hef));
			}
		}
		constexpr void HandUnequipped(const HandsFlags newhands) noexcept { cost_eq.handunequipped(newhands); }

		constexpr void FastTravelEnd() noexcept { AddReliefs(-misc_pun.nofasttravel_penalty()); }

		constexpr void Brawled(const array<Vanilla::RCE, 8>& races) noexcept {
			for (const auto& race : races) {
				switch (race) {
				case (Vanilla::RCE::Nord): { prev_sl.brawled(PreventersSL::BrawlNord); break; }
				case (Vanilla::RCE::Imperial): { prev_sl.brawled(PreventersSL::BrawlImperial); break; }
				case (Vanilla::RCE::Breton): { prev_sl.brawled(PreventersSL::BrawlBreton); break; }
				case (Vanilla::RCE::Redguard): { prev_sl.brawled(PreventersSL::BrawlRedguard); break; }
				case (Vanilla::RCE::Orc): { prev_sl.brawled(PreventersSL::BrawlOrsimer); break; }
				case (Vanilla::RCE::HighElf): { prev_sl.brawled(PreventersSL::BrawlAltmer); break; }
				case (Vanilla::RCE::DarkElf): { prev_sl.brawled(PreventersSL::BrawlDunmer); break; }
				case (Vanilla::RCE::WoodElf): { prev_sl.brawled(PreventersSL::BrawlBosmer); break; }
				case (Vanilla::RCE::Khajiit): { prev_sl.brawled(PreventersSL::BrawlKhajit); break; }
				case (Vanilla::RCE::Argonian): { prev_sl.brawled(PreventersSL::BrawlArgonian); break; }
				}
			}
		}
		constexpr void EnteredUnfairBrawl() noexcept { prev_sl.unfairbrawl(); }
		constexpr void EnteredBrawl(const lazy_vector<string>& tags) noexcept {
			bool found_raw = false;
			bool found_angry = false;
			bool found_merry = false;
			for (const auto& tag : tags) {
				if (!found_raw and StrICmp(tag.c_str(), "raw"))		{ prev_sl.rawbrawl(); found_raw = true; }
				if (!found_angry and StrICmp(tag.c_str(), "angry"))	{ prev_sl.angrybrawl(); found_angry = true; }
				if (!found_merry and StrICmp(tag.c_str(), "merry"))	{ prev_sl.merrybrawl(); found_merry = true; }
			}
		}

		// Granter interface
		constexpr u32 NumGranters() const noexcept {
			u32 count = 0;
			ApplyToAllGrantersC([&](const auto& r) { count += r.active(); });
			return count;
		}
		constexpr bool HasGranters() const noexcept { return NumGranters() != 0; }
		constexpr bool CanAddGranter() const noexcept { return NumGranters() < GranterID::Total; }
		constexpr bool AddGranter(const GranterMaker maker) noexcept { return ApplyToGranter(maker.ID(), [&](auto& r) -> bool { return r.init(maker.Difficulty(), maker.RoC()); }); }
		constexpr bool RemoveGranter(const GranterID gid) noexcept { return ApplyToGranter(gid, [](auto& r) -> bool { return r.set_inactive(); }); }
		constexpr void AdvanceGranter(const GranterID gid, const u32 amount) noexcept {
			i32 earned = 0;
			ApplyToGranter(gid, [&](auto& r) {
				earned = r.advance(static_cast<u16>(amount));
				return true;
			});
			AddReliefs(earned);
		}
		constexpr lazy_vector<GranterID> GetGranterIDs(const bool actives) const {
			lazy_vector<GranterID> res{};
			res.reserve(NumGranters());
			for (u32 i = 0; i < granter_bigs.size(); ++i) {
				if ((granter_bigs[i].active() == actives) and !res.try_append(i)) {
					return {};
				}
			}
			for (u32 i = 0; i < granter_meds.size(); ++i) {
				if ((granter_meds[i].active() == actives) and !res.try_append(static_cast<GranterMed::ID>(i))) {
					return {};
				}
			}
			for (u32 i = 0; i < granter_smalls.size(); ++i) {
				if ((granter_smalls[i].active() == actives) and !res.try_append(static_cast<GranterSmall::ID>(i))) {
					return {};
				}
			}
			for (u32 i = 0; i < granter_tinies.size(); ++i) {
				if ((granter_tinies[i].active() == actives) and !res.try_append(static_cast<GranterTiny::ID>(i))) {
					return {};
				}
			}
			for (u32 i = 0; i < granter_timeds.size(); ++i) {
				if ((granter_timeds[i].active() == actives) and !res.try_append(static_cast<GranterTimed::ID>(i))) {
					return {};
				}
			}
			return res;
		}
		static constexpr lazy_vector<string> GranterStaticNames(const lazy_vector<GranterID>& ids) {
			lazy_vector<string> res{};
			if (res.reserve(ids.size())) {
				for (const auto& id : ids) {
					res.append(GranterNames[id]);
				}
			}
			return res;
		}
		static constexpr string GranterStaticName(const GranterID id) { return GranterNames[id]; }
		constexpr string Overview(const GranterID id, const u64 indents = 0) const {
			string res(indents, '\t');
			struct granter_vals {
				bool active;
				bool roc;
				u16 goal;
				u16 remaining;
				u16 progress;
			};
			granter_vals vals{};
			auto func = [&] (const auto& r) {
				vals.active = r.active();
				vals.roc = r.remove_on_completion();
				vals.goal = r.goal();
				vals.remaining = r.remaining();
				vals.progress = r.progress();
				return true;
			};
			if (ApplyToGranterC(id, func)) {
				if (vals.active) {
					res += GranterOverviewTexts[id].first;
					res += to_string(vals.remaining);
					res += GranterOverviewTexts[id].second;
					if (!vals.roc or id.IsTimed()) {
						res += " (" + to_string(vals.progress) + "/" + to_string(vals.goal) + ")";
					}
					if (vals.roc) {
						res += " Removed when completed.";
					}
				} else {
					res += "Rule <"s + GranterNames[id] + "> is not active!";
				}
			}
			return res;
		}
		constexpr string GetGranterStrBase(const u64 indents) const {
			const string indent(indents, '\t');
			if (not HasGranters()) {
				return indent + string{ "No rules are active" };
			}

			string result{};
			for (GranterID id = 0; id < GranterID::Total; ++id) {
				result += Overview(id, indents) + '\n';
			}

			while (result.back() == '\n') { // It should be exactly 1 here, but w/e keep the loop.
				result.pop_back();
			}

			return result;
		}


		// Records
		__forceinline constexpr array<float, 3> Boosts() const { return array<float, 3>{ mrate, srate, speed }; }

		__forceinline constexpr void AddBoosts(const array<float, 3>& extra) noexcept {
			mrate += extra[0];
			srate += extra[1];
			speed += extra[2];
		}
		__forceinline constexpr void ClearBoosts() noexcept {
			mrate = 0.0f;
			srate = 0.0f;
			speed = 0.0f;
		}
		
		__forceinline constexpr satu32 Total() const noexcept { return total; }
		__forceinline constexpr satu32 Streak() const noexcept { return streak; }
		__forceinline constexpr satu32 BestStreak() const noexcept { return best_streak; }
		__forceinline constexpr satu32 SinceBoost() const noexcept { return since_boost; }

		__forceinline constexpr satu32 Total(const u32 num) noexcept { return (total = num); }
		__forceinline constexpr satu32 Streak(const u32 num) noexcept {
			if ((streak = num) > best_streak) {
				best_streak = streak;
			}
			return streak;
		}
		__forceinline constexpr satu32 BestStreak(const u32 num) noexcept { return (best_streak = num); }
		__forceinline constexpr satu32 SinceBoost(const u32 num) noexcept { return (since_boost = num); }

		constexpr void TickRecents(const float sec) noexcept {
			// Non-ranged for loop consistently generates the best code on MSVC, at worst tied with something else. Both indexed and iterated are equally fast, with minor asm differences.
			// std::for_each_n() and hardcoded loop-less do well at low n. std::for_each() does well at high n.
			for (size_t i = 0; i < recent_dodges.size(); ++i) {
				recent_dodges[i] -= sec;
			}
		}
		constexpr u32 RecentsCount() const noexcept {
			 // Non-ranged for loop wins here too, over count_if(). Also slightly better than doing "if > 0, ++res". Compiler smart.
			 u32 res = 0;
			 for (size_t i = 0; i < recent_dodges.size(); ++i) {
				 res += (recent_dodges[i] > 0.0f);
			 }
			 return res;
		 }
		constexpr u32 ClearRecents() noexcept {
			// Non-ranged for loop wins here too, over fill(), fill_n(), and recent_dodges = array<...>{}
			const u32 count = RecentsCount();
			for (size_t i = 0; i < recent_dodges.size(); ++i) {
				recent_dodges[i] = 0.0f;
			}
			return count;
		}

		string RecordsString() const {
			string result{ "Total times Dodged:\t\t" + total.str() };
			result += "\nDodges Streak:\t\t\t" + streak.str();
			result += "\nBest Streak:\t\t\t" + best_streak.str();
			result += "\nDodges since last Boost:\t" + since_boost.str();
			return result;
		}


		void Swap(rules_info& other) noexcept {
			enum : size_t { swapsize = sizeof(rules_info) };
			static_assert(std::is_trivially_copyable_v<rules_info> and swapsize == 128);
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


	private:
		sati32 earned_reliefs{};			// 4	u
		sat0flt update_days_sum{};			// 4	u
		sat01flt hardened_exp{};			// 4	u	= 12

		CostersEquip cost_eq{};				// 16	u
		CostersEquipTiered cost_eq_tr{};	// 16	u
		PreventersEquip prev_eq{};			// 4
		PreventersSL prev_sl{};				// 4	u
		PreventersEquipTiered prev_eq_tr{};	// 3
		PreventerDodge prev_dg{};			// 2	u
		MiscPunishers misc_pun;				// 1	u	+ 46 = 58

		bool can_earn_reliefs{};			// 1	u	

		GranterTimeds granter_timeds{};		// 3	u	+ 1 + 3 = 62	All 55 used in Update() in first cacheline (marked with "u")

		GranterTinys granter_tinies{};		// 1
		char pad;							// 1	+ 1 + 1 = 64

		// Cacheline cutoff. Could put these 2 at the end so that the one Big would be in the first cacheline, but this is simpler.

		GranterBigs granter_bigs{};			// 4	
		GranterMeds granter_meds{};			// 4
		GranterSmalls granter_smalls{};		// 4	+ 12 = 76

		array<float, 6> recent_dodges{};	// 24	+ 24 = 100
		sat0flt mrate{};					// 4
		sat0flt srate{};					// 4
		sat0flt speed{};					// 4
		satu32 total{};						// 4
		satu32 streak{};					// 4
		satu32 best_streak{};				// 4
		satu32 since_boost{};				// 4	+ 28 = 128


		constexpr void UpdateCanEarn() noexcept { can_earn_reliefs = (prev_eq.allowed() bitor prev_eq_tr.allowed() bitor prev_sl.allowed() bitor prev_dg.allowed() bitor misc_pun.allowed()); }

		u32 AddPunishes(u32 add_count, const EquipState& equips, const bool follower) noexcept {
			enum Category : u8 {
				Unavailable = 0,

				Coster,
				CosterTiered,
				Preventer,
				PreventerTiered,
				PreventerDodge,
				PreventerSL,
				Misc,

				TotalPunishers = Misc,
			};
			
			// Another excercise in making this branchless. Most if-checks would have to be made in the average case, so performance-wise it's worth it. Still looks ugly, though.
			
			array<Category, Category::TotalPunishers> availables{};
			auto initialize_get_categories_cound = [this, &availables] () {
				u32 idx = 0;
				availables[idx] = static_cast<Category>(Coster & bool_extend(cost_eq.any_inactive()));
				availables[idx += (availables[idx] != Unavailable)] = static_cast<Category>(CosterTiered & bool_extend(cost_eq_tr.any_inactive()));
				availables[idx += (availables[idx] != Unavailable)] = static_cast<Category>(Preventer & bool_extend(prev_eq.any_inactive()));
				availables[idx += (availables[idx] != Unavailable)] = static_cast<Category>(PreventerTiered & bool_extend(prev_eq_tr.any_inactive()));
				availables[idx += (availables[idx] != Unavailable)] = static_cast<Category>(PreventerDodge & bool_extend(not prev_dg.active()));
				availables[idx += (availables[idx] != Unavailable)] = static_cast<Category>(PreventerSL & bool_extend(prev_sl.any_inactive()));
				availables[idx += (availables[idx] != Unavailable)] = static_cast<Category>(Misc & bool_extend(misc_pun.any_inactive()));
				return static_cast<u32>(idx + (availables[idx] != Unavailable));
			};

			if (const u32 initial_categories_count = initialize_get_categories_cound(); initial_categories_count != 0) {
				static RNG::gamerand gen{ RNG::random_state{} };
				const AnalyzedBasicSlots flagpack{ equips };
				for (u32 categories_count = initial_categories_count; (categories_count != 0) bitand (add_count != 0); --add_count) { // Decrement add_count for each added
					bool filled = false;
					const u32 idx = gen.next(categories_count);
					switch (availables[idx]) {
					case Unavailable:		{ break; } // Should never get here
					case Coster:			{ filled = cost_eq.activate_random_check_full(gen, flagpack, equips.hands); break; }
					case CosterTiered:		{ filled = cost_eq_tr.activate_random_check_full(gen, flagpack); break; }
					case Preventer:			{ filled = prev_eq.activate_random_check_full(gen, flagpack, equips.hr); break; }
					case PreventerTiered:	{ filled = prev_eq_tr.activate_random_check_full(gen, flagpack); break; }
					case PreventerDodge:	{ filled = prev_dg.activate_check_full(); break; }
					case PreventerSL:		{ filled = prev_sl.activate_random_check_full(gen); break; }
					case Misc:				{ filled = misc_pun.activate_random_check_full(gen, follower); break; }
					default:				{ __assume(0); }
					}
					const u32 fill_mask = bool_extend<u32>(filled);
					const u32 not_fill_mask = static_cast<u32>(~fill_mask);
					categories_count -= filled; // If decreased, it becomes the index of the last available category
					availables[idx] = availables[static_cast<u32>((categories_count & fill_mask) | (idx & not_fill_mask))]; // Replace current with last available if filled, or self if not filled.
				}
			}

			return add_count; // Return how many could not be added
		}

		constexpr void AddRecent() noexcept {
			// Both non-ranged for loop and hardcoded assignments generate the same. Using std::memmove() calls it. Interestingly, GCC transforms the loop version into a std::memmove() call.
			for (size_t i = recent_dodges.size() - 1; i != 0; --i) {
				recent_dodges[i] = recent_dodges[i - 1];
			}
			recent_dodges[0] = 30.0f; // Kick em all back by 1, and place the new and biggest one at the start
		}

		template<typename F>
		constexpr bool ApplyToGranterC(const GranterID id, F func) const {
			if (id.IsBig())		{ return func(granter_bigs[id]); }
			if (id.IsMed())		{ return func(granter_meds[id.ToIndex()]); }
			if (id.IsSmall())	{ return func(granter_smalls[id.ToIndex()]); }
			if (id.IsTiny())	{ return func(granter_tinies[id.ToIndex()]); }
			if (id.IsTimed())	{ return func(granter_timeds[id.ToIndex()]); }
			return false;
		}
		template<typename F>
		constexpr bool ApplyToGranter(const GranterID id, F func) {
			if (id.IsBig())		{ return func(granter_bigs[id]); }
			if (id.IsMed())		{ return func(granter_meds[id.ToIndex()]); }
			if (id.IsSmall())	{ return func(granter_smalls[id.ToIndex()]); }
			if (id.IsTiny())	{ return func(granter_tinies[id.ToIndex()]); }
			if (id.IsTimed())	{ return func(granter_timeds[id.ToIndex()]); }
			return false;
		}
		template<typename F>
		constexpr void ApplyToAllGrantersC(F func) const {
			for (const auto& r : granter_bigs) { func(r); }
			for (const auto& r : granter_meds) { func(r); }
			for (const auto& r : granter_smalls) { func(r); }
			for (const auto& r : granter_tinies) { func(r); }
			for (const auto& r : granter_timeds) { func(r); }
		}
		template<typename F>
		constexpr void ApplyToAllGranters(F func) {
			for (auto& r : granter_bigs) { func(r); }
			for (auto& r : granter_meds) { func(r); }
			for (auto& r : granter_smalls) { func(r); }
			for (auto& r : granter_tinies) { func(r); }
			for (auto& r : granter_timeds) { func(r); }
		}
		template<typename F>
		constexpr bool TryApplyToAllGrantersC(F func) const {
			for (const auto& r : granter_bigs) { if (not func(r)) { return false; } }
			for (const auto& r : granter_meds) { if (not func(r)) { return false; } }
			for (const auto& r : granter_smalls) { if (not func(r)) { return false; } }
			for (const auto& r : granter_tinies) { if (not func(r)) { return false; } }
			for (const auto& r : granter_timeds) { if (not func(r)) { return false; } }
			return true;
		}

	};
	static_assert(std::is_trivially_copyable_v<rules_info> and sizeof(rules_info) == 128);






}
