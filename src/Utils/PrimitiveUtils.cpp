#include "PrimitiveUtils.h"


namespace PrimitiveUtils {

	string addr_str(void* ptr) {
		std::stringstream ss;
		ss << ptr;
		return ss.str();
	}

	string bit_str(void* ptr, u64 byteNum) {
		auto ptrC = reinterpret_cast<unsigned char*>(ptr);
		string str{};
		for (u64 i = 0; i < byteNum; ++i) {
			if (i > 0 && i % 8 == 0) {
				str += "| ";
			}
			for (int k = 0; k < sizeof(unsigned char) * 8; ++k) {
				if (ptrC[i] & (1 << k))
					str += '1';
				else
					str += '0';
			}
			str += ' ';
		}
		if (str.length() > 0)
			str.pop_back();
		return str;
	}

	string bool_str(bool flag) { static const array<string, 2u> boolStrings{ "false", "true" }; return boolStrings[flag]; }

	string u32_str(u32 num) {
		string numStr(reinterpret_cast<char*>(&num));
		std::reverse(numStr.begin(), numStr.end()); // Without this a reversed string is actually returned, eg for num=1129203015, which is 'CNEG', numStr.c_str() would be "GENC".
		return numStr;
	}

	string u32_xstr(const u32 num, const u64 length, const char pad) {
		std::stringstream ss{};
		if (length > 0) { ss << std::setw(length) << std::setfill(pad) << std::uppercase << std::hex << num; }
		else { ss << std::uppercase << std::hex << num; }
		return ss.str();
	}




}

