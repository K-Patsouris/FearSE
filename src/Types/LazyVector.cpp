#include "LazyVector.h"


namespace LazyVector {


	lazy_vector<string> Split(const string& str, const string& delim) noexcept {
		if (str.empty() or delim.empty()) {
			return lazy_vector<string>{ str };
		}
		lazy_vector<string> result{}; 
		u64 pos{ 0 };
		u64 fnd{ str.find(delim, pos) };
		while (fnd != str.npos) {
			if (!result.try_append(str.substr(pos, fnd - pos))) {
				return result;
			}
			pos = fnd + delim.length(); // .find guarantees this is <= str.size()
			fnd = str.find(delim, pos);
		}
		if (pos < (str.length() - 1)) { // str has characters left after last delim
			result.try_append(str.substr(pos));
		}
		return result;
	}


	lazy_vector<u32> StrToU32(const lazy_vector<string>& vec) noexcept {
		if (lazy_vector<u32> vals{}; vals.reserve(vec.size())) {
			for (const auto& str : vec) {
				u32 parse;
				try {
					parse = std::stoul(str.c_str(), nullptr);
				}
				catch (const std::invalid_argument& ex) {
					Log::Error("LazyVector::StrToU32: Caught invalid_argument({}) while trying to parse <{}> to a uint32! Fell back to 0."sv, ex.what(), str);
					parse = 0;
				}
				vals.append(parse);
			}
			return vals;
		}
		return {};
	}
	lazy_vector<i32> StrToS32(const lazy_vector<string>& vec) noexcept {
		if (lazy_vector<i32> vals{}; vals.reserve(vec.size())) {
			for (const auto& str : vec) {
				i32 parse;
				try {
					parse = std::stol(str.c_str(), nullptr);
				}
				catch (const std::invalid_argument& ex) {
					Log::Error("LazyVector::StrToU32: Caught invalid_argument({}) while trying to parse <{}> to a int32! Fell back to 0."sv, ex.what(), str);
					parse = 0;
				}
				vals.append(parse);
			}
			return vals;
		}
		return {};
	}
	lazy_vector<float> StrToFl(const lazy_vector<string>& vec) noexcept {
		if (lazy_vector<float> vals{}; vals.reserve(vec.size())) {
			for (const auto& str : vec) {
				float parse;
				try {
					parse = std::stof(str.c_str(), nullptr);
				}
				catch (const std::invalid_argument& ex) {
					Log::Error("LazyVector::StrToFl: Caught invalid_argument({}) while trying to parse <{}> to a float! Fell back to 0.0f."sv, ex.what(), str);
					parse = 0;
				}
				vals.append(parse);
			}
			return vals;
		}
		return {};
	}


}

