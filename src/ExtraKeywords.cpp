#include "ExtraKeywords.h"
#include "Logger.h"
#include "Utils/PrimitiveUtils.h"

namespace ExtraKeywords {
	
	// Ancient feature. Not used in-game but kept as is for reference.

	namespace Data {
		using Locker = std::lock_guard<std::mutex>;
		std::mutex lock;
		using Map = map<RE::FormID, std::set<RE::FormID>>; // Key:formID of forms to add keywords to		Value:set of the formIDs of all the keywords to add to Key's form
		using Iterator = Map::iterator;
		Map data;


		// Non-lockers, for internal use
		u64 ClearInvalidsImpl() {
			u64 count = 0;
			for (Iterator iter = data.begin(), end = data.end(); iter != end;) {
				if (iter->second.empty()) { // No data for this formiD
					iter = data.erase(iter);
					++count;
				}
				else if (const auto kwdForm = RE::TESForm::LookupByID<RE::BGSKeywordForm>(iter->first); !kwdForm || kwdForm->numKeywords == 0) { // formID can't produce a form with keywords
					iter = data.erase(iter);
					++count;
				}
				else {	
					++iter;
				}
			}
			return count;
		}


		// Interface implementation		All these take the lock
		bool Has(RE::TESForm* form, RE::BGSKeyword* keyword) {
				if (!form || !keyword)
					return false;
				Locker locker(lock);
				Iterator iter = data.find(form->formID);
				if (iter != data.end())
					return iter->second.contains(keyword->formID);
				return false;
			}
		std::deque<bool> Has(RE::TESForm* form, vector<RE::BGSKeyword*> keywords) {
			if (!form || keywords.size() <= 0)
				return std::deque<bool>(keywords.size());
			Locker locker(lock);
			Iterator iter = data.find(form->formID);
			if (iter == data.end())
				return std::deque<bool>(keywords.size());
			std::deque<bool> result(keywords.size(), false);
			for (u64 i = 0; i < result.size(); ++i) {
				if (keywords[i]) [[likely]]
					result[i] = iter->second.contains(keywords[i]->formID);
			}
			return result;
		}
		std::deque<bool> Has(RE::TESForm* form, RE::BGSListForm* formlist) {
			if (!form || !formlist || formlist->forms.size() <= 0)
				return std::deque<bool>{};
			Locker locker(lock);
			Iterator iter = data.find(form->formID);
			if (iter == data.end())
				return std::deque<bool>{};
			std::deque<bool> result(formlist->forms.size(), false);
			for (u32 i = 0; i < result.size(); ++i) {
				if (formlist->forms[i]) [[likely]]
					result[i] = iter->second.contains(formlist->forms[i]->formID);
			}
			return result;
		}
		std::deque<bool> CrossKeywords(RE::TESForm* form, RE::BGSListForm* formlist) {
			if (!form || !formlist || formlist->forms.empty() || formlist->forms.size() > INT_MAX) // Return shouldn't exceed UINT_MAX in size cause Papyrus, so input should be at most INT_MAX size
				return std::deque<bool>{};
			const auto kwdForm = form->As<RE::BGSKeywordForm>();
			if (!kwdForm)
				return std::deque<bool>{};
			// [0 - size-1]     Toggle state:		has keyword
			// [size - 2*size-1] Option flag:		has the keyword added and not natively
			u32 size = formlist->forms.size();
			std::deque<bool> result(2 * size, false);
			for (u32 i = 0; i < size; ++i) { // Check has keywords
				if (formlist->forms[i]) [[likely]]
					result[i] = kwdForm->HasKeywordID(formlist->forms[i]->formID);
			}
			Locker locker(lock); // Don't need the lock earlier
			Iterator iter = data.find(form->formID);
			if (iter == data.end())
				return result; // Checked for keywords and there is no data of added ones so just return
			for (u32 i = 0; i < size; ++i) { // Find if it has keywords natively
				if (!result[i]) // Doesn't have the keyword, skip checks
					continue;
				if (formlist->forms[i]) // Don't access nullptr
					result[static_cast<u64>(i) + size] = iter->second.contains(formlist->forms[i]->formID); // Found in data so it does not have the keyword natively
			}


			return result;
		}
		bool Add(RE::TESForm* form, RE::BGSKeyword* keyword) {
			if (!form || !keyword)
				return false;

			auto kwdForm = form->As<RE::BGSKeywordForm>();
			if (!kwdForm || kwdForm->HasKeyword(keyword))
				return false;

			Locker locker(lock);
			data[form->formID].insert(keyword->formID); // Attempt to add to set
			if (data[form->formID].contains(keyword->formID) && kwdForm->AddKeyword(keyword))
				return true; // Exists in set and got added to form successfully
			else // Either insertion in set or addition of keyword failed. Being here means that the keyword has NOT been added to the form.
				data[form->formID].erase(keyword->formID); // So just erase the keyword formID from the set, if it existed.
			return false;
		}
		bool Remove(RE::TESForm* form, RE::BGSKeyword* keyword) {
			if (!form || !keyword)
				return false;

			Locker locker(lock);
			Iterator iter = data.find(form->formID);
			if (!iter->second.contains(keyword->formID))
				return false; // Attempted to remove a keyword we didn't add

			auto kwdForm = form->As<RE::BGSKeywordForm>();
			if (!kwdForm)
				return false; // form can't hold keywords

			if (!kwdForm->HasKeyword(keyword))
				return iter->second.erase(keyword->formID) > 0; // keyword formID did exist in the set but the form doesn't have the keyword for some reason

			if (kwdForm->RemoveKeyword(keyword)) {
				iter->second.erase(keyword->formID); // Keyword did get removed, so erase entry from set. erase() can't fail.
				return true;
			}
			
			return false; // form had the keyword, and the set was up to date, but the keyword removal failed for some reason
		}

		void Clear() { Locker locker(lock); data.clear(); }
		u64 ClearInvalids() { Locker locker(lock); return ClearInvalidsImpl(); }
		string Dump() {
			string result = "";
			Locker locker(lock);
			result = "[ExtraKeywords Dump] (" + to_string(data.size()) + " entries)\n";
			for (const auto& [formID, kwds] : data) {
				if (RE::TESForm* form = RE::TESForm::LookupByID(formID); form)
					result += "\n\t" + PrimitiveUtils::u32_xstr(formID) + " (" + form->GetName() + ")";
				else
					result += "\n\t" + PrimitiveUtils::u32_xstr(formID);
				result += " has " + to_string(kwds.size()) + " extra keywords:";

				for (const auto& kwd : kwds) {
					if (RE::BGSKeyword* kwdForm = RE::TESForm::LookupByID<RE::BGSKeyword>(kwd); kwdForm)
						result += "\n\t\t" + PrimitiveUtils::u32_xstr(kwd) + " (" + string{ kwdForm->formEditorID } + ")";
					else
						result += "\n\t\t" + PrimitiveUtils::u32_xstr(kwd);
				}
			}
			return result + "\n[ExtraKeywords Dump Finished]";
		}


		bool Save(SKSE::SerializationInterface& intfc) {
			using PrimitiveUtils::u32_xstr;

			if (!intfc.OpenRecord(SerializationType, SerializationVersion))
				return false;

			Locker locker(lock);

			ClearInvalidsImpl();

			if (!intfc.WriteRecordData(data.size())) {	// Serialize how many pairs are gonna be serialized
				Log::Critical("Failed to serialize ExtraKeywords data map size!"sv);
				return false;
			}

			for (const auto& [formID, kwds] : data) {
				if (!intfc.WriteRecordData(formID)) { Log::Critical("Failed to serialize formID {} for ExtraKeywords!"sv, u32_xstr(formID)); return false; }

				if (!intfc.WriteRecordData(kwds.size())) { Log::Critical("Failed to serialize {}'s extra keywords!"sv, u32_xstr(formID)); return false; }
				for (const auto kwd : kwds) {
					if (!intfc.WriteRecordData(kwd)) { Log::Critical("Failed to serialize extra keyword {} for {}!"sv,  u32_xstr(kwd), u32_xstr(formID)); return false; }
				}
			}
			Log::Info("Serialized ExtraKeywords"sv);
			return true;
		}

		bool Load(SKSE::SerializationInterface& intfc) {
			using PrimitiveUtils::u32_xstr;

			u64 size;
			if (!intfc.ReadRecordData(size)) {  // Deserialize how many pairs we are gonna read
				Log::Critical("Failed to read size of ExtraKeywords data map!"sv);
				return false;
			}

			Locker locker(lock);
			data.clear();

			u64 i = 0;
			for (i = 0; i < size; i++) {
				RE::FormID formID;

				if (!intfc.ReadRecordData(formID)) { Log::Critical("Failed to deserialize formID {} for ExtraKeywords!"sv, u32_xstr(formID)); return false; }
				bool failstate = false; // Resolve failure doesn't mean we're out. Deserialize keyword ids to progress the buffer, ignore this formID, and move to the next.
				if (!intfc.ResolveFormID(formID, formID)) { Log::Critical("Failed to resolve formID {}!"sv, u32_xstr(formID)); failstate = true; }


				u64 keywordsSize;
				if (!intfc.ReadRecordData(keywordsSize)) { Log::Critical("Failed to read extra keywords set size for formID {}!"sv, u32_xstr(formID)); return false; }

				RE::BGSKeywordForm* kwdForm = nullptr;
				Iterator iter = data.end();
				if (!failstate) {
					if (kwdForm = RE::TESForm::LookupByID<RE::BGSKeywordForm>(formID); kwdForm) {
						iter = data.emplace(formID, std::set<RE::FormID>{}).first; // Make a map entry here so we can check its success and avoid adding keywords if it failed
					}
					failstate = !kwdForm || iter == data.end();
				}

				for (u64 k = 0; k < keywordsSize; k++) {
					RE::FormID kwdID;
					if (!intfc.ReadRecordData(kwdID)) { Log::Error("Failed to read extra keyword formID for {}!"sv, u32_xstr(formID)); return false; }
					// Reading is done for this loop, so only continue if we haven't failed.
					if (!failstate) {
						if (!intfc.ResolveFormID(kwdID, kwdID)) {
							Log::Error("Failed to resolve extra keyword formID {} and cannot add it to {}!"sv, u32_xstr(kwdID), u32_xstr(formID));
							continue;
						}
						if (RE::BGSKeyword* kwd = RE::TESForm::LookupByID<RE::BGSKeyword>(kwdID); kwd && iter->second.insert(kwdID).second) {
							if (!kwdForm->AddKeyword(kwd)) { // Adding the keyword to the form failed
								iter->second.erase(kwdID); // So erase the kwdID that just got inserted and go on with looping
							}
						}
					}
				}
			}
			Log::Info("Deserialized {} Keyword sets"sv, i);
			return true;
		}

		void Revert() { Locker locker(lock); data.clear(); }



	}

}
