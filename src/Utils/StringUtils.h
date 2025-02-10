#pragma once
#include "Common.h"


namespace StringUtils {

	string GetTimeString(const float targetTime);
	
	vector<string> Split(const string& str, const string& delim);
	string Join(const vector<string>& vec, const string& delim = "\n");

	string ObjectivePronouns(const RE::Actor* act);

	// Case insensitive string comparison
	constexpr bool StrICmp(const char* c1, const char* c2) noexcept {
		/*static constexpr array<u8, 256> lowers {
		'\0', 1,    2,    3,    4,    5,    6,    7,
		8,    9,    10,   11,   12,   13,   14,   15,
		16,   17,   18,   19,   20,   21,   22,   23,
		24,   25,   26,   27,   28,   29,   30,   31,
		' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
		'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
		'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
		'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
		'@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
		'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
		'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
		'x',  'y',  'z',  '[',  '\\', ']',  '^',  '_',
		'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
		'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
		'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
		'x',  'y',  'z',  '{',  '|',  '}',  '~',  127,
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199,
		200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215,
		216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255,
		};

		while (*c1 and lowers[std::bit_cast<u8>(*c1)] == lowers[std::bit_cast<u8>(*c2)]) { // std::bit_cast() for proper indexing. Two's complement is mandated.
		++c1;
		++c2;
		}// */

		while (*c1  and ((std::bit_cast<u8>(*c1) bitor static_cast<u8>((static_cast<u8>(std::bit_cast<u8>(*c1) - 65) <= 25) << 5))
						 == (std::bit_cast<u8>(*c2) bitor static_cast<u8>((static_cast<u8>(std::bit_cast<u8>(*c2) - 65) <= 25) << 5)))
			   )
		{
			++c1;
			++c2;
		}// */

		 // Array is 10x faster than std::tolower(). _stricmp() is about half as fast as array. Bit masking and is ~40% slower than array but still faster than anything else.
		 // So, array is the fastest, but it's an extra 256 bytes in the binary, and needs to read a couple of cachelines.
		 // 10k comparisons of random strings of length 5-15 (with ~<5 matches) takes ~20us with array and ~28us with bits. 10k cycles is 30us, so that's more than 1 string comparison per cycle.
		 // It's impossible to measure, but for low number of string comparisons, which is almost always what's needed, the bits should far outpace the array because they don't need memory loads.
		 // Meaning, just use the bits. 

		 // Negative if c1 shorter, positive if c1 longer, 0 if equal length and same characters case insensitively. Negative/positive when non matching chars found, depending on each char's int representation.
		return (*c1 - *c2) == 0;
	}

	string FlToStr(const float flt, i64 precision);
	string DaysToDur(const float days);
	string HrsToDur(const float hrs);

}
