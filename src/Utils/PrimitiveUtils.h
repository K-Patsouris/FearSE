#pragma once
#include "Common.h"


namespace PrimitiveUtils {

	string addr_str(void* ptr);
	string bit_str(void* ptr, u64 byteNum);
	string bool_str(bool flag);

	string u32_str(u32 num);
	string u32_xstr(const u32 num, const u64 length = 8, const char pad = '0');

	template<typename T> requires (std::unsigned_integral<T>)
	constexpr bool has_single_bit(const T num) noexcept {
		const T x1 = num - 1;	// This unsets the lowest set bit and sets all the ones lower than it. eg.	1100->1011,		0001->0000,		1000->0111,			0000->1111 (underflow)
		const T x2 = num ^ x1;	// This keeps the bit unset before, the bits set before, and no else. eg.	1100^1011=0111,	0001^0000=0001,	1000->0111=1111,	0000^1111=1111
		return x1 < x2;			// If power of 2, the ^ result will always be greater than the -1 result because bits are never lost. 0 won't pass because it won't provide bits to ^ to break the ==.
	}

	// Returns std::numeric_limits<T>::max() (all bits 1) if flag == true, and 0 otherwise
	template<typename T = u8> requires (std::unsigned_integral<T>)
	__forceinline constexpr T bool_extend(const bool flag) noexcept {
		// Could use flag as index to static array, or unsigned shift LSB to MSB then signed shift MSB to LSB to sign-extend, or just cast and negate
		// Array is by far the slowest. Shifts is 3 instructions (movzx, shr, sal). Negation is 2 (movzx, neg).
		return 0 - static_cast<T>(flag); // Defined underflow. 1 (true) becomes 255 (1 under 0), 0 (false) stays 0
	}

	// Returns val if flag == true, and 32 unset bits otherwise (= 0.0f if IEEE754, which MSVC guarantees).
	[[nodiscard]] __forceinline constexpr float mask_float(const float val, const bool flag) noexcept {
		return std::bit_cast<float>(std::bit_cast<u32>(val) & bool_extend<u32>(flag));
	}
	

	// Number to number functions perform clamping where necessary to avoid overflow/underflow. eg S23ToS8 with input 200 will return 127 which is the i8 max.
	constexpr i64		to_s64(u64 num)	noexcept { return static_cast<i64>	(std::clamp<u64>(num, 0u, LLONG_MAX)); }
	constexpr u32	to_u32(u64 num)	noexcept { return static_cast<u32>	(std::clamp<u64>(num, 0u, UINT_MAX)); }
	constexpr i32		to_s32(u64 num)	noexcept { return static_cast<i32>	(std::clamp<u64>(num, 0u, INT_MAX)); }
	constexpr u8		to_u8(u64 num)	noexcept { return static_cast<u8>	(std::clamp<u64>(num, 0u, UCHAR_MAX)); }
	constexpr i8		to_s8(u64 num)	noexcept { return static_cast<i8>		(std::clamp<u64>(num, 0u, SCHAR_MAX)); }

	constexpr u64	to_u64(i64 num)	noexcept { return static_cast<u64>	(std::clamp<i64>(num, 0, LLONG_MAX)); }
	constexpr u32	to_u32(i64 num)	noexcept { return static_cast<u32>	(std::clamp<i64>(num, 0, UINT_MAX)); }
	constexpr i32		to_s32(i64 num)	noexcept { return static_cast<i32>	(std::clamp<i64>(num, INT_MIN, INT_MAX)); }
	constexpr u8		to_u8(i64 num)	noexcept { return static_cast<u8>	(std::clamp<i64>(num, 0, UCHAR_MAX)); }
	constexpr i8		to_s8(i64 num)	noexcept { return static_cast<i8>		(std::clamp<i64>(num, SCHAR_MIN, SCHAR_MAX)); }

	constexpr i32		to_s32(u32 num)	noexcept { return static_cast<i32>	(std::clamp<u32>(num, 0u, INT_MAX)); }
	constexpr u8		to_u8(u32 num)	noexcept { return static_cast<u8>	(std::clamp<u32>(num, 0u, UCHAR_MAX)); }
	constexpr i8		to_s8(u32 num)	noexcept { return static_cast<i8>		(std::clamp<u32>(num, 0u, SCHAR_MAX)); }

	constexpr u64	to_u64(i32 num)	noexcept { return static_cast<u64>	(std::clamp<i32>(num, 0, INT_MAX)); }
	constexpr u32	to_u32(i32 num)	noexcept { return static_cast<u32>	(std::clamp<i32>(num, 0, INT_MAX)); }
	constexpr u8		to_u8(i32 num)	noexcept { return static_cast<u8>	(std::clamp<i32>(num, 0, UCHAR_MAX)); }
	constexpr i8		to_s8(i32 num)	noexcept { return static_cast<i8>		(std::clamp<i32>(num, SCHAR_MIN, SCHAR_MAX)); }

	constexpr i8		to_s8(u8 num)	noexcept { return static_cast<i8>		(std::clamp<u8>(num, 0u, SCHAR_MAX)); }

	constexpr u64	to_u64(i8 num)	noexcept { return static_cast<u64>	(std::clamp<i8>(num, 0, SCHAR_MAX)); }
	constexpr u32	to_u32(i8 num)	noexcept { return static_cast<u32>	(std::clamp<i8>(num, 0, SCHAR_MAX)); }
	constexpr u8		to_u8(i8 num)		noexcept { return static_cast<u8>	(std::clamp<i8>(num, 0, SCHAR_MAX)); }

	constexpr u64	to_u64(float num)	noexcept { return static_cast<u64>	(std::clamp<float>(num, 0.0f, static_cast<float>(ULLONG_MAX))); }
	constexpr i64		to_s64(float num)	noexcept { return static_cast<i64>	(std::clamp<float>(num, static_cast<float>(LLONG_MIN), static_cast<float>(LLONG_MAX))); }
	constexpr u32	to_u32(float num)	noexcept { return static_cast<u32>	(std::clamp<float>(num, 0.0f, static_cast<float>(UINT_MAX))); }
	constexpr i32		to_s32(float num)	noexcept { return static_cast<i32>	(std::clamp<float>(num, static_cast<float>(INT_MIN), static_cast<float>(INT_MAX))); }
	constexpr u8		to_u8(float num)	noexcept { return static_cast<u8>	(std::clamp<float>(num, 0.0f, static_cast<float>(UCHAR_MAX))); }
	constexpr i8		to_s8(float num)	noexcept { return static_cast<i8>		(std::clamp<float>(num, static_cast<float>(SCHAR_MIN), static_cast<float>(SCHAR_MAX))); }


}
