#pragma once
#include "Common.h"


namespace ExtraKeywords {

	namespace Data {
		constexpr u32 SerializationType{ 'EXKW' };
		constexpr u32 SerializationVersion{ 1 };

		bool Has(RE::TESForm* form, RE::BGSKeyword* keyword);
		std::deque<bool> Has(RE::TESForm* form, vector<RE::BGSKeyword*> keywords);
		std::deque<bool> Has(RE::TESForm* form, RE::BGSListForm* formlist);
		std::deque<bool> CrossKeywords(RE::TESForm* form, RE::BGSListForm* formlist);
		bool Add(RE::TESForm* form, RE::BGSKeyword* keyword);
		bool Remove(RE::TESForm* form, RE::BGSKeyword* keyword);


		u64 ClearInvalids();


		bool Save(SKSE::SerializationInterface& intfc);
		bool Load(SKSE::SerializationInterface& intfc); // This applies them as it reads them
		void Revert();

	}


}


















