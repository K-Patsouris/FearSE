#include "StringUtils.h"
#include <cmath>


namespace StringUtils {

	string GetTimeString(const float targetTime) {
		auto cal = RE::Calendar::GetSingleton();
		if (!cal)
			return "";

		float diff = targetTime - cal->GetCurrentGameTime();		// The most accurate way to find the date is via relevance to current time

		float baseF;												// Dummy helper to store whole parts from std::modf
		float targetDec = std::modf(targetTime, &baseF);			// Keep decimal of targetTime
		std::modf(diff, &baseF);									// Split diff into decimal (discarded) and whole (&base)
		i32 diffBase = static_cast<i32>(baseF);	// Convert whole of diff to integer

		if (targetDec < 0.0f) {	// targetTime is a time before current
			diffBase--;			// An extra day back from time zero
			targetDec += 1.0f;	// and this so we don't actually go back a full day but just the hours needed
		}

		i32 curY = static_cast<i32>(cal->GetYear());	// 0+
		i32 curM = static_cast<i32>(cal->GetMonth());	// 1-12
		i32 curD = static_cast<i32>(cal->GetDay());		// 1-31

		if ((curY < 0) || (curM < 1 || curM > 12) || (curD < 1 || curD > 31))
			return "GlobalVariable data out of sensible bounds!";

		i32 targetDays = (curY * 365) + curD;
		for (i32 i = curM - 1; i >= 0; i--)
			targetDays += RE::Calendar::DAYS_IN_MONTH[i];

		targetDays += diffBase;
		if (targetDays < 0)
			return "Requested time before the start of time!";

		i32 year = -1;
		i32 month = -1;
		i32 day = -1;
		i32 hour = -1;
		i32 minute = -1;

		year = targetDays / 365;
		targetDays -= (targetDays / 365) * 365;

		for (i32 i = 0; i < 12; i++) {
			if (RE::Calendar::DAYS_IN_MONTH[i] >= targetDays) {
				month = i + 1;
				break;
			}
			else {
				targetDays -= RE::Calendar::DAYS_IN_MONTH[i];
			}
		}

		day = targetDays;

		minute = static_cast<i32>(std::modf((targetDec * 24.0f), &baseF) * 60.0f);
		hour = static_cast<i32>(baseF);

		string result = to_string(day) + "/" + to_string(month) + "/" + to_string(year) + " ";
		string hoursStr = to_string(hour);
		result += (hoursStr.length() == 1) ? "0" + hoursStr + ":" : hoursStr + ":";
		string minuteStr = to_string(minute);
		result += (minuteStr.length() == 1) ? "0" + minuteStr : minuteStr;

		return result;
	}

	vector<string> Split(const string& str, const string& delim) {
		if (str.length() <= 0u || delim.length() <= 0u)
			return vector<string>{ str };

		vector<string> result{};

		u64 pos{ 0u };
		u64 fnd{ str.find(delim, pos) };
		while (fnd != str.npos) {
			result.push_back(str.substr(pos, fnd - pos));
			pos = fnd + delim.length(); // .find guarantees this is <= str.size()
			fnd = str.find(delim, pos);
		}
		if (pos < (str.length() - 1)) { // str has characters left after last delim
			result.push_back(str.substr(pos));
		}

		return result;
	}

	string Join(const vector<string>& vec, const string& delim) {
		if (vec.empty()) {
			return string{};
		}
		string result{ vec.front() };
		for (u64 i = 1; i < vec.size(); ++i) {
			result += delim + vec[i];
		}
		return result;
	}

	string ObjectivePronouns(const RE::Actor* act) {
		// if (playerSeparate && act->formID == 0x14u) // Player
		// 	return player1st ? "me" : "you";
		static const string pron[4]{ "them", "him", "her", "it" };
		u32 idx = 0;
		if (const auto base = act->GetActorBase(); base) {
			idx = base->GetSex() + 1; // So "kNone" becomes 0 from u32 max zzz
		}
		return pron[idx];
	}

	string FlToStr(const float flt, i64 precision) {
		std::stringstream ss;
		ss << std::fixed << std::setprecision(precision) << flt;
		const std::string s = ss.str();
		return s;
	}
	string DaysToDur(const float days) {
		if (days <= 0.0f) {
			return string{ "0d" };
		}
		u64 ys = 0;
		u64 ws = 0;
		u64 ds = 0;
		u32 hrs = 0;
		u32 mins = 0;

		float wholeF = 0.0f;
		float frac = std::modf(days, &wholeF) * 1440.0f;
		u64 whole = static_cast<u64>(wholeF);

		ys = whole / 365;
		whole -= ys * 365;

		ws = whole / 7;
		whole -= ws * 7;
		
		ds = whole;

		hrs = static_cast<u32>(frac) / 60;
		mins = static_cast<u32>(frac) - (hrs * 60);
		
		string result{};
		if (ys > 0) result += to_string(ys) + 'y';
		if (ws > 0) result += to_string(ws) + 'w';
		if (ds > 0) result += to_string(ds) + 'd';
		if (hrs > 0) result += to_string(hrs) + 'h';
		if (mins > 0) result += to_string(mins) + "min";
		return result;
	}
	string HrsToDur(const float hrs) {
		if (hrs <= 0.0f) {
			return string{ "0h" };
		}
		if (hrs > 24.0f) {
			return DaysToDur(hrs / 24.0f);
		}

		float hrsW = 0.0f;

		if (const u64 mins = static_cast<u64>((std::modf(hrs, &hrsW) * 1000.0f) / 16.0f); mins > 0) {
			return string{ to_string(static_cast<u64>(hrsW)) + 'h' + to_string(mins) + 'm' };
		}
		return string{ to_string(static_cast<u64>(hrsW)) + 'h' };
	}
}

