#include "VectorUtils.h"
#include "Logger.h"


namespace VectorUtils {
	
	vector<u32> StrToU32(const vector<string>& vec) noexcept {
		try {
			vector<u32> vals{};
			vals.reserve(vec.size());
			for (const auto& str : vec) {
				u32 parse;
				try {
					parse = std::stoul(str.c_str(), nullptr);
				}
				catch (const std::invalid_argument& ex) {
					Log::Error("VectorUtils::StrToU32: Caught invalid_argument({}) while trying to parse <{}> to a uint32! Fell back to 0."sv, ex.what(), str);
					parse = 0;
				}
				vals.push_back(parse);
			}
			return vals;
		}
		catch (...) { return {}; }
	}
	vector<i32> StrToS32(const vector<string>& vec) noexcept {
		try {
			vector<i32> vals{};
			vals.reserve(vec.size());
			for (const auto& str : vec) {
				i32 parse;
				try {
					parse = std::stol(str.c_str(), nullptr);
				}
				catch (const std::invalid_argument& ex) {
					Log::Error("VectorUtils::StrToS32: Caught invalid_argument({}) while trying to parse <{}> to a int32! Fell back to 0."sv, ex.what(), str);
					parse = 0;
				}
				vals.push_back(parse);
			}
			return vals;
		}
		catch (...) { return {}; }
	}
	vector<float> StrToFl(const vector<string>& vec) noexcept {
		try {
			vector<float> vals{};
			vals.reserve(vec.size());
			for (const auto& str : vec) {
				float parse;
				try {
					parse = std::stof(str.c_str(), nullptr);
				}
				catch (const std::invalid_argument& ex) {
					Log::Error("VectorUtils::StrToFl: Caught invalid_argument({}) while trying to parse <{}> to a float! Fell back to 0.0f."sv, ex.what(), str);
					parse = 0.0f;
				}
				vals.push_back(parse);
			}
			return vals;
		}
		catch (...) { return {}; }
	}

}

