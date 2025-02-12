#include "TestFunctions.h"
#include "Logger.h"

// #include "Utils/ScriptUtils.h"
#include "Utils/MenuUtils.h"
// #include "Utils/StringUtils.h"
#include "Utils/GameDataUtils.h"
#include "Utils/PrimitiveUtils.h"
#include <chrono>
// using namespace std::chrono;
using namespace GameDataUtils;
using namespace PrimitiveUtils;

#include "Forms/VanillaForms.h"

#include "RulesMenu.h"


namespace TestFunctions {

	// xd
	string ToBits (const u64 num) {
		string str{};

		if (num & (1ui64 << 63)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 62)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 61)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 60)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 59)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 58)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 57)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 56)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 55)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 54)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 53)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 52)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 51)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 50)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 49)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 48)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 47)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 46)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 45)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 44)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 43)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 42)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 41)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 40)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 39)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 38)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 37)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 36)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 35)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 34)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 33)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 32)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 31)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 30)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 29)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 28)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 27)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 26)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 25)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 24)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 23)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 22)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 21)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 20)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 19)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 18)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 17)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 16)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 15)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 14)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 13)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 12)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 11)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 10)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 9)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 8)) { str += '1'; } else { str += '0'; }
		str += ' ';
		if (num & (1ui64 << 7)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 6)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 5)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 4)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 3)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 2)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 1)) { str += '1'; } else { str += '0'; }
		if (num & (1ui64 << 0)) { str += '1'; } else { str += '0'; }

		return str;
	}



	void DoSomething2(StaticFunc) {
		// auto token = Log::RegisterSink(Log::ConsoleCallback, Log::info, Log::prohibit);
		Log::Info("Do2: ran!"sv);
		

		static auto mb = [] {
			Log::Info("Do2: MBV thread ran!"sv);
			const string msg{ "asd\n<font color = '#FF21A43F'>a\na\na\na\na\n<font color = '#FFFF419D'>a\na\na\na\n1\n2\n3\n4\n5\n6666666666666666666666666 66666666666666666666666666666 66666666666666666666666666 666666666666666666666666 6666666666666666666666666666666" };
			lazy_vector<string> btns{};
			btns.try_append("first");
			btns.try_append("second");
			btns.try_append("1111111111111a1111111111111 2222222222222b2222222222222 3333333333333c3333333333333 4444444444444d4444444444444 5555555555555e5555555555555 6666666666666f6666666666666");
			btns.try_append("<font color = '#FF21FF3F'>fourth");
			btns.try_append("<font color = '#FFFFFF3F'>fifth");
			btns.try_append("<font color = '#FFDD403F'>sixth");
			btns.try_append("<font color = '#FF2150FF'>seventh");
			btns.try_append("<font color = '#FFAAFF10'>eight");
			btns.try_append("<font color = '#FFBCDEF0'>ninth");
			btns.try_append("<font color = '#FF01FDBC'>tenth");
			btns.try_append("<font color = '#FF01FDAA'>eleventh");
			btns.try_append("<font color = '#FFA10D50'>twelfth");
			btns.try_append("<font color = '#FFF1AD80'>thirteenth");
			btns.try_append("<font color = '#FF508DF0'>fourteenth");
			btns.try_append("<font color = '#FF99FDA0'>fifteenth");
			if (btns.size() != 15) {
				Log::Warning("Do2: MB thread only created {} out of 15 buttons!"sv, btns.size());
			}
			// const auto result = MenuUtils::WaitEVMMBV(msg, btns);
			const auto result = MenuUtils::WaitEVMMBV(msg, btns);
			Log::Info("Do2: MBV thread ended! Result <{}>"sv, result);
		};
		static auto sm = [] {
			Log::Info("Do2: SM thread ran!"sv);
			std::array tac{ "My Title!"s, "Aggsept"s, "NO!"s };
			lazy_vector<MenuUtils::slider_params> params{};
			params.try_append(MenuUtils::slider_params::values{ .text = "first", .info = "first info", .min = 0.0f, .max = 100.0f, .start = 2.0f, .step = 1.0f, .decimals = 0 });
			params.try_append(MenuUtils::slider_params::values{ .text = "secon", .info = "secon info", .min = -5.0f, .max = 5.0f, .start = -1.0f, .step = 0.5f, .decimals = 1 });
			params.try_append(MenuUtils::slider_params::values{ .text = "third", .info = "third info", .min = 5.0f, .max = 50.0f, .start = 20.0f, .step = 5.0f, .decimals = 0 });
			if (params.size() != 3) {
				Log::Warning("Do2: SM thread only created {} out of 3 sliders!"sv, params.size());
			}
			const auto result = MenuUtils::WaitEVSM(tac, params);
			string msg{ "< "};
			for (const auto& r : result) {
				msg += r + ' ';
			}
			msg += '>';
			Log::Info("Do2: SM thread ended! Result: {}"sv, msg);
		};
		static auto mb2 = [] {
			Log::Info("Do2: MBV2 thread ran!"sv);
			const auto result = ::Data::PlayerRules::show_menu({});
			Log::Info("Do2: MBV2 thread ended!"sv);
		};


		std::thread{ mb2 }.detach();
		// std::thread{ mb }.detach();
		// std::thread{ sm }.detach();
		

		Log::Info("Do2: done!"sv);
	}

	void DoSomething3(StaticFunc, RE::Actor* act, RE::SpellItem* spell) {
		auto token = Log::RegisterSink(Log::ConsoleCallback, { .skse = Log::Severity::info, .papyrus = Log::Severity::prohibit });
		Log::Info("Do3: ran!"sv);
		if (!act or !spell) {
			Log::Info("Do3: null stuff!"sv);
			return;
		}

		if (!act->RemoveSpell(spell)) {
			Log::Warning("Do3: failed to remove spell!"sv);
		}


		Log::Info("Do3: done!"sv);
	}

	void DoSomething4(StaticFunc) {
		// auto token = Log::RegisterSink(Log::ConsoleCallback, Log::info, Log::prohibit);
		Log::Info("Do4: new ran!"sv);

		

		Log::Info("Do4: new done!"sv);
	}

	// Don't touch this. Reference code in case I ever do C++ overlay stuff.
	void DoSomething(VM* vm, StackID stackID, StaticFunc, RE::Actor* act, float mult, float green) {
		if (!vm || stackID < 0)
			stackID = stackID; // use args
		if (!act) {
			Log::ToConsole("Do: act invalid!"sv);
			return;
		}


		return;

		string name{ act->GetDisplayFullName() };

		auto actor3D2 = act->Get3D(false); if (!actor3D2) { Log::ToConsole("Do: Failed to get {}'s tp!"sv, name); return; }
		auto node3D = actor3D2->AsNode(); if (!node3D) { Log::ToConsole("Do: Failed to get {}'s root!"sv, name); return; }
		auto str3D = PrimitiveUtils::addr_str(actor3D2);
		auto strNode = PrimitiveUtils::addr_str(node3D);
		Log::ToConsole("Do: {}'s tp addr<{}>, node addr<{}>"sv, name, str3D, strNode);
		return;

		// To remove skin overlay named "ovlName":
		// 1: Get node:			Get3D(true/false)->AsNode(), ensuring pointers are valid
		// 2: Get ovl:			node->GetObjectByName(ovlName)
		// 3: Null ovl skin:	ovl->AsGeometry()->skinInstance = nullptr, ensuring it is a geometry
		// 4: Detach ovl:		node->DetachChild(ovl)
		// 5: Get ovl again:	node->GetObjectByName(ovlName)
		// 6: Repeat 2-4 until 4 gives nullptr
		// 




		// These can be stored. But better not to in case user fucks with overlays/overlay amounts in runtime. Could end up crashing?
		RE::BSLightingShaderProperty* lightingShader = nullptr;
		RE::BSLightingShaderMaterialFacegenTint* material = nullptr;

		if (lightingShader && material) {
			Log::ToConsole("Do: shader and material both exist! Recoloring..."sv);
			lightingShader->emissiveMult = mult; // Change emissive mult, aka how much the thing shines, aka key 1 for NiOverride Papyrus parameters
			*(lightingShader->emissiveColor) = RE::NiColor{ 0.0f, green, 0.0f }; // Change emissive color, aka the color of the thing shines as, aka key 0 for NiOverride Papyrus parameters
			material->tintColor = RE::NiColor{ 0.0f, green, 0.0f }; // Change tint color, aka the base color of the thing, aka key 7 for NiOverride Papyrus parameters
		}
		else {
			Log::ToConsole("Do: shader and/or material are null! Obtaining..."sv);
			auto actor3D = act->Get3D(false); if (!actor3D) { Log::ToConsole("Do: Failed to get tp!"sv); return; }
			auto ovl = actor3D->GetObjectByName("Body [ovl1]"); if (!ovl) { Log::ToConsole("Do: Failed to get ovl!"sv); return; }
			auto geom = ovl->AsGeometry(); if (!geom) { Log::ToConsole("Do: Failed to get geom!"sv); return; }
			auto effect = geom->properties[RE::BSGeometry::States::kEffect].get();
			if (effect) {
				lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect);
				if (lightingShader) {
					if (lightingShader->material && lightingShader->material->GetFeature() == RE::BSShaderMaterial::Feature::kFaceGenRGBTint) { // GetFeature() here always returns 5, aka kFaceGenRGBTint
						material = static_cast<RE::BSLightingShaderMaterialFacegenTint*>(lightingShader->material);
					}
					else { Log::ToConsole("Do: Failed to get material/not tint!"sv); return; }
				}
				else { Log::ToConsole("Do: Failed to get lighting shader!"sv); return; }
			}
			else { Log::ToConsole("Do: Failed to get effect!"sv); return; }

			if (lightingShader && material)
				Log::ToConsole("Do: successfully obtained shader and material!"sv);
			else
				Log::ToConsole("Do: Failed to obtain shader and/or material!"sv);
		}





		Log::ToConsole("Do: Done"sv);
		return;

	}

	/*
	bool FrameTest(VM* vm, StackID stackID, StaticFunc, RE::reference_array<i32> myArr, RE::TESForm* form, string name, string varName) {
		if (!vm->allRunningStacks.contains(stackID)) {
			Log::ToConsole("FrameTest: stackID <{}> not in running stacks!"sv, stackID);
			return false;
		}

		auto temp1 = *vm->allRunningStacks.find(stackID);
		auto stack = temp1.second;
		auto frames = stack->frames;
		auto top = stack->top;

		Log::ToConsole("FrameTest: stackID({}) frames({})"sv, stackID, frames);

		auto frame = top;
		for (u32 i = frames; i > 0; --i) {
			auto size = frame->size;
			auto pageHint = stack->GetPageForFrame(frame);
			auto fnName = string{ frame->owningFunction->GetName() };
			auto type = string{ frame->owningFunction->GetObjectTypeName() };
			auto numParams = frame->owningFunction->GetParamCount();
			Log::ToConsole("\tframe{}: size({}) page({}) type({}) func({}) params({})"sv, i, size, pageHint, type, fnName, numParams);
			for (u32 k = 0; k < numParams; ++k) {
				RE::BSFixedString out;
				frame->owningFunction->GetVarNameForStackIndex(k, out);
				auto paramName = string{ out };
				Log::ToConsole("\t\t#" + to_string(k + 1) + " " + paramName);
			}
			frame = frame->previousFrame;
		}


		if (varName.length() <= 0) {
			Log::ToConsole("FrameTest: Bad variable names"sv);
			return false;
		}
		vector<RE::BSScript::Variable*> vars;
		if (!ScriptUtils::GetVariables(form, name, vector<string>{ varName }, vars)) {
			Log::ToConsole("FrameTest: Failed to get variables for {}"sv, name);
			return false;
		}
		if (vars.size() != 1) {
			Log::ToConsole("FrameTest: Bad obtained {} array size"sv, name);
			return false;
		}
		if (!vars[0]->IsLiteralArray()) {
			Log::ToConsole("FrameTest: Bad obtained {} array type"sv, name);
			return false;
		}

		auto frame2 = top;
		auto hint = stack->GetPageForFrame(frame2);
		RE::BSScript::Variable arr = frame2->GetStackFrameVariable(0, hint);
		RE::BSScript::Variable& arr2 = frame2->GetStackFrameVariable(0, hint);
		Log::ToConsole("\n\tjanky var: {}"sv, ScriptUtils::VarInfo(&arr));


		auto addrRet = PrimitiveUtils::addr_str(std::addressof(frame2->GetStackFrameVariable(0, hint)));
		auto addrCpy = PrimitiveUtils::addr_str(std::addressof(arr));
		auto addrRef = PrimitiveUtils::addr_str(std::addressof(arr2));
		auto addrGot = PrimitiveUtils::addr_str(vars[0]);

		Log::ToConsole("FrameTest: Addresses:\n\tret({})\tcpy({})\n\tref({})\tgot({})"sv, addrRet, addrCpy, addrRef, addrGot);



		return true;
	}
	*/



	bool Register(IVM* vm) {

		vm->RegisterFunction("DoSomething"sv, script, DoSomething);
		vm->RegisterFunction("DoSomething4"sv, script, DoSomething4);
		vm->RegisterFunction("DoSomething3"sv, script, DoSomething3);
		vm->RegisterFunction("DoSomething2"sv, script, DoSomething2);
		// vm->RegisterFunction("FrameTest"sv, script, FrameTest, true);

		return true;
	}

}

