#pragma once
#include "Common.h"
#include <intrin.h>
#pragma intrinsic(_mm_pause)

namespace SyncTypes {
	using mo = std::memory_order;

	enum class LockingMode {
		Shared,
		Exclusive
	};

	template<typename LockT, LockingMode Mode = LockingMode::Exclusive>
	requires (	((Mode == LockingMode::Exclusive)	and noexcept(std::declval<LockT>().lock())) or
				((Mode == LockingMode::Shared)		and noexcept(std::declval<LockT>().lock_shared())) )
	class noexlock_guard {
	public:
		explicit noexlock_guard(LockT& lock) noexcept : lock_ref{ lock } {
			if constexpr (Mode == LockingMode::Exclusive) {
				lock_ref.lock();
			} else {
				lock_ref.lock_shared();
			}
		}
		~noexlock_guard() noexcept {
			if constexpr (Mode == LockingMode::Exclusive) {
				lock_ref.unlock();
			} else {
				lock_ref.unlock_shared();
			}
		}

		noexlock_guard(const noexlock_guard&) = delete;
		noexlock_guard(noexlock_guard&&) = delete;
		noexlock_guard& operator=(const noexlock_guard&) = delete;
		noexlock_guard& operator=(noexlock_guard&&) = delete;

	private:
		LockT& lock_ref; // Must take by pointer for move constructor
	};

	class spinlock {
		using nanoseconds = std::chrono::nanoseconds;
		using steady_clock = std::chrono::steady_clock;
		std::atomic<bool> locked{ false };
		static_assert(decltype(locked)::is_always_lock_free, "spinlock flag implementation is not always lock-free, so probably just use a (shared) mutex");
	public:
		void lock() noexcept {
			for (;;) {
				if (locked.exchange(true, mo::acquire) == false) { // acquire is enough, making Loads/stores unable to move up. This thread should not be operating on protected data before lock() returns.
					return; // Previous value was false. Got the lock.
				}
				do {
					_mm_pause();  // Noop without SSE2 (released in 2000).
				} while (locked.load(mo::relaxed)); // This needn't protect anything, so use relaxed.
			}
		}
		[[nodiscard]] bool try_lock(const nanoseconds timeout) noexcept {
			if (timeout <= 0ns) {
				return locked.exchange(true, mo::acquire) == false;
			} else {
				// Assume no overflow. Overflow would happen if the cutoff was a point in time after the year 2262, because eithr timeout_ns or ::now() too big. So, yeah.
				const auto cutoff = steady_clock::now() + timeout;
				for (;;) {
					if (locked.exchange(true, mo::acquire) == false) {
						return true;
					}
					do {
						_mm_pause();
						if (steady_clock::now() >= cutoff) {
							return false;
						}
					} while (locked.load(mo::relaxed));
				}
			}
		}
		void unlock() noexcept {
			locked.store(false, mo::release); // No UB if unlocked without being locked. That's fine, I guess.
		}
	};
	static_assert(sizeof(spinlock) == 1);

	class shared_spinlock {
		using lock_t = u32;
		enum : lock_t {
			Unused = 0,
			Writing = std::numeric_limits<lock_t>::max(),
			MaxReaders = Writing - 1 // Any value < Writing works
		};

		std::atomic<lock_t> locked{ 0 };
		static_assert(decltype(locked)::is_always_lock_free, "shared_spinlock lock implementation is not always lock-free, so probably just use a shared mutex");
	public:
		void lock() noexcept {
			for (;;) {
				lock_t expected = Unused;
				if (locked.compare_exchange_strong(expected, Writing, mo::acquire, mo::relaxed)) { // acquire on success, relaxed on failure (not like it does anything on x86)
					return;
				}
				do {
					_mm_pause();
				} while (locked.load(mo::relaxed) != Unused);
			}
		}
		void unlock() noexcept {
			locked.store(Unused, mo::release); // Can cause UB later if not having lock() previously by underflowing when potential readers do unlock_shared()
		}

		void lock_shared() noexcept {
			lock_t expected = 0, desired = 1;
			for (;;) {
				if (locked.compare_exchange_strong(expected, desired, mo::acquire, mo::relaxed)) {
					return;
				}
				while (expected >= MaxReaders) { // Spin-load if locking failed because not Hurdle_23. Skip and go straight to CAS with corrected values otherwise.
					_mm_pause();
					expected = locked.load(mo::relaxed);
				}
				desired = expected + 1; // Here, expected represents a Hurdle_23 state (< MaxReaders which implies != Writing)
			}
		}
		void unlock_shared() noexcept {
			locked.fetch_sub(1, mo::release); // UB (potential underflow or changing writing state to reading state) if not having lock_shared() previously
		}
	};
	static_assert(sizeof(shared_spinlock) == 4);

	template<typename payload, typename shared_lock = shared_spinlock> requires((sizeof(payload) + sizeof(shared_lock)) <= 64)
	class alignas(64) LockProtectedResourceAligned {
		template<size_t padbytes>
		struct with_padding_t {
			payload data{};
			shared_lock lock{};
			char pad[padbytes];
		};
		struct without_padding_t {
			payload data{};
			shared_lock lock{};
		};
		static constexpr bool NeedPadding = sizeof(without_padding_t) < 64;
		static constexpr size_t PadBytes = 64 - sizeof(without_padding_t);
		using grouped_t = std::conditional_t<NeedPadding, with_padding_t<PadBytes>, without_padding_t>;
		// I want the payload and the lock to end up in the same cacheline so padding may have to be added because MSVC warns on alignas() introducing padding and I want to keep the warnings.
		// Just adding a char pad[PadBytes] fails if PadBytes == 0 because 0 sized stack object yada yada. So I have to do this cancer.


		// Lock is taken/released with RAII
		template<bool AllowWrite>
		class LockedAccessor {
		public:
			using shared_guard_t = SyncTypes::noexlock_guard<shared_lock, SyncTypes::LockingMode::Shared>;
			using exclusive_guard_t = SyncTypes::noexlock_guard<shared_lock, SyncTypes::LockingMode::Exclusive>;
			using vec_t = std::conditional_t<AllowWrite, payload, const payload>;
			using guard_t = std::conditional_t<AllowWrite, exclusive_guard_t, shared_guard_t>;

			friend class Locker;
			explicit LockedAccessor(vec_t& vec, shared_lock& lock) noexcept : vecptr{ &vec }, guard{ lock } {}
			LockedAccessor(const LockedAccessor&) = delete;
			LockedAccessor(LockedAccessor&&) = delete;

			vec_t* vecptr;
			guard_t guard;

		public:
			vec_t* operator->() const noexcept { return vecptr; }
			vec_t& operator*() const noexcept { return *vecptr; }
		};

		grouped_t data_and_lock{};

	public:
		LockProtectedResourceAligned() noexcept = default;
		LockProtectedResourceAligned(const LockProtectedResourceAligned&) = delete;
		LockProtectedResourceAligned(LockProtectedResourceAligned&&) = delete;

		LockedAccessor<true> GetExclusive() noexcept { return LockedAccessor<true>{ data_and_lock.data, data_and_lock.lock }; }
		LockedAccessor<false> GetShared() noexcept { return LockedAccessor<false>{ data_and_lock.data, data_and_lock.lock }; }
	};

	template<typename payload, typename shared_lock = shared_spinlock>
	class LockProtectedResource {
		// Lock is taken/released with RAII
		template<bool AllowWrite>
		class LockedAccessor {
		public:
			using shared_guard_t = SyncTypes::noexlock_guard<shared_lock, SyncTypes::LockingMode::Shared>;
			using exclusive_guard_t = SyncTypes::noexlock_guard<shared_lock, SyncTypes::LockingMode::Exclusive>;
			using data_t = std::conditional_t<AllowWrite, payload, const payload>;
			using guard_t = std::conditional_t<AllowWrite, exclusive_guard_t, shared_guard_t>;

			friend class Locker;
			explicit LockedAccessor(data_t& data, shared_lock& lock) noexcept : dataptr{ &data }, guard{ lock } {}
			LockedAccessor(const LockedAccessor&) = delete;
			LockedAccessor(LockedAccessor&&) = delete;

			data_t* operator->() const noexcept { return dataptr; }
			data_t& operator*() const noexcept { return *dataptr; }

		private:
			data_t* dataptr;
			guard_t guard;
		};

		payload data{};
		shared_lock lock{};

	public:
		LockProtectedResource() noexcept = default;
		LockProtectedResource(const LockProtectedResource&) = delete;
		LockProtectedResource(LockProtectedResource&&) = delete;

		LockedAccessor<true> GetExclusive() noexcept { return LockedAccessor<true>{ data, lock }; }
		LockedAccessor<false> GetShared() noexcept { return LockedAccessor<false>{ data, lock }; }
	};



}
