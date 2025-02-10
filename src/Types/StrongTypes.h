#pragma once
#include "Common.h"

namespace StrongTypes {

	// It DOES seem to work, if needed
	template <typename T> requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
	struct num {
	public:
		constexpr num(T i = 0) noexcept : n(i) {}
		constexpr operator T& () noexcept { return n; }
		constexpr T* operator &() noexcept { return &n; }
	private:
		T n;
	};
	static_assert(std::is_trivially_copyable_v<num<i32>>);
	static_assert(std::is_nothrow_default_constructible_v<num<i32>>);
	static_assert(std::is_nothrow_copy_constructible_v<num<i32>>);
	static_assert(std::is_nothrow_move_constructible_v<num<i32>>);
	static_assert(std::is_nothrow_copy_assignable_v<num<i32>>);
	static_assert(std::is_nothrow_move_assignable_v<num<i32>>);
	static_assert(std::is_nothrow_swappable_v<num<i32>>);
	static_assert(std::is_nothrow_destructible_v<num<i32>>);



}
