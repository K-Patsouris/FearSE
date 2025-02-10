#pragma once
#include "Common.h"

namespace FlagTypes {


	struct double_flag {
	public:
		constexpr double_flag() noexcept = default;
		constexpr double_flag(const double_flag&) noexcept = default;
		constexpr double_flag(double_flag&&) noexcept = default;
		constexpr double_flag& operator=(const double_flag&) noexcept = default;
		constexpr double_flag& operator=(double_flag&&) noexcept = default;
		constexpr ~double_flag() noexcept = default;
		constexpr double_flag(bool first_, bool last_) noexcept : f(static_cast<u8>(first_ << 7) | static_cast<u8>(last_)) {}

		constexpr bool first() const noexcept { return (f & FIRST); }
		constexpr bool last() const noexcept { return (f & LAST); }

		constexpr void first(const bool val) noexcept {
			f <<= 1;
			f >>= 1;
			f |= static_cast<u8>(val << 7);
		}
		constexpr void last(const bool val) noexcept {
			f >>= 1;
			f <<= 1;
			f |= static_cast<u8>(val);
		}

		constexpr bool both() const noexcept { return f == BOTH; }


		constexpr u8 GetRaw() const noexcept { return f; }

	private:
		static constexpr u8 FIRST = 0b1000'0000ui8;
		static constexpr u8 LAST = 0b0000'0001ui8;
		static constexpr u8 BOTH = FIRST | LAST;

		u8 f;
	};
	static_assert(std::is_trivial_v<double_flag> and sizeof(double_flag) == 1);

	// bitset interface of size T::Total, which must exist and be the single largest enumerator of T. T enumerators with the same value will not be differentiated.
	// Similar to stl::enumeration but can hold any amount of flags, instead of only powers of 2 up to 2^6 (64).
	template<typename T> requires ( std::is_enum_v<T> )
	struct bitflags {
	public:
		bitflags() noexcept : flags() {}
		bitflags(const bitflags& init) : flags(init.flags.to_string()) {}
		bitflags(const std::bitset<static_cast<u64>(T::Total)>& init) : flags(init.to_string()) {}

		bitflags& operator=(const bitflags& rhs) { this->flags = rhs.flags.to_string(); return *this; }

		~bitflags() = default;


		template<typename... Ts>
		requires(std::same_as<Ts, T> and ...)
		bool set(T first, Ts... args) noexcept {
			try {
				flags.set(static_cast<u64>(first), true);
				(flags.set(static_cast<u64>(args), true), ...);
				return true;
			}
			catch (...) {
				return false;
			}
		}

		template<typename... Ts>
		requires(std::same_as<Ts, T> and ...)
		bool unset(T first, Ts... args) noexcept {
			try {
				flags.set(static_cast<u64>(first), false);
				(flags.set(static_cast<u64>(args), false), ...);
				return true;
			}
			catch (...) {
				return false;
			}
		}

		template<typename... Ts>
		requires(std::same_as<Ts, T> and ...)
		bool all_of(T first, Ts... args) const noexcept {
			try {
				bool res = flags.test(static_cast<u64>(first));
				((!res ? false : (res = flags.test(static_cast<u64>(args)), true)) and ...); // Stops when res is first found false and function returns false
				return res;
			}
			catch (...) {
				return false;
			}
		}

		template<typename... Ts>
		requires(std::same_as<Ts, T> and ...)
		bool any_of(T first, Ts... args) const noexcept {
			try {
				bool res = flags.test(static_cast<u64>(first));
				((res ? false : (res = flags.test(static_cast<u64>(args)), true)) and ...); // Stops when res is first found true and function returns true
				return res;
			}
			catch (...) {
				return false;
			}
		}

		template<typename... Ts>
		requires(std::same_as<Ts, T> and ...)
		bool none_of(T first, Ts... args) const noexcept {
			try {
				bool res = flags.test(static_cast<u64>(first));
				((res ? false : (res = flags.test(static_cast<u64>(args)), true)) and ...); // Stops when res is first found true and function returns false
				return !res;
			}
			catch (...) {
				return false;
			}
		}


		static consteval size_t size() noexcept { return flags.size(); }
		void reset() noexcept { flags.reset(); }
		string to_string() const { return flags.to_string(); }

	private:
		std::bitset<static_cast<size_t>(T::Total)> flags;
		// 42 in binary:		0	0	1	0	1	0	1	0
		// bit weight:		0	0	32	0	8	0	2	0
		// bitset indices:	7	6	5	4	3	2	1	0
	};


}
