#include "SLHelpers.h"
#include "Utils/ScriptUtils.h"


namespace SLHelpers {
	using namespace ScriptUtils;

	sslThreadController_interface::sslThreadController_interface(RE::TESForm* controller) noexcept : actor_count{ 0 } {
		if (RE::BSTSmartPointer<RE::BSScript::Object> object; GetScriptObject(controller, "sslThreadController", object)) {
			
			if (auto Positions = object->GetProperty("Positions"); ArrVarIsClass(Positions, "Actor")) {
				if (std::vector<RE::Actor*> temp{ Positions->Unpack<vector<RE::Actor*>>() }; !temp.empty()) {

					RE::ActorPtr passive{ RE::ActorHandle{ temp.front() }.get() };
					if (passive.get() == nullptr) {
						return; // Consider null passive an invalid state
					}

					actors[0] = std::move(passive);
					actor_count = 1;

					for (size_t i = 1, max = std::min<size_t>(temp.size(), MaxActors); i < max; ++i) {
						if (RE::ActorPtr ptr{ RE::ActorHandle{ temp[i] }.get()}; ptr) {
							actors[actor_count] = std::move(ptr);
							++actor_count;
						}
					}

				}
			}

			if (auto Victims = object->GetProperty("Victims"); ArrVarIsClass(Victims, "Actor")) {
				std::vector<RE::Actor*> temp{ Victims->Unpack<vector<RE::Actor*>>() };
				auto find = [this](RE::Actor* act) {
					u32 i = 0;
					for (; i < actor_count; ++i) {
						if (actors[i].get() == act) {
							break;
						}
					}
					return i;
				};
				for (const auto act : temp) {
					victim_flags.set(find(act));
				}
			}

			if (auto Animation = object->GetProperty("Animation"); VarIsClass(Animation, "sslBaseAnimation")) {
				if (auto animobj = Animation->GetObject(); animobj) {
					if (auto Tags = animobj->GetVariable("Tags"); VarIsStringArr(Tags)) {
						tags = DumbUnpack<string>(Tags);
						tags.erase_all("");
					}
				}
			}

		}
	}

}

