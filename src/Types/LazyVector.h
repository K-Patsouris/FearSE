#pragma once
#include "Common.h"
#include "Logger.h"
#include "Utils/PrimitiveUtils.h"

namespace LazyVector {

	consteval bool is_power_of_2(const size_t val) { return (val != 0) and ((val & (val - 1)) == 0); }

	// Simple vector-like container, with optimizations for types with trivial special member functions, less strict built-in methods, and some unchecked interface functions.
	// The main motivation for it was to somehow do away with the redundant capacity checks vector::push_back() does in the reserve-push_back idiom.
	// This can call reserve() to allocate, and then do append() for unchecked in-place constructions.
	// For trivially_default_constructible T: default constructed elements (resize) will be memset to 0 without constructor calls.
	// For trivially_copy_constructible T: copy-construction will memcpy from other without constructor calls.
	// For trivially_copy_assignable T: if also trivially_copy_constructible, copy-assignment too will memcpy from other without constructor calls.
	// For trivially_move_constructible T: reallocations triggered by exceeding capacity will use realloc() without constructor calls.
	// For trivially_move_assignable T: methods that rearrange elements will use memcpy/memmove, potentially in bulk, without assignment calls.
	// For trivially_destructible T: destructors will never be called.
	// For other T: constructors, assignments, and destructors will be called as appropriate, similar to std::vector<T>.
	template<typename T, size_t Alignment = alignof(T)>
	requires (
		(Alignment >= alignof(T))			// No underaligning
		and (Alignment <= sizeof(T))		
		and (sizeof(T) % Alignment == 0)	// No support for pad between elements
		and (is_power_of_2(Alignment)))		// Integral power of 2
	class lazy_vector {
	private:
		static constexpr size_t Bytes = sizeof(T);
		static constexpr bool Small = Bytes <= 8;

		static constexpr bool TrivialDefaultConstruction = std::is_trivially_default_constructible_v<T>;
		static constexpr bool TrivialCopyConstruction = std::is_trivially_copy_constructible_v<T>;
		static constexpr bool TrivialMoveConstruction = std::is_trivially_move_constructible_v<T>;
		static constexpr bool TrivialCopyAssignment = std::is_trivially_copy_assignable_v<T>;
		static constexpr bool TrivialMoveAssignment = std::is_trivially_move_assignable_v<T>;
		static constexpr bool TrivialDestruction = std::is_trivially_destructible_v<T>;

		static constexpr bool SafeDefaultConstruction = std::is_nothrow_default_constructible_v<T>; // Always true if TrivialDefaultConstruction == true.
		static constexpr bool SafeCopyConstruction = std::is_nothrow_copy_constructible_v<T>; // Always true if TrivialCopying == true.
		static constexpr bool SafeMoveConstruction = std::is_nothrow_move_constructible_v<T>; // Always true if TrivialCopying == true.
		static constexpr bool SafeCopyAssignment = std::is_nothrow_copy_assignable_v<T>; // Always true if TrivialCopying == true.
		static constexpr bool SafeMoveAssignment = std::is_nothrow_move_assignable_v<T>; // Always true if TrivialCopying == true.

	public:
		using size_type = size_t;
		using value_type = T;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;
		using iterator = pointer;
		using const_iterator = const_pointer;

		constexpr lazy_vector() noexcept = default;
		constexpr lazy_vector(lazy_vector&& other) noexcept
			: arr_first(other.arr_first)
			, arr_sentinel(other.arr_sentinel)
			, alloc_sentinel(other.alloc_sentinel)
		{
			other.arr_first = nullptr;
			other.arr_sentinel = nullptr;
			other.alloc_sentinel = nullptr;
		}
		constexpr lazy_vector(const lazy_vector& other) noexcept(SafeCopyConstruction) {
			if (other.arr_first and !copy_init(other.arr_first, (other.arr_sentinel - other.arr_first))) {
				SKSE::stl::report_and_fail("out of memory"sv);
			}
		}
		constexpr lazy_vector& operator=(lazy_vector&& rhs) noexcept {
			if (this != std::addressof(rhs)) {
				purge_self();

				this->arr_first = rhs.arr_first;
				this->arr_sentinel = rhs.arr_sentinel;
				this->alloc_sentinel = rhs.alloc_sentinel;

				rhs.arr_first = nullptr;
				rhs.arr_sentinel = nullptr;
				rhs.alloc_sentinel = nullptr;
			}
			return *this;
		}
		constexpr lazy_vector& operator=(const lazy_vector& rhs) noexcept((SafeCopyConstruction and SafeCopyAssignment) or (TrivialCopyConstruction and TrivialCopyAssignment)) {
			if (this != std::addressof(rhs)) {
				copy_assign(rhs.arr_first, rhs.arr_sentinel);
			}
			return *this;
		}
		constexpr ~lazy_vector() noexcept { purge_self(); }

		constexpr lazy_vector(const size_t init_size) noexcept(SafeDefaultConstruction) {
			if (!resize(init_size)) {
				SKSE::stl::report_and_fail("out of memory"sv); // resize() only returns false if oom. It won't return if constructor throws.
			}
		}
		constexpr lazy_vector(const T* init_data, const size_t init_size) noexcept(SafeCopyConstruction) {
			if (init_data and (init_size > 0) and !copy_init(init_data, init_size)) {
				SKSE::stl::report_and_fail("out of memory"sv);
			}
		}
		constexpr lazy_vector(initializer_list<T> il) noexcept(SafeCopyConstruction) {
			if ((il.size() > 0) and !copy_init(il.begin(), il.size())) { // Will avoid allocation of the 1 past-the-end element if il is empty
				SKSE::stl::report_and_fail("out of memory"sv);
			}
		}

		constexpr lazy_vector(const std::vector<T>& vec) noexcept(SafeCopyConstruction) : lazy_vector(vec.data(), vec.size()) {}
		constexpr lazy_vector& operator=(const std::vector<T>& rhs) noexcept((SafeCopyConstruction and SafeCopyAssignment) or (TrivialCopyConstruction and TrivialCopyAssignment)) {
			copy_assign(rhs.data(), rhs.data() + rhs.size());
			return *this;
		}


		constexpr size_t capacity() const noexcept { return alloc_sentinel - arr_first; }
		constexpr size_t size() const noexcept { return arr_sentinel - arr_first; }
		constexpr bool empty() const noexcept { return arr_first == arr_sentinel; }

		constexpr T* data() noexcept { return arr_first; }
		constexpr const T* data() const noexcept { return arr_first; }

		constexpr T& operator[](const size_t index) noexcept { return arr_first[index]; }
		constexpr const T& operator[](const size_t index) const noexcept { return arr_first[index]; }

		constexpr T& front() noexcept { return *arr_first; }
		constexpr const T& front() const noexcept { return *arr_first; }
		constexpr T& back() noexcept { return *(arr_sentinel - 1); }
		constexpr const T& back() const noexcept { return *(arr_sentinel - 1); }

		constexpr T* begin() noexcept { return arr_first; }
		constexpr const T* begin() const noexcept { return arr_first; }
		constexpr T* end() noexcept { return arr_sentinel; }
		constexpr const T* end() const noexcept { return arr_sentinel; }


		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find_in(const U val, T* from, T* to) const noexcept {
			T* it = from;
			for (; (it != to) and (*it != val); ++it) { ; }
			return it;
		}
		constexpr T* find_in(const T& elem, T* from, T* to) const noexcept requires (!Small) {
			T* it = from;
			for (; (it != to) and (*it != elem); ++it) { ; }
			return it;
		}
		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find_in(const U val, const size_t start, const size_t end) const noexcept { return find_in(val, arr_first + start, arr_first + end); }
		constexpr T* find_in(const T& elem, const size_t start, const size_t end) const noexcept requires (!Small) { return find_in(elem, arr_first + start, arr_first + end); }

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find_until(const U val, T* to) const noexcept { return find_in(val, arr_first, to); }
		constexpr T* find_until(const T& elem, T* to) const noexcept requires (!Small) { return find_in(elem, arr_first, to); }
		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find_until(const U val, const size_t idx) const noexcept { return find_in(val, arr_first, arr_first + idx); }
		constexpr T* find_until(const T& elem, const size_t idx) const noexcept requires (!Small) { return find_in(elem, arr_first, arr_first + idx); }

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find_from(const U val, T* from) const noexcept { return find_in(val, from, arr_sentinel); }
		constexpr T* find_from(const T& elem, T* from) const noexcept requires (!Small) { return find_in(elem, from, arr_sentinel); }
		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find_from(const U val, const size_t idx) const noexcept { return find_in(val, arr_first + idx, arr_sentinel); }
		constexpr T* find_from(const T& elem, const size_t idx) const noexcept requires (!Small) { return find_in(elem, arr_first + idx, arr_sentinel); }

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr T* find(const U val) const noexcept { return find_in(val, arr_first, arr_sentinel); }
		constexpr T* find(const T& elem) const noexcept requires (!Small) { return find_in(elem, arr_first, arr_sentinel); }

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr bool contains_until(const U val, const size_t idx) const noexcept {
			T* to = arr_first + idx;
			return find_until(val, to) != to;
		}
		constexpr bool contains_until(const T& elem, const size_t idx) const noexcept requires (!Small) {
			T* to = arr_first + idx;
			return find_until(elem, to) != to;
		}

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr bool contains(const U val) const noexcept { return find(val) != arr_sentinel; }
		constexpr bool contains(const T& elem) const noexcept requires (!Small) { return find(elem) != arr_sentinel; }

		constexpr bool resize(const size_t newsize) noexcept(SafeDefaultConstruction) {
			const size_t size = static_cast<size_t>(arr_sentinel - arr_first);
			if (newsize < size) {
				if constexpr (!TrivialDestruction) {
					destruct(arr_first + newsize, arr_sentinel);
				}
				arr_sentinel = arr_first + newsize;
				return true;
			}
			if (newsize > size) {
				if ((newsize <= static_cast<size_t>(alloc_sentinel - arr_first)) or expand(newsize)) {
					arr_sentinel = arr_first + newsize;
					default_construct(arr_first + size, arr_sentinel);
					return true;
				}
				return false; // Only fail case is needing and failing to exceed capacity
			}
			return true;
		}

		constexpr bool reserve(const size_t newcacapity) noexcept(SafeMoveConstruction) {
			if (newcacapity <= static_cast<size_t>(alloc_sentinel - arr_first)) {
				return true;
			}
			return expand(newcacapity);
		}

		// No boundchecks
		template<typename... Args>
		constexpr void append(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) { std::construct_at(arr_sentinel++, std::forward<Args>(args)...); }
		template<typename U>
		constexpr void append_range_copy(const U* first, const U* last) noexcept(std::is_nothrow_constructible_v<T, const U>) {
			if constexpr (std::is_same_v<T, U> and TrivialCopyConstruction) {
				const size_t range_size = last - first;
				std::memcpy(arr_sentinel, first, range_size * sizeof(T));
				arr_sentinel += range_size;
			} else {
				while (first != last) {
					std::construct_at(arr_sentinel++, *(first++));
				}
			}
		}
		constexpr void append_range_move(T* first, T* last) noexcept(std::is_nothrow_move_constructible_v<T>) {
			if constexpr (TrivialMoveConstruction) {
				append_range_copy(first, last);
			} else {
				while (first != last) {
					std::construct_at(arr_sentinel++, std::move(*(first++)));
				}
			}
		}
		
		template<typename... Args>
		constexpr bool try_append(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
			if ((arr_sentinel != alloc_sentinel) or expand()) {
				std::construct_at(arr_sentinel++, std::forward<Args>(args)...); // Trivially copyable are optimized and inlined properly, same with append().
				return true;
			}
			return false;
		}
		template<typename U>
		constexpr bool try_append_range_copy(const U* first, const U* last) noexcept(std::is_nothrow_constructible_v<T, const U>) {
			const auto held = (arr_sentinel - arr_first);
			const auto available = (alloc_sentinel - arr_sentinel);
			const auto news = (last - first);
			if ((available >= news) or expand(static_cast<size_t>(held + news))) {
				append_range_copy(first, last);
				return true;
			}
			return false;
		}
		template<typename U>
		constexpr bool try_append_range_copy(const U* first, const size_t count) noexcept(std::is_nothrow_constructible_v<T, const U>) {
			return try_append_range_copy(first, first + count);
		}
		constexpr bool try_append_range_move(T* first, T* last) noexcept(std::is_nothrow_move_constructible_v<T>) {
			const auto held = (arr_sentinel - arr_first);
			const auto available = (alloc_sentinel - arr_sentinel);
			const auto news = (last - first);
			if ((available >= news) or expand(static_cast<size_t>(held + news))) {
				append_range_move(first, last);
				return true;
			}
			return false;
		}
		template<typename U>
		constexpr bool try_append_n(size_t count, const U u) noexcept(std::is_nothrow_constructible_v<T, const U>) {
			const auto held = (arr_sentinel - arr_first);
			const auto available = static_cast<size_t>(alloc_sentinel - arr_sentinel);
			if ((available >= count) or expand(static_cast<size_t>(held + count))) {
				for (; count > 0; --count) {
					std::construct_at(arr_sentinel++, u);
				}
				return true;
			}
			return false;
		}

		template<typename... Args>
		constexpr void append_sort(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
			append(std::forward<Args>(args)...);
			std::sort(arr_first, arr_sentinel);
		}

		template<typename... Args>
		constexpr bool try_append_sort(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
			if (try_append(std::forward<Args>(args)...)) {
				std::sort(arr_first, arr_sentinel);
				return true;
			}
			return false;
		}

		template<typename... Args>
		constexpr void push_sorted(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...> and std::is_nothrow_move_constructible_v<T> and (std::is_nothrow_move_assignable_v<T> or TrivialMoveAssignment)) {
			T newelem{ std::forward<Args>(args)... }; // Must construct first to use for comparisons
			if (T* iter = std::lower_bound(arr_first, arr_sentinel, newelem); iter == arr_sentinel) { // All elements are smaller
				std::construct_at(arr_sentinel++, std::move(newelem)); // Just append
			}
			else { // At least 1 bigger element. Shift that and everything after it 1 to the right, and place newelem at that bigger element's original place.
				if constexpr (TrivialMoveAssignment) {
					const size_t bytecount = static_cast<size_t>(arr_sentinel - iter) * sizeof(T); // Amount of bytes to shift
					std::memmove(iter + 1, iter, bytecount);
				}
				else {
					std::construct_at(arr_sentinel, std::move(*(arr_sentinel - 1))); // Move-construct at end from last. Guaranteed not empty vector if here.
					for (auto last = arr_sentinel - 1; last != iter; --last) { // Shift all remaining elements, including iter. Runs 0 times if iter was the last element
						std::construct_at(last, std::move(*(last - 1)));
					}
				}
				++arr_sentinel;
				std::construct_at(iter, std::move(newelem)); // Move the newelem in the place found by lower_bound
			}
		}
		template<typename... Args>
		constexpr bool try_push_sorted(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...> and std::is_nothrow_move_constructible_v<T> and (std::is_nothrow_move_assignable_v<T> or TrivialMoveAssignment)) {
			if ((arr_sentinel != alloc_sentinel) or expand()) {
				push_sorted(std::forward<Args>(args)...);
				return true;
			}
			return false;
		}

		constexpr void overwrite(const size_t pos, const T elem) noexcept(SafeCopyAssignment) requires(Small) { arr_first[pos] = elem; }
		constexpr void overwrite(const size_t pos, const T& elem) noexcept(SafeCopyAssignment) requires(!Small) { arr_first[pos] = elem; }
		constexpr void overwrite(const size_t pos, T&& elem) noexcept(SafeMoveAssignment) { arr_first[pos] = std::move(elem); }

		constexpr void pop_back() noexcept {
			--arr_sentinel;
			if constexpr (!TrivialDestruction) {
				std::destroy_at(arr_sentinel);
			}
		}

		constexpr void steal_back(const T* ptr) noexcept(SafeMoveAssignment) { // Passing an arr_first element pointer as ptr essentially erases it
			*ptr = std::move(*(--arr_sentinel));
			if constexpr (!TrivialDestruction) {
				std::destroy_at(arr_sentinel);
			}
		} 

		constexpr void erase(T* loc) noexcept(SafeMoveAssignment) {
			*loc = std::move(*(--arr_sentinel)); // Sees through trivial moves
			if constexpr (!TrivialDestruction) {
				std::destroy_at(arr_sentinel);
			}
		}
		constexpr void erase_ordered(T* loc) noexcept(SafeMoveAssignment) {
			T* first_after = loc + 1;
			if constexpr (TrivialMoveAssignment) {
				if (const i64 move_count = (arr_sentinel - first_after); move_count > 0) { // loc is not the last element
					std::memmove(loc, first_after, static_cast<size_t>(move_count) * Bytes);
				}
			}
			else {
				for (; first_after != arr_sentinel; ++loc, ++first_after) {
					*loc = std::move(*first_after);
				}
				if constexpr (!TrivialDestruction) {
					std::destroy_at(arr_sentinel - 1);
				}
			}
			--arr_sentinel;
		}

		constexpr void erase(const size_t pos) noexcept(SafeMoveAssignment) { erase(arr_first + pos); }
		constexpr void erase_ordered(const size_t pos) noexcept(SafeMoveAssignment) { erase_ordered(arr_first + pos); }

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr bool erase(const U val) noexcept(SafeMoveAssignment) {
			for (T* it = arr_first; it != arr_sentinel; ++it) {
				if (*it == val) {
					erase(it);
					return true;
				}
			}
			return false;
		}
		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr bool erase_ordered(const U val) noexcept(SafeMoveAssignment) {
			for (T* it = arr_first; it != arr_sentinel; ++it) {
				if (*it == val) {
					erase_ordered(it);
					return true;
				}
			}
			return false;
		}

		constexpr bool erase(const T& elem) noexcept(SafeMoveAssignment) requires (!Small) {
			for (T* it = arr_first; it != arr_sentinel; ++it) {
				if (*it == elem) {
					erase(it);
					return true;
				}
			}
			return false;
		}
		constexpr bool erase_ordered(const T& elem) noexcept(SafeMoveAssignment) requires (!Small) {
			for (T* it = arr_first; it != arr_sentinel; ++it) {
				if (*it == elem) {
					erase_ordered(it);
					return true;
				}
			}
			return false;
		}

		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr size_t erase_all(const U val) noexcept(SafeMoveAssignment) {
			auto old_size = arr_sentinel - arr_first;
			for (T* it = arr_first; it < arr_sentinel; ++it) {
				if (*it == val) {
					*it = std::move(*(--arr_sentinel));
					if constexpr (!TrivialDestruction) {
						std::destroy_at(arr_sentinel);
					}
				}
			}
			return static_cast<size_t>(old_size - (arr_sentinel - arr_first));
		}
		template<typename U> requires(!std::is_same_v<T*, U> and (Small or !std::is_same_v<T, U>))
		constexpr size_t erase_all_ordered(const U val) noexcept(SafeMoveAssignment or TrivialMoveAssignment) {
			T* first_del = find(val); // Skip not-to-be-deleted elements
			if (first_del == arr_sentinel) { // None to delete
				return 0;
			}
			T* move_place = first_del; // Move not-to-be-deleted ranges starting here
			if constexpr (TrivialMoveAssignment) {
				for (T* it = first_del; it != arr_sentinel; ++it) { // Find an element to delete
					if (*it == val) {
						T* first = it + 1;		// First of not-to-be-deleted elements in the range [first, last) starting as soon after it as possible
						T* last = arr_sentinel;	// Last of not-to-be-deleted elements in the range [first, last) starting as soon after it as possible
						for (; first != arr_sentinel; ++first) { // Find first
							if (*first != val) {
								last = first + 1;
								for (; last != arr_sentinel; ++last) { // Find last
									if (*last == val) {
										break;
									}
								}
								break;
							}
						}
						const size_t count = static_cast<size_t>(last - first); // Copy [first, last)
						std::memmove(move_place, first, count * Bytes);
						move_place += count;
						it = last - 1; // So it becomes last after the for ++
					}
				}
			}
			else {
				for (T* it = first_del + 1; it != arr_sentinel; ++it) { // Find an element to delete
					if (*it != val) {
						*(move_place++) = std::move(*it);
					}
				}
			}
			if constexpr (!TrivialDestruction) {
				destruct(move_place, arr_sentinel);
			}
			const size_t deleted_count = static_cast<size_t>(arr_sentinel - move_place);
			arr_sentinel = move_place;
			return deleted_count;
		}

		constexpr size_t erase_all(const T& elem) noexcept(SafeMoveAssignment) requires (!Small) {
			auto old_size = arr_sentinel - arr_first;
			for (T* it = arr_first; it < arr_sentinel; ++it) {
				if (*it == elem) {
					*it = std::move(*(--arr_sentinel));
					if constexpr (!TrivialDestruction) {
						std::destroy_at(arr_sentinel);
					}
				}
			}
			return static_cast<size_t>(old_size - (arr_sentinel - arr_first));
		}
		constexpr size_t erase_all_ordered(const T& elem) noexcept(SafeMoveAssignment or TrivialMoveAssignment) requires (!Small) {
			T* first_del = find(elem); // Skip not-to-be-deleted elements
			if (first_del == arr_sentinel) { // None to delete
				return 0;
			}
			T* move_place = first_del; // Move not-to-be-deleted ranges starting here
			if constexpr (TrivialMoveAssignment) {
				for (T* it = first_del; it != arr_sentinel; ++it) { // Find an element to delete
					if (*it == elem) {
						T* first = it + 1;		// First of not-to-be-deleted elements in the range [first, last) starting as soon after it as possible
						T* last = arr_sentinel;	// Last of not-to-be-deleted elements in the range [first, last) starting as soon after it as possible
						for (; first != arr_sentinel; ++first) { // Find first
							if (*first != elem) {
								last = first + 1;
								for (; last != arr_sentinel; ++last) { // Find last
									if (*last == elem) {
										break;
									}
								}
								break;
							}
						}
						const size_t count = static_cast<size_t>(last - first); // Copy [first, last)
						std::memmove(move_place, first, count * Bytes);
						move_place += count;
						it = last - 1; // So it becomes last after the for ++
					}
				}
			}
			else {
				for (T* it = first_del + 1; it != arr_sentinel; ++it) { // Find an element to delete
					if (*it != elem) {
						*(move_place++) = std::move(*it);
					}
				}
			}
			if constexpr (!TrivialDestruction) {
				destruct(move_place, arr_sentinel);
			}
			const size_t deleted_count = static_cast<size_t>(arr_sentinel - move_place);
			arr_sentinel = move_place;
			return deleted_count;
		}

		// Fix this for moving
		template<typename Pred>
		constexpr size_t erase_all_if(Pred pred) noexcept(SafeMoveAssignment) {
			auto old_size = arr_sentinel - arr_first;
			for (T* it = arr_first; it < arr_sentinel; ++it) {
				if (pred(*it)) {
					*it = std::move(*(--arr_sentinel));
					if constexpr (!TrivialDestruction) {
						std::destroy_at(arr_sentinel);
					}
				}
			}
			return static_cast<size_t>(old_size - (arr_sentinel - arr_first));
		}

		// [from, to)	Relative order up to before the deleted range unchanged. Relative order after will change if [to, end()) larger than [from, to).
		constexpr void erase(T* from, T* to) noexcept(SafeMoveAssignment) {
			if (const i64 del_count = (to - from); del_count > 0) { // If equal, noop.
				const i64 after_del_count = (arr_sentinel - to);
				if constexpr (TrivialMoveAssignment) {
					if (after_del_count > 0) { // to != arr_sentinel. If till end, no need to copy anything.
						const i64 move_count = (after_del_count > del_count) ? del_count : after_del_count; // Find min after_del_count to copy, to leave a sequence of deleted at the end.
						T* newend = arr_sentinel - static_cast<size_t>(move_count);
						std::memcpy(from, newend, move_count * Bytes); // Copy from the last X valid, and reduce the sentinel.
						if (!TrivialDestruction) {
							destruct(newend, arr_sentinel);
						}
						arr_sentinel = newend;
						return;
					} // Erase to end so do no copying and just go destruct.
				}
				else {
					if (del_count >= after_del_count) { // All elements after to need to be moved so can keep relative ordering for free
						for (; to != arr_sentinel; ++from, ++to) {
							*from = std::move(*to);
						}
					}
					else { // More elements after to than can fit in [from, to) so move them in reverse until that range is filled
						for (T* back = arr_sentinel - 1, *breakpoint = from + static_cast<size_t>(del_count); from != breakpoint; ++from, --back) {
							*from = std::move(*back);
						}
					}
				}
				if constexpr (!TrivialDestruction) {
					destruct(from, arr_sentinel);
				}
				arr_sentinel = from;
			}
		}
		// [from, to)	Retains relative order of remaining elements
		constexpr void erase_ordered(T* from, T* to) noexcept(SafeMoveAssignment) {
			if constexpr (TrivialMoveAssignment) {
				if (from != to) {
					if (const size_t move_count = static_cast<size_t>(arr_sentinel - to); move_count > 0) { // Find how many elements there are after the deleted range
						std::memmove(from, to, move_count * Bytes); // Move them to the start of that range
						if constexpr (!TrivialDestruction) {
							destruct((from + move_count), arr_sentinel);
						}
						arr_sentinel = (from + move_count); // Update sentinel
						return;
					}
				} // Erase to end so do no copying and just go destruct.
			}
			else {
				for (; to != arr_sentinel; ++from, ++to) {
					*from = std::move(*to); // Move from the last to delete, to the first to delete, in order.
				}
			}
			if constexpr (!TrivialDestruction) {
				destruct(from, arr_sentinel);
			}
			arr_sentinel = from;
		}

		constexpr void trim(T* from) noexcept {
			if constexpr (!TrivialDestruction) {
				destruct(from, arr_sentinel);
			}
			arr_sentinel = from;
		}
		constexpr void trim(const size_t from_pos) noexcept { trim(arr_first + from_pos); }

		// Will not free memory
		constexpr void clear() noexcept {
			if constexpr (!TrivialDestruction) {
				destruct_all();
			}
			arr_sentinel = arr_first;
		}

		bool serialize(SKSE::SerializationInterface& intfc) const requires(TrivialCopyConstruction) {
			static constexpr u32 max_write_count = (UINT32_MAX / static_cast<u32>(sizeof(T))); // Max UINT_MAX bytes in a record
			const u32 elem_count = static_cast<u32>(std::clamp<i64>((arr_sentinel - arr_first), 0, max_write_count));
			if (!intfc.WriteRecordData(elem_count)) {
				Log::Critical("Failed to serialize vector size!"sv);
				return false;
			}
			if (elem_count > 0) {
				if (!intfc.WriteRecordData(arr_first, elem_count * static_cast<u32>(sizeof(T)))) { // # of elements * # of bytes in element
					Log::Critical("Failed to serialize vector!"sv);
					return false;
				}
			}
			return true;
		}
		bool deserialize(SKSE::SerializationInterface& intfc) requires(TrivialCopyConstruction) {
			u32 elem_count{ 0 };
			if (!intfc.ReadRecordData(elem_count)) {
				Log::Critical("Failed to read vector size!"sv);
				return false;
			}
			clear();
			if (elem_count > 0) {
				if (!reserve(elem_count)) {
					Log::Critical("Failed to allocate <{}> bytes to deserialize vector in!"sv, elem_count * static_cast<u32>(sizeof(T)));
					return false;
				}
				if (!intfc.ReadRecordData(arr_first, elem_count * static_cast<u32>(sizeof(T)))) { // # of elements * # of bytes in element
					Log::Error("Failed to deserialize vector data!"sv);
					return false;
				}
			}
			return true;
		}


	private:
		T* arr_first{ nullptr };
		T* arr_sentinel{ nullptr };
		T* alloc_sentinel{ nullptr };

		constexpr void purge_self() noexcept {
			destruct_all();
			free_self();
			arr_first = nullptr;
			arr_sentinel = nullptr;
			alloc_sentinel = nullptr;
		}

		constexpr bool copy_init(const T* source, const size_t size) noexcept(SafeCopyConstruction or TrivialCopyConstruction) {
			if (arr_first = allocate(size + 1); arr_first) { // Allocate exactly as many as needed. Ignore source capacity (if applicable).
				arr_sentinel = arr_first + size;
				alloc_sentinel = arr_sentinel;
				if constexpr (TrivialCopyConstruction) {
					std::memcpy(arr_first, source, size * Bytes);
				}
				else {
					for (T* it = arr_first; it != arr_sentinel; ++it, ++source) {
						std::construct_at(it, *source); // Copy-construct each element
					}
				}
				return true;
			}
			return false;
		}
		// [from, to)
		constexpr void copy_assign(const T* first, const T* to) noexcept((SafeCopyConstruction and SafeCopyAssignment) or (TrivialCopyConstruction and TrivialCopyAssignment)) {
			if (!first) { // rhs null
				destruct_all();
				this->arr_sentinel = this->arr_first;
			}

			if (const size_t rhs_size = static_cast<size_t>(to - first); this->arr_first and (rhs_size <= capacity())) { // Fits in our storage
				if constexpr (TrivialCopyConstruction and TrivialCopyAssignment) {
					std::memcpy(this->arr_first, first, rhs_size * Bytes);
					this->arr_sentinel = this->arr_first + rhs_size; // Update end
				}
				else {
					const size_t sz = size();
					const size_t first_del = std::clamp<size_t>(rhs_size, 0, sz);
					destruct(this->arr_first + first_del, this->arr_sentinel); // Destroy those that wouldn't get copied over
					this->arr_sentinel = this->arr_first + rhs_size; // Update end
					T* it = this->arr_first;
					T* source = first;
					for (T* const last = this->arr_first + first_del; it != last; ++it, ++source) { // Up to, and not including, the first one deleted
						*it = *source; // Copy-assign the existing ones
					}
					for (; it != this->arr_sentinel; ++it, ++source) {
						std::construct_at(it, *source); // Copy-construct in place the new ones
					}
				}
			}
			else { // We're null or it doesn't fit in our storage
				if (T* newloc = allocate(rhs_size + 1); newloc) {
					purge_self(); // Fully clear self
					this->arr_first = newloc; // Yoink new memory
					this->arr_sentinel = this->arr_first + rhs_size; // Update sentinels
					this->alloc_sentinel = this->arr_sentinel;

					if constexpr (TrivialCopyConstruction) {
						std::memcpy(this->arr_first, first, rhs_size * Bytes);
					}
					else {
						for (T* it = this->arr_first, *source = first; it != this->arr_sentinel; ++it, ++source) {
							std::construct_at(it, *source); // Copy-construct each element
						}
					}
				}
				else { // Failed to allocate
					SKSE::stl::report_and_fail("out of memory"sv);
				}
			}
		}
		// [from, to)
		constexpr void default_construct(T* from, T* to) noexcept(SafeDefaultConstruction) {
			if constexpr (TrivialDefaultConstruction) {
				std::memset(from, 0, (to - from) * Bytes);
			}
			else {
				for (; from != to; ++from) {
					std::construct_at(from);
				}
			}
		}
		// [from, to)
		constexpr void destruct(T* from, T* to) noexcept {
			if constexpr (!TrivialDestruction) { // For good measure
				while (from != to) {
					std::destroy_at(from++);
				}
			}
		}

		constexpr void destruct_all() noexcept {
			if constexpr (!TrivialDestruction) { // For good measure
				destruct(arr_first, arr_sentinel);
			}
		}

		constexpr bool expand() noexcept(SafeMoveConstruction) {
			const size_t cur_cap = static_cast<size_t>(alloc_sentinel - arr_first);
			const size_t new_cap = std::bit_ceil(cur_cap + 1); // +1 so that it correctly grows even if cur_cap is a power of 2
			return expand_impl(new_cap);
		}
		constexpr bool expand(size_t needed_elem_count) noexcept(SafeMoveConstruction) {
			return expand_impl(std::bit_ceil(needed_elem_count)); // If needed_elem_count not power of 2, grow to the smallest power of 2 larger than needed_elem_count.
		}
		constexpr bool expand_impl(size_t needed_elem_count) noexcept(SafeMoveConstruction or TrivialMoveConstruction) {
			if constexpr (TrivialMoveConstruction) {
				if (arr_first) {
					const auto size = (arr_sentinel - arr_first);
					if (T* newloc = reallocate_self(needed_elem_count + 1); newloc) {
						arr_first = newloc;
						arr_sentinel = arr_first + size;
						alloc_sentinel = arr_first + needed_elem_count;
						return true;
					}
				}
				else { // No allocated memory
					if (T* newloc = allocate(needed_elem_count + 1); newloc) {
						arr_first = newloc;
						arr_sentinel = arr_first;
						alloc_sentinel = arr_first + needed_elem_count;
						return true;
					}
				}
			}
			else {
				if (T* newloc = allocate(needed_elem_count + 1); newloc) {
					T* it = newloc;
					T* const last = newloc + (arr_sentinel - arr_first);
					T* old_first = arr_first;
					while (it != last) {
						std::construct_at(it++, std::move(*(old_first++))); // Move-construct new elements
					}
					if constexpr (!TrivialDestruction) {
						destruct(arr_first, arr_sentinel); // Destruct old elements
					}
					free_self(); // Free old memory
					arr_first = newloc; // Yoink new memory
					arr_sentinel = last; // Update last
					alloc_sentinel = arr_first + needed_elem_count; // Update capacity
					return true;
				}
			}
			return false;
		}


		constexpr T* allocate(const size_t size) noexcept { return static_cast<T*>(_aligned_malloc(size * Bytes, Alignment)); }
		constexpr T* reallocate_self(const size_t newcapacity) noexcept { return static_cast<T*>(_aligned_realloc(arr_first, newcapacity * Bytes, Alignment)); }
		constexpr void free_self() noexcept { _aligned_free(arr_first); }

	};


	template<typename T>
	[[nodiscard]] lazy_vector<T> DumbUnpack(const RE::BSScript::Variable* a_src) {
		lazy_vector<T> container{};
		if (!a_src->IsNoneObject()) {
			if (auto array = a_src->GetArray(); array) {
				container.reserve(array->size());
				for (const auto& elem : *array) {
					container.append(elem.Unpack<T>());
				}
			}
		}

		return container;
	}
	

	lazy_vector<string> Split(const string& str, const string& delim) noexcept;

	lazy_vector<u32> StrToU32(const lazy_vector<string>& vec) noexcept;
	lazy_vector<i32> StrToS32(const lazy_vector<string>& vec) noexcept;
	lazy_vector<float> StrToFl(const lazy_vector<string>& vec) noexcept;


}
