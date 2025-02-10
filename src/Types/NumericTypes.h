#pragma once
// #include <intrin.h> // We only need the below, but keeping this here in case the below filename ever changes.
#include <intrin0.inl.h>
#pragma intrinsic(_umul128)

namespace NumericTypes {


	template<typename T, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max()> requires (
		(sizeof(T) <= sizeof(std::uint64_t)) // In case we ever get >64bit numbers that pass std::integral/std::floating_point.
		and (max > min)
		and (std::integral<T> or (std::floating_point<T> and (min >= std::numeric_limits<T>::lowest()) and (max <= std::numeric_limits<T>::max())))
	)
	struct saturating {
	private:
		using i64t = std::int64_t;
		using u64t = std::uint64_t;
		using i32t = std::int32_t;
		using u32t = std::uint32_t;
		using i16t = std::int16_t;
		using u16t = std::uint16_t;
		using i8t = std::int8_t;
		using u8t = std::uint8_t;

		static constexpr size_t bytes{ sizeof(T) };
		static constexpr bool is_integral{ std::integral<T> };
		static constexpr bool is_floating{ std::floating_point<T> };
		static constexpr bool is_signed{ std::signed_integral<T> };
		static constexpr bool under64{ bytes < sizeof(u64t) };

		// Floatings
		template <bool, size_t> struct helper_types { using sT = void; using uT = void; using swT = void; using uwT = void; };
		// Integrals
		template <> struct helper_types<true, 1> { using sT = i8t;	using uT = u8t;	using swT = i16t;	using uwT = u16t; };
		template <> struct helper_types<true, 2> { using sT = i16t;	using uT = u16t;using swT = i32t;	using uwT = u32t; };
		template <> struct helper_types<true, 4> { using sT = i32t;	using uT = u32t;using swT = i64t;	using uwT = u64t; };
		template <> struct helper_types<true, 8> { using sT = i64t;	using uT = u64t;using swT = void;	using uwT = void; };

		using sT = helper_types<is_integral, bytes>::sT;
		using uT = helper_types<is_integral, bytes>::uT;
		using swT = helper_types<is_integral, bytes>::swT;
		using uwT = helper_types<is_integral, bytes>::uwT;


		// Integrals
		template <typename U> struct helper_constants {
			static constexpr U shrcnt = static_cast<U>(sizeof(T) * 8 - 1); // Amount to shift right to signmask. SIGNED right shift extends the leftmost bit; instead of just prepending 0s like unsigned.
			static constexpr U tminabs = (U{ 1 } << shrcnt); // Absolute of minimum of T
			static constexpr U tmaxabs = tminabs - 1; // Absolute of maximum of T
			static constexpr U utmin = 0; // Zero of unsigned of bitlength of T
			static constexpr U utmax = std::numeric_limits<U>::max(); // Max of unsigned of bitlength of T
			static constexpr U minsgn = std::bit_cast<U>(static_cast<T>(min >> shrcnt)); // Signmask for min. All 1s if negative, all 0s otherwise.
			static constexpr U maxsgn = std::bit_cast<U>(static_cast<T>(max >> shrcnt)); // Signmask for max. All 1s if negative, all 0s otherwise.
			static constexpr U minabs = (std::bit_cast<U>(min) ^ minsgn) - minsgn; // Absolute of min
			static constexpr U maxabs = (std::bit_cast<U>(max) ^ maxsgn) - maxsgn; // Absolute of max
		};
		// Floatings
		template <> struct helper_constants<void> {};

		using constants = helper_constants<uT>;


		template <typename U>
		__forceinline static constexpr const U& mymin(const U& l, const U& r) noexcept { return l < r ? l : r; }
		template <typename U>
		__forceinline static constexpr const U& mymax(const U& l, const U& r) noexcept { return l < r ? r : l; }
		template <typename U>
		__forceinline static constexpr U& clamp_minmax(U& v) noexcept {
			v = mymax<U>(v, min);
			v = mymin<U>(v, max);
			return v;
		}
		__forceinline constexpr void clamp_self() noexcept requires(is_floating) {
			val = mymax(val, min);
			val = mymin(val, max);
		}

	public:
		constexpr saturating() noexcept = default;
		constexpr saturating() noexcept requires (min > 0) : val{ min } {}
		constexpr saturating(const saturating&) noexcept = default;
		constexpr saturating(saturating&&) noexcept = default;
		constexpr saturating& operator=(const saturating&) noexcept = default;
		constexpr saturating& operator=(saturating&&) noexcept = default;
		constexpr ~saturating() noexcept = default;
		constexpr saturating(T init) noexcept : val{ clamp_minmax(init) } {}

		constexpr operator T() const noexcept { return val; }

		constexpr T get() const noexcept { return val; }

		constexpr saturating& operator=(T rhs) noexcept {
			val = clamp_minmax(rhs);
			return *this;
		}

		constexpr saturating& operator+=(T rhs) noexcept {
			if constexpr (is_integral) {
				if constexpr (is_signed) { // clamp sucks ass. Ternary is minimal and branchless. If produces a branch.
					T tomin = -std::bit_cast<T>(std::min<uT>((std::bit_cast<uT>(val) - std::bit_cast<uT>(min)), constants::tminabs));
					T tomax = std::bit_cast<T>(std::min<uT>((std::bit_cast<uT>(max) - std::bit_cast<uT>(val)), constants::tmaxabs));
					(rhs < tomin) ? rhs = tomin : rhs; // Faster codegen than std::min or a=c?b:a for some reason
					(rhs > tomax) ? rhs = tomax : rhs;
					val += rhs;
				}
				else {
					val += mymin<T>(rhs, (max - val));
				}
			}
			else {
				val += rhs;
				clamp_self();
			}
			return *this;
		}
		constexpr saturating& addpos(T rhs) noexcept requires(is_signed) {
			T tomax = std::bit_cast<T>(std::min<uT>((std::bit_cast<uT>(max) - std::bit_cast<uT>(val)), constants::taxabs));
			(rhs > tomax) ? rhs = tomax : rhs;
			val += rhs;
			return *this;
		}
		constexpr saturating& addneg(T rhs) noexcept requires(is_signed) {
			T tomin = -std::bit_cast<T>(std::min<uT>((std::bit_cast<uT>(val) - std::bit_cast<uT>(min)), constants::tminabs));
			(rhs < tomin) ? rhs = tomin : rhs;
			val += rhs;
			return *this;
		}

		constexpr saturating& operator-=(T rhs) noexcept {
			if constexpr (is_integral) {
				if constexpr (is_signed) {
					T tomin = std::bit_cast<T>(std::clamp<uT>((std::bit_cast<uT>(val) - std::bit_cast<uT>(min)), 0, constants::tminabs));
					T tomax = -std::bit_cast<T>(std::clamp<uT>((std::bit_cast<uT>(max) - std::bit_cast<uT>(val)), 0, constants::taxabs));
					(rhs > tomin) ? rhs = tomin : rhs;
					(rhs < tomax) ? rhs = tomax : rhs;
					val -= rhs;
				}
				else {
					val -= mymin<T>(rhs, (val - min));
				}
			}
			else {
				val -= rhs;
				clamp_self();
			}
			return *this;
		}
		constexpr saturating& subpos(T rhs) noexcept requires(is_signed) {
			T tomin = std::bit_cast<T>(std::clamp<uT>((std::bit_cast<uT>(val) - std::bit_cast<uT>(min)), 0, constants::tminabs));
			(rhs > tomin) ? rhs = tomin : rhs;
			val -= rhs;
			return *this;
		}
		constexpr saturating& subneg(T rhs) noexcept requires(is_signed) {
			T tomax = -std::bit_cast<T>(std::clamp<uT>((std::bit_cast<uT>(max) - std::bit_cast<uT>(val)), 0, constants::taxabs));
			(rhs < tomax) ? rhs = tomax : rhs;
			val -= rhs;
			return *this;
		}

		constexpr saturating& operator*=(const T rhs) noexcept {
			if constexpr (is_integral) {
				if constexpr (under64) { // Under 64bit
					if constexpr (is_signed) { // Under 64bit signed
						enum : swT {
							wmin = static_cast<swT>(min),
							wmax = static_cast<swT>(max)
						};
						swT temp = static_cast<swT>(val) * static_cast<swT>(rhs);
						temp = (temp < wmin) ? wmin : temp; // Don't clamp. Shit codegen.
						temp = (temp > wmax) ? wmax : temp;
						val = static_cast<T>(temp);
					}
					else { // Under 64bit unsigned
						uwT temp = static_cast<uwT>(val) * rhs;
						val = static_cast<T>(mymin<uwT>(temp, max));
						val = mymax(val, min); // Handle mul with 0
					}
				}
				else { // 64bit
					if constexpr (is_signed) { // 64bit signed
						/*The unsigned method used is ~20% faster. Perhaps there is more optimization to be done so saving this here too.
						sT high;
						sT low = _mul128(val, rhs, &high);
						const u64t zero_if_no_overflow = std::bit_cast<u64t>(high) + (std::bit_cast<u64t>(low) >> 63); // 0 if 0+0 or if u64max + 1. So if the top 65 (64 + 1) bits are the same, this becomes 0.
						const sT highsign = (high >> 63);
						const sT overflow_mask = std::bit_cast<sT>(static_cast<u64t>((zero_if_no_overflow != 0)) << 63) >> 63; // All 1s if overflow, all 0s otherwise.
						val = low & ~overflow_mask; // Sets to 0 if overflow, and sets to low otherwise.
						val += INT64_MIN & (overflow_mask & highsign); // Adds 0 if no overflow or high not negative, and adds INT64_MIN otherwise.
						val += INT64_MAX & (overflow_mask & ~highsign); // Adds 0 if no overflow or high negative, and adds INT64_MAX otherwise.
						val = std::max(val, static_cast<sT>(min)); // Manually inlining them produces branches...
						val = std::min(val, static_cast<sT>(max));*/

						const uT valsgn = std::bit_cast<uT>(val >> 63); // Calculate signs of inputs. Arithmetic right shift. All 1s if negative, all 0s otherwise.
						const uT rhssgn = std::bit_cast<uT>(rhs >> 63);
						const uT ressgn = valsgn ^ rhssgn; // Sign of the result. All 1s if exactly one of val and rhs is negative, all 0s otherwise.

						const uT valabs = (std::bit_cast<uT>(val) ^ valsgn) - valsgn; // Absolute of val. Treat as unsigned cause absolute of min is 1 above absolute of max.
						const uT rhsabs = (std::bit_cast<uT>(rhs) ^ rhssgn) - rhssgn; // Absolute of rhs.

						uT high; // Will hold the high 64 bits of the 128 bit result.
						uT low = _umul128(valabs, rhsabs, &high); // Returns the low 64 bits. Remember, this is UNSIGNED multiplication.

						const uT maxlow = (constants::minabs & ressgn) | (constants::maxabs & ~ressgn); // minabs if result is negative, maxabs if result is positive.

						// Overflow if any high bit is set, or if low is bigger than max allowed. Shift left as unsigned (signed shl is UB), then right as signed for arithmetic shift, then store as unsigned cause w/e.
						const uT overflow_mask = std::bit_cast<uT>(std::bit_cast<i64t>(static_cast<uT>((high != 0) | (low > maxlow)) << 63) >> 63); // All 1s if overflow, all 0s otherwise.

						low = (low & ~overflow_mask) | (maxlow & overflow_mask); // Keep as is if not overflow, or set to the appropriate absolute min or max depending on result sign otherwise.
						low = static_cast<uT>(low ^ ressgn) - ressgn; // Sign to the value.
						val = std::bit_cast<i64t>(low); // Reinterpret as i64t
					}
					else { // 64bit unsigned
						uT high;
						val = _umul128(val, rhs, &high); // Don't use temporary for low. Worse codegen.
						const uT overflow_mask = std::bit_cast<uT>(std::bit_cast<i64t>(static_cast<uT>(high != 0) << 63) >> 63);
						val = (overflow_mask & max) | (~overflow_mask & val);
						val = clamp_minmax(val);
					}
				}
			}
			else {
				val *= rhs;
				clamp_self();
			}
			return *this;
		}

		constexpr saturating& operator/=(const T rhs) noexcept {
			if constexpr (is_integral) {
				if constexpr (is_signed) {
					if constexpr (min == std::numeric_limits<T>::min()) { // val could be minimum T, and if it is and rhs is -1 then overflow
						enum : T {
							tmin = (1i32 << (sizeof(T) * 8i32 - 1i32))
						};
						val += ((val == tmin) & (rhs == -1)); // Avoid that by adding 1 to val, making its absolute equal to the absolute of max, and division by /-1 later return max, which is what we want.
					}
					T temp = val / rhs;
					temp = (temp < min) ? min : temp; // Don't change to assignment only if cond is true. Generates branches. Idfk.
					temp = (temp > max) ? max : temp;
					val = temp;
				}
				else {
					val /= rhs;
					val = mymax(val, min);
				}
			}
			else {
				val /= rhs;
				clamp_self();
			}
			return *this;
		}

		constexpr saturating& operator++() noexcept requires(is_integral) {
			val += (val != max);
			return *this;
		}
		constexpr T operator++(int) noexcept requires(is_integral) {
			const T old = val;
			val += (val != max);
			return old;
		}

		constexpr saturating& operator--() noexcept requires(is_integral) {
			// Go full circle (max + 1) if at min (no change) or 1 short otherwise (-1). Faster than sub.
			val = std::bit_cast<T>(static_cast<uT>(std::bit_cast<uT>(val) + constants::utmax + static_cast<uT>(val == min)));
			return *this;
		}
		constexpr T operator--(int) noexcept requires(is_integral) {
			const T old = val;
			val = std::bit_cast<T>(static_cast<uT>(std::bit_cast<uT>(val) + constants::utmax + static_cast<uT>(val == min)));
			return old;
		}


		[[nodiscard]] constexpr i8t		i8()	const noexcept requires(not std::is_same_v<T, i8t>)		{ return static_cast<i8t>(val); }
		[[nodiscard]] constexpr i16t	i16()	const noexcept requires(not std::is_same_v<T, i16t>)	{ return static_cast<i16t>(val); }
		[[nodiscard]] constexpr i32t	i32()	const noexcept requires(not std::is_same_v<T, i32t>)	{ return static_cast<i32t>(val); }
		[[nodiscard]] constexpr i64t	i64()	const noexcept requires(not std::is_same_v<T, i64t>)	{ return static_cast<i64t>(val); }
		[[nodiscard]] constexpr u8t		u8()	const noexcept requires(not std::is_same_v<T, u8t>)		{ return static_cast<u8t>(val); }
		[[nodiscard]] constexpr u16t	u16()	const noexcept requires(not std::is_same_v<T, u16t>)	{ return static_cast<u16t>(val); }
		[[nodiscard]] constexpr u32t	u32()	const noexcept requires(not std::is_same_v<T, u32t>)	{ return static_cast<u32t>(val); }
		[[nodiscard]] constexpr u64t	u64()	const noexcept requires(not std::is_same_v<T, u64t>)	{ return static_cast<u64t>(val); }

		[[nodiscard]] constexpr float	flt()	const noexcept { return static_cast<float>(val); }
		[[nodiscard]] constexpr double	dbl()	const noexcept { return static_cast<double>(val); }

		[[nodiscard]] auto str() const { return std::to_string(val); }
		[[nodiscard]] auto stri64() const requires(not is_integral) { return std::to_string(static_cast<i64t>(val)); } // Truncated floating

	private:
		T val{};
	};
	static_assert(std::is_trivially_copyable_v<saturating<std::uint32_t, 1, 100>>);
	static_assert(std::is_trivially_copyable_v<saturating<std::int64_t, -100, 100>>);
	static_assert(std::is_trivially_copyable_v<saturating<float, -100.0f, 100.0f>>);

	
	template <typename T> requires (std::integral<T> or std::floating_point<T>)
	struct optional_num {
	public:
		explicit constexpr optional_num() noexcept = default;
		constexpr optional_num(const optional_num&) noexcept = default;
		constexpr optional_num(optional_num&&) noexcept = default;
		constexpr optional_num& operator=(const optional_num&) noexcept = default;
		constexpr optional_num& operator=(optional_num&&) noexcept = default;
		constexpr ~optional_num() noexcept = default;

		explicit constexpr optional_num(const T initval) noexcept : hasval(true), val(initval) {}
		constexpr optional_num& operator=(const T newval) noexcept { hasval = true; val = newval; return *this; }

		constexpr optional_num(const std::optional<T>& rhs) noexcept : hasval(rhs.has_value()), val(hasval ? rhs.value() : T{}) {}
		constexpr optional_num& operator=(const std::optional<T>& rhs) noexcept {
			this->hasval = rhs.has_value();
			if (this->hasval) {
				this->val = rhs.value();
			}
			return *this;
		}
		constexpr operator std::optional<T>() const { if (hasval) { return { val }; } return std::nullopt; }


		constexpr bool has_value() const noexcept { return hasval; }
		constexpr T value_or(T fallback) const noexcept { return (hasval ? val : fallback); }
		constexpr T value() const noexcept { return (hasval ? val : T{}); }
		constexpr void reset() noexcept { hasval = false; }


	private:
		T val{};
		bool hasval{}; // Does default to false through zero-initialization
	};
	static_assert(std::is_trivially_copyable_v<optional_num<float>>);
	static_assert(std::is_trivially_copyable_v<optional_num<std::int64_t>>);

	struct optional_float {
	public:
		constexpr explicit optional_float() noexcept : val(NAN) {}
		constexpr optional_float(const optional_float&) noexcept = default;
		constexpr optional_float(optional_float&&) noexcept = default;
		constexpr optional_float& operator=(const optional_float&) noexcept = default;
		constexpr optional_float& operator=(optional_float&&) noexcept = default;
		constexpr ~optional_float() noexcept = default;

		constexpr explicit optional_float(const float initval) noexcept : val(initval) {}
		constexpr optional_float& operator=(const float newval) noexcept { val = newval; return *this; }


		bool has_value() const noexcept { return !std::isnan(val); }
		float value_or(float fallback) const noexcept { if (has_value()) { return { val }; } return fallback; }
		constexpr float value() const noexcept { return val; }
		constexpr void reset() noexcept { val = NAN; }

	private:
		float val;
	};
	static_assert(std::is_trivially_copyable_v<optional_float> and sizeof(optional_float) == 4);



	template<std::unsigned_integral T, std::size_t FlagCount> requires((FlagCount <= (sizeof(T) * CHAR_BIT)) and (FlagCount > 0))
	struct flaggy_uint {
	private:
		static constexpr std::size_t BitCount = sizeof(T) * CHAR_BIT;	// Number of bits in T
		static constexpr std::size_t MaxBitIdx = BitCount - 1;			// Maximum 0-based bit index of T
		static constexpr bool FlagsOnly = (FlagCount == BitCount);		// True if all bits store bools

		static constexpr T AllOnes = std::numeric_limits<T>::max();		// T with all bits set
		static constexpr T UintMask = static_cast<T>(static_cast<T>(AllOnes << FlagCount) >> FlagCount); // All bits storing flags 0. All others 1.
		static constexpr T FlagMask = static_cast<T>(~UintMask);		// All bits storing flags 1. All others 0.

	public:
		static constexpr T MaxNum = UintMask;

		explicit constexpr flaggy_uint() noexcept = default;
		constexpr flaggy_uint(const flaggy_uint&) noexcept = default;
		constexpr flaggy_uint(flaggy_uint&&) noexcept = default;
		constexpr flaggy_uint& operator=(const flaggy_uint&) noexcept = default;
		constexpr flaggy_uint& operator=(flaggy_uint&&) noexcept = default;
		constexpr ~flaggy_uint() noexcept = default;

		template<std::integral U> requires(sizeof(U) == sizeof(T))
		constexpr flaggy_uint(const U initbits) noexcept : val{ std::bit_cast<T>(initbits) } {}

		template<std::integral U> requires(FlagCount == 1)
		constexpr flaggy_uint(const bool initflag, const U initnum) noexcept
			: val{ static_cast<T>(static_cast<T>(static_cast<T>(initflag) << MaxBitIdx) | static_cast<T>(static_cast<T>(initnum) & UintMask)) }
		{}


		constexpr bool flag() const noexcept requires(FlagCount == 1) {
			return (val & FlagMask) > 0; // Better codegen than shift
		}
		constexpr void flag(const bool newflag) noexcept requires(FlagCount == 1) {
			val &= UintMask; // Unset
			val |= static_cast<T>(static_cast<T>(newflag) << MaxBitIdx); // Set to new value
		}
		constexpr void flag_andeq(const bool rhs) noexcept requires(FlagCount == 1) {
			val &= static_cast<T>(~static_cast<T>(static_cast<T>(not rhs) << MaxBitIdx));
		}
		constexpr void flag_oreq(const bool rhs) noexcept requires(FlagCount == 1) {
			val |= static_cast<T>(static_cast<T>(rhs) << MaxBitIdx);
		}
		constexpr void flag_noteq() noexcept requires(FlagCount == 1) {
			val ^= static_cast<T>(T{ 1 } << MaxBitIdx);
		}

		constexpr bool flag(const std::size_t pos) const noexcept requires(FlagCount > 1) {
			const std::uint8_t posmask = 1 << (MaxBitIdx - pos);
			return val & posmask; // Better codegen than shifts
		}
		constexpr void flag(const std::size_t pos, const bool newflag) noexcept requires(FlagCount > 1) {
			const std::size_t idx = MaxBitIdx - pos; 
			val &= static_cast<T>(~static_cast<T>(T{ 1 } << idx)); // Unset
			val |= static_cast<T>(static_cast<T>(newflag) << idx); // Set to new value
		}
		constexpr void flag_andeq(const std::size_t pos, const bool rhs) noexcept requires(FlagCount > 1) {
			val &= static_cast<T>(~static_cast<T>(static_cast<T>(not rhs) << (MaxBitIdx - pos)));
		}
		constexpr void flag_oreq(const std::size_t pos, const bool rhs) noexcept requires(FlagCount > 1) {
			val |= static_cast<T>(static_cast<T>(rhs) << (MaxBitIdx - pos));
		}
		constexpr void flag_noteq(const std::size_t pos) noexcept requires(FlagCount > 1) {
			val ^= static_cast<T>(T{ 1 } << pos);
		}


		constexpr T num() const noexcept requires(!FlagsOnly) {
			return val & UintMask;
		}
		constexpr void num(const T newnum) noexcept requires(!FlagsOnly) {
			val = static_cast<T>(val & FlagMask) | static_cast<T>(newnum & UintMask);
		}
		constexpr void num_shl(const std::uint8_t count) noexcept requires(!FlagsOnly) {
			val = static_cast<T>(val & FlagMask) | static_cast<T>(static_cast<T>(val << count) & UintMask);
		}
		constexpr void num_shr(const std::uint8_t count) noexcept requires(!FlagsOnly) {
			val = static_cast<T>(val & FlagMask) | static_cast<T>(static_cast<T>(val & UintMask) >> count);
		}


		constexpr T operator+=(const T rhs) noexcept requires(!FlagsOnly) {
			const T newnum = num() + rhs;
			num(newnum);
			return newnum;
		}
		constexpr T operator-=(const T rhs) noexcept requires(!FlagsOnly) {
			const T newnum = num() - rhs;
			num(newnum);
			return newnum;
		}

		constexpr T operator++() noexcept requires(!FlagsOnly) { return this->operator+=(1); }
		constexpr T operator--() noexcept requires(!FlagsOnly) { return this->operator-=(1); }


	private:
		T val{};	// The highest FlagCount bits each represent a boolean. Remaining bits, if any, represent an unsigned integer.
	};
	static_assert(sizeof(flaggy_uint<std::uint32_t, 13>) == sizeof(std::uint32_t));
	static_assert(std::is_trivially_copyable_v<flaggy_uint<std::uint32_t, 13>>);
	static_assert(flaggy_uint<std::uint8_t, 5>::MaxNum == 7);
	static_assert(flaggy_uint<std::uint8_t, 1>::MaxNum == 127);



}
