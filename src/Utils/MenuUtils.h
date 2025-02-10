#pragma once
#include "Common.h"
#include <Types/LazyVector.h>


namespace MenuUtils {
	using namespace LazyVector;

	
	void NotifyMenuChange(const char* name, const bool opened) noexcept;
	void NotifyModEvent(const SKSE::ModCallbackEvent* event) noexcept;
	

	// The menu waiters sleep while setting up and while waiting for result so only call on a self-managed thread

	enum alignment : u64 {
		left = 0,
		center = 1,
		right = 2
	};

	i32 WaitEVMMBV(const string& msg, const lazy_vector<string>& btns, const alignment align = center) noexcept;

	i32 WaitEVMMBBL(const string& msg, const lazy_vector<string>& btns, const alignment align = center) noexcept;

	struct slider_params {
		struct values {
			string text;
			string info;
			float min;
			float max;
			float start;
			float step;
			u32 decimals;
			char pad[4];
		};
		static_assert(std::is_aggregate_v<values> and (sizeof(values) == 88));

		constexpr slider_params() noexcept = default;
		constexpr slider_params(const slider_params&) = default;
		constexpr slider_params(slider_params&&) noexcept = default;
		constexpr slider_params& operator=(const slider_params&) = default;
		constexpr slider_params& operator=(slider_params&&) noexcept = default;
		constexpr ~slider_params() noexcept = default;

		constexpr slider_params(const string& init) : val(init) {}
		constexpr slider_params(string&& init) noexcept : val(std::move(init)) {}
		constexpr slider_params& operator=(const string& init) { val = init; return *this; }
		constexpr slider_params& operator=(string&& init) noexcept { val = std::move(init); return *this; }

		constexpr slider_params(const values& init)
			: val("||"+init.text+"||"+init.info+"||"+to_string(init.min)+"||"+to_string(init.max)+"||"+to_string(init.start)+"||"+to_string(init.step)+"||"+to_string(init.decimals))
		{}
		constexpr slider_params& operator=(const values & init) {
			val = ("||"+init.text+"||"+init.info+"||"+to_string(init.min)+"||"+to_string(init.max)+"||"+to_string(init.start)+"||"+to_string(init.step)+"||"+to_string(init.decimals));
			return *this;
		}

		constexpr string str() const noexcept { return val; }
		constexpr const string& str_cref() const noexcept { return val; }
		constexpr const char* c_str() const noexcept { return val.c_str(); }


	private:
		string val{};
	};
	static_assert(sizeof(slider_params) == 32);

	lazy_vector<string> WaitEVSM(const array<string, 3>& tac, const lazy_vector<slider_params>& params) noexcept;
	lazy_vector<u32> WaitEVSM_U32(const array<string, 3>& tac, const lazy_vector<slider_params>& params) noexcept;
	lazy_vector<i32> WaitEVSM_S32(const array<string, 3>& tac, const lazy_vector<slider_params>& params) noexcept;
	lazy_vector<float> WaitEVSM_F(const array<string, 3>& tac, const lazy_vector<slider_params>& params) noexcept;
	
}
