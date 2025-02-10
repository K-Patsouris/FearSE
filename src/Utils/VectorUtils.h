#pragma once
#include "Common.h"


namespace VectorUtils {

	template<typename T> requires(sizeof(T) <= 8 && std::is_trivially_copyable_v<T>)
	bool Contains(const vector<T>& vec, const T elem) noexcept { return std::find(vec.begin(), vec.end(), elem) != vec.end(); }
	template<typename T>
	bool Contains(const vector<T>& vec, const T& elem) noexcept { return std::find(vec.begin(), vec.end(), elem) != vec.end(); }

	template<typename T> requires(sizeof(T) <= 8 && std::is_trivially_copyable_v<T>)
	vector<T>::const_iterator Find(const vector<T>& vec, const T elem) { return std::find(vec.begin(), vec.end(), elem); }
	template<typename T>
	vector<T>::const_iterator Find(const vector<T>& vec, const T& elem) { return std::find(vec.begin(), vec.end(), elem); }
	template<typename T> requires(sizeof(T) <= 8 && std::is_trivially_copyable_v<T>)
	vector<T>::iterator Find(vector<T>& vec, const T elem) { return std::find(vec.begin(), vec.end(), elem); }
	template<typename T>
	vector<T>::iterator Find(vector<T>& vec, const T& elem) { return std::find(vec.begin(), vec.end(), elem); }

	template <typename T> requires(sizeof(T) <= 8 && std::is_trivially_copyable_v<T>)
	vector<T>::const_iterator BinaryFind(const vector<T>& vec, const T val) {
		auto it = std::lower_bound(vec.begin(), vec.end(), val);
		return (it != vec.end() && *it == val) ? it : vec.end();
	}
	template <typename T>
	vector<T>::const_iterator BinaryFind(const vector<T>& vec, const T& val) {
		auto it = std::lower_bound(vec.begin(), vec.end(), val);
		return (it != vec.end() && *it == val) ? it : vec.end();
	}
	template <typename T> requires(sizeof(T) <= 8 && std::is_trivially_copyable_v<T>)
	vector<T>::iterator BinaryFind(vector<T>& vec, const T val) {
		auto it = std::lower_bound(vec.begin(), vec.end(), val);
		return (it != vec.end() && *it == val) ? it : vec.end();
	}
	template <typename T>
	vector<T>::iterator BinaryFind(vector<T>& vec, const T& val) {
		auto it = std::lower_bound(vec.begin(), vec.end(), val);
		return (it != vec.end() && *it == val) ? it : vec.end();
	}



	vector<u32> StrToU32(const vector<string>& vec) noexcept;
	vector<i32> StrToS32(const vector<string>& vec) noexcept;
	vector<float> StrToFl(const vector<string>& vec) noexcept;

	// Vector utils
	template<typename T>
	[[nodiscard]] constexpr bool Reserve(vector<T>& vec, const size_t newcapacity) noexcept {
		try { vec.reserve(newcapacity); }
		catch (...) { return false; }
		return true;
	}
	template<typename T>
	[[nodiscard]] constexpr bool Resize(vector<T>& vec, const size_t newsize) noexcept {
		try { vec.resize(newsize); }
		catch (...) { return false; }
		return true;
	}
	template<typename T>
	[[nodiscard]] constexpr bool PushBack(vector<T>& vec, const T& val) noexcept {
		try { vec.push_back(val); }
		catch (...) { return false; }
		return true;
	}

	template<typename T>
	constexpr bool InsertSort(vector<T>& vec, const T& val) noexcept {
		if (PushBack(vec, val)) {
			std::sort(vec.begin(), vec.end());
			return true;
		}
		return false;
	}

	template <typename T, typename U>
	constexpr vector<T>::iterator Find(vector<T>& vec, const U val) noexcept {
		for (auto it = vec.begin(), end = vec.end(); it != end; ++it) {
			if (*it == val) {
				return it;
			}
		}
		return vec.end();
	}
	template <typename T, typename U>
	constexpr vector<T>::const_iterator Find(const vector<T>& vec, const U val) noexcept {
		for (auto it = vec.cbegin(), end = vec.cend(); it != end; ++it) {
			if (*it == val) {
				return it;
			}
		}
		return vec.cend();
	}

	template <typename T, typename U>
	constexpr bool Contains(const vector<T>& vec, const U val) noexcept {
		for (const auto& elem : vec) {
			if (elem == val) {
				return true;
			}
		}
		return false;
	}

	template <typename T, typename U> requires (std::is_integral_v<T> || std::is_floating_point_v<T>) && (std::is_integral_v<U> || std::is_floating_point_v<U>)
	constexpr vector<U> StaticCast(const vector<T>& vec) noexcept {
		try {
			vector<U> result{};
			result.reserve(vec.size());
			for (const auto elem : vec) {
				result.push_back(static_cast<U>(elem));
			}
			return result;
		}
		catch (...) { return {}; }
	}


}
