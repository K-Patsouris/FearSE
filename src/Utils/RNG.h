#pragma once
#include "Common.h"

namespace RNG {

	enum : u32 { sign_and_exponent = 127 << 23 };
	struct random_state {};

	// 64bit stack state. Fastest. Credit to Ian C. Bullard.
	class gamerand {
	public:
		explicit constexpr gamerand() noexcept = default;
		constexpr gamerand(gamerand&&) noexcept = default;
		constexpr gamerand(const gamerand&) noexcept = default;
		constexpr gamerand& operator=(gamerand&&) noexcept = default;
		constexpr gamerand& operator=(const gamerand&) noexcept = default;
		constexpr ~gamerand() noexcept = default;

		constexpr gamerand(const u32 state) noexcept : high{ state }, low{ state ^ 0x49616E42 } {}
		gamerand(random_state) noexcept : high{ std::random_device{}() }, low{ high ^ 0x49616E42 } {}

		constexpr void set_state(const u32 state) noexcept {
			high = state;
			low = state ^ 0x49616E42;
		}
		void set_state(random_state) noexcept {
			high = std::random_device{}();
			low = high ^ 0x49616E42;
		}

		constexpr u32 next() noexcept {
			high = static_cast<u32>(high >> 16) + static_cast<u32>(high << 16);
			high += low;
			low += high;
			return high;
		}
		// [0, cap). cap must be > 0
		constexpr u32 next(const u32 cap) noexcept { return next() % cap; }
		constexpr float nextf01() noexcept { return std::bit_cast<float>(static_cast<u32>(sign_and_exponent | static_cast<u32>(next() >> 9))) - 1.0f; }
		constexpr float nextf(const float min, const float max) noexcept { return min + (nextf01() * (max - min)); }

	private:
		u32 high{ 1 };
		u32 low{ 1 ^ 0x49616E42 };
	};

	// 64bit stack state. Fourth fastest.
	class mwc {
	public:
		constexpr mwc() noexcept = delete;
		constexpr mwc(mwc&&) noexcept = default;
		constexpr mwc(const mwc&) noexcept = default;
		constexpr mwc& operator=(mwc&&) noexcept = default;
		constexpr mwc& operator=(const mwc&) noexcept = default;
		constexpr ~mwc() noexcept = default;

		constexpr mwc(const u32 init_z, const u32 init_w) noexcept : z(init_z), w(init_w) {}
		mwc(random_state) noexcept : z(std::random_device{}()), w(std::random_device{}()) {}

		constexpr void set_state(const u32 sz, const u32 sw) noexcept {
			z = sz;
			w = sw;
		}
		void set_state(random_state) noexcept {
			std::random_device gen{};
			z = gen();
			w = gen();
		}

		constexpr u32 next() noexcept { return ((next_z() << 16) + next_w()); }
		// [0, cap). cap must be > 0
		constexpr u32 next(const u32 cap) noexcept { return next() % cap; }
		constexpr float nextf01() noexcept { return std::bit_cast<float>(static_cast<u32>(sign_and_exponent | static_cast<u32>(next() >> 9))) - 1.0f; }
		constexpr float nextf(const float min, const float max) noexcept { return min + (nextf01() * (max - min)); }

	private:
		__forceinline constexpr u32 next_z() noexcept { return (z = 36969 * (z & 65535) + (z >> 16)); }
		__forceinline constexpr u32 next_w() noexcept { return (w = 18000 * (w & 65535) + (w >> 16)); }

		u32 z{}; // Marsaglia defaults: z{ 362436069 }, w{ 521288629 }
		u32 w{};
	};

	// 128bit stack state. Fourth fastest. (tied)
	class xoshiro128ss {
	public:
		constexpr xoshiro128ss() noexcept = delete;
		constexpr xoshiro128ss(xoshiro128ss&&) noexcept = default;
		constexpr xoshiro128ss(const xoshiro128ss&) noexcept = default;
		constexpr xoshiro128ss& operator=(xoshiro128ss&&) noexcept = default;
		constexpr xoshiro128ss& operator=(const xoshiro128ss&) noexcept = default;
		constexpr ~xoshiro128ss() noexcept = default;

		constexpr xoshiro128ss(const u32 s0_, const u32 s1_, const u32 s2_, const u32 s3_) noexcept : s0(s0_), s1(s1_), s2(s2_), s3(s3_) {}
		xoshiro128ss(random_state) noexcept
			: s0(std::random_device{}())
			, s1(std::random_device{}())
			, s2(std::random_device{}())
			, s3(std::random_device{}())
		{}

		constexpr void set_state(const u32 s0_, const u32 s1_, const u32 s2_, const u32 s3_) noexcept {
			s0 = s0_;
			s1 = s1_;
			s2 = s2_;
			s3 = s3_;
		}
		void set_state(random_state) noexcept {
			std::random_device gen{};
			s0 = gen();
			s1 = gen();
			s2 = gen();
			s3 = gen();
		}

		constexpr u32 next() noexcept {
			const u32 result = rotl(s1 * 5, 7) * 9;
			const u32 t = s1 << 9;
			s2 ^= s0;
			s3 ^= s1;
			s1 ^= s2;
			s0 ^= s3;
			s2 ^= t;
			s3 = rotl(s3, 11);
			return result;
		}
		// [0, cap). cap must be > 0
		constexpr u32 next(const u32 cap) noexcept { return next() % cap; }
		constexpr float nextf01() noexcept { return std::bit_cast<float>(static_cast<u32>(sign_and_exponent | static_cast<u32>(next() >> 9))) - 1.0f; }
		constexpr float nextf(const float min, const float max) noexcept { return min + (nextf01() * (max - min)); }

	private:
		__forceinline static constexpr u32 rotl(const u32 x, const u32 k) noexcept { return (x << k) | (x >> (32ui32 - k)); }

		u32 s0{};
		u32 s1{};
		u32 s2{};
		u32 s3{};
	};

	// 128bit stack state. Third fastest. Apparently bad randomness on low bits.
	class xoshiro128p {
	public:
		constexpr xoshiro128p() noexcept = delete;
		constexpr xoshiro128p(xoshiro128p&&) noexcept = default;
		constexpr xoshiro128p(const xoshiro128p&) noexcept = default;
		constexpr xoshiro128p& operator=(xoshiro128p&&) noexcept = default;
		constexpr xoshiro128p& operator=(const xoshiro128p&) noexcept = default;
		constexpr ~xoshiro128p() noexcept = default;

		constexpr xoshiro128p(const u32 s0_, const u32 s1_, const u32 s2_, const u32 s3_) noexcept : s0(s0_), s1(s1_), s2(s2_), s3(s3_) {}
		xoshiro128p(random_state) noexcept
			: s0(std::random_device{}())
			, s1(std::random_device{}())
			, s2(std::random_device{}())
			, s3(std::random_device{}())
		{}

		constexpr void set_state(const u32 s0_, const u32 s1_, const u32 s2_, const u32 s3_) noexcept {
			s0 = s0_;
			s1 = s1_;
			s2 = s2_;
			s3 = s3_;
		}
		void set_state(random_state) noexcept {
			std::random_device gen{};
			s0 = gen();
			s1 = gen();
			s2 = gen();
			s3 = gen();
		}

		constexpr u32 next() noexcept {
			const u32 result = s0 + s3;
			const u32 t = s1 << 9;
			s2 ^= s0;
			s3 ^= s1;
			s1 ^= s2;
			s0 ^= s3;
			s2 ^= t;
			s3 = rotl(s3, 11);
			return result;
		}
		// [0, cap). cap must be > 0
		constexpr u32 next(const u32 cap) noexcept { return next() % cap; }
		constexpr float nextf01() noexcept { return std::bit_cast<float>(static_cast<u32>(sign_and_exponent | static_cast<u32>(next() >> 9))) - 1.0f; }
		constexpr float nextf(const float min, const float max) noexcept { return min + (nextf01() * (max - min)); }

	private:
		__forceinline static constexpr u32 rotl(const u32 x, const u32 k) noexcept { return (x << k) | (x >> (32ui32 - k)); }

		u32 s0{};
		u32 s1{};
		u32 s2{};
		u32 s3{};
	};

	// 96bit stack state. Generally second fastest but kind of volatile.
	class xorshf96 {
	public:
		constexpr xorshf96() noexcept = delete;
		constexpr xorshf96(xorshf96&&) noexcept = default;
		constexpr xorshf96(const xorshf96&) noexcept = default;
		constexpr xorshf96& operator=(xorshf96&&) noexcept = default;
		constexpr xorshf96& operator=(const xorshf96&) noexcept = default;
		constexpr ~xorshf96() noexcept = default;

		constexpr xorshf96(const u32 s0_, const u32 s1_, const u32 s2_) noexcept : s0(s0_), s1(s1_), s2(s2_) {}
		xorshf96(random_state) noexcept
			: s0(std::random_device{}())
			, s1(std::random_device{}())
			, s2(std::random_device{}())
		{}

		constexpr void set_state(const u32 s0_, const u32 s1_, const u32 s2_) noexcept {
			s0 = s0_;
			s1 = s1_;
			s2 = s2_;
		}
		void set_state(random_state) noexcept {
			std::random_device gen{};
			s0 = gen();
			s1 = gen();
			s2 = gen();
		}

		constexpr u32 next() {
			s0 ^= s0 << 16;
			s0 ^= s0 >> 5;
			s0 ^= s0 << 1;

			u32 t = s0;
			s0 = s1;
			s1 = s2;
			s2 = t ^ s0 ^ s1;
			return s2;
		}
		// [0, cap). cap must be > 0
		constexpr u32 next(const u32 cap) noexcept { return next() % cap; }
		constexpr float nextf01() noexcept { return std::bit_cast<float>(static_cast<u32>(sign_and_exponent | static_cast<u32>(next() >> 9))) - 1.0f; }
		constexpr float nextf(const float min, const float max) noexcept { return min + (nextf01() * (max - min)); }

	private:
		u32 s0{};
		u32 s1{};
		u32 s2{};
	};


	// mt19937 takes ~2us just for seeded construction cause 5KB stack state. static mostly amends that. It generates ~200 scaled floats per 1us, varying wildly (0.8us-2.5us).
	// Faster than rand (which is SLOW) and about as fast as minstd, not faster than mwc or xoshiro.
	// Seeding: std::random_device is about as fast as std::time, but falls of a cliff when it runs out of entropy which is often (low 100s calls on my system).
	// For sparse/bulky calls they are fine, using a static vastly outperforms both for frequent calls.

	// Floatings
	template <size_t> struct helper_types { using uT = void; };
	// Integrals
	template <> struct helper_types<1> { using uT = u8; };
	template <> struct helper_types<2> { using uT = u16; };
	template <> struct helper_types<4> { using uT = u32; };
	template <> struct helper_types<8> { using uT = u64; };

	// Expects min <= max
	template <typename T> requires (std::integral<T> or std::is_same_v<T, float>) // double support would be a pain, and no use case.
	T Random(T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max()) noexcept {
		if (min == max) {
			return min;
		}
		static gamerand gen{ std::random_device{}() };
		if constexpr (std::integral<T>) {
			using uT =
				std::conditional_t<sizeof(T) == 8, u64,
					std::conditional_t<sizeof(T) == 4, u32,
						std::conditional_t<sizeof(T) == 2, u16,
							u8
				>>>;
			uT val;
			if constexpr (sizeof(T) == 8) {
				val = static_cast<u64>(static_cast<u64>(gen.next()) << 32) & static_cast<u64>(gen.next());
			} else {
				val = static_cast<uT>(gen.next());
			}
			val = val % (std::bit_cast<uT>(max) - std::bit_cast<uT>(min)); // Works for 2's complement signeds
			return min + static_cast<T>(val); // Cast to T for signed
		} else {
			return gen.nextf(min, max);
		}
	}
	// Expects min <= max
	template <typename T> requires (std::integral<T> or std::floating_point<T>)
	std::vector<T> RandomVec(const std::size_t size, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) noexcept {
		std::vector<T> result(size); // Let allocation failure terminate
		static std::mt19937 generator{ std::random_device{}() };
		if constexpr (std::integral<T>) {
			if constexpr (std::numeric_limits<T>::digits == 8) {
				std::uniform_int_distribution<std::int16_t> distribution{ min, max }; // distribution needs at least 16 bits
				for (auto& r : result) {
					r = static_cast<T>(distribution(generator));
				}
			}
			else {
				std::uniform_int_distribution<T> distribution{ min, max };
				for (auto& r : result) {
					r = distribution(generator);
				}
			}
		}
		else {
			std::uniform_real_distribution<T> distribution{ min, max };
			for (auto& r : result) {
				r = distribution(generator);
			}
		}
		return result;
	}
	// Expects min <= max
	template<typename T, u64 size>
	array<T, size> RandomArray(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) noexcept {
		array<T, size> result{}; // Allocation failure would be stack overflow
		static std::mt19937 generator{ std::random_device{}() };
		if constexpr (std::integral<T>) {
			if constexpr (std::numeric_limits<T>::digits == 8) {
				std::uniform_int_distribution<std::int16_t> distribution{ min, max }; // distribution needs at least 16 bits
				for (auto& r : result) {
					r = static_cast<T>(distribution(generator));
				}
			}
			else {
				std::uniform_int_distribution<T> distribution{ min, max };
				for (auto& r : result) {
					r = distribution(generator);
				}
			}
		}
		else {
			std::uniform_real_distribution<T> distribution{ min, max };
			for (auto& r : result) {
				r = distribution(generator);
			}
		}
		return result;
	}



}
