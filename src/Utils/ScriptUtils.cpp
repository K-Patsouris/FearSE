#include "ScriptUtils.h"
#include "Logger.h"
#include "OutUtils.h"
#include "StringUtils.h"

namespace ScriptUtils {
	
	bool GetScriptObject(RE::TESForm* form, string scriptName, RE::BSTSmartPointer<RE::BSScript::Object>& object_out, RE::BSScript::Internal::VirtualMachine* vm) {
		if (!form or scriptName.length() <= 0)
			return false;

		if (!vm)
			if (vm = RE::BSScript::Internal::VirtualMachine::GetSingleton(); !vm)
				return false;
		const auto policy = vm->GetObjectHandlePolicy();
		if (!policy)
			return false;
		const auto handle = policy->GetHandleForObject(static_cast<RE::VMTypeID>(form->GetFormType()), form);
		if (handle == policy->EmptyHandle())
			return false;
		if (!vm->FindBoundObject(handle, scriptName.c_str(), object_out) || !object_out)
			return false;

		return true;
	}

	// If script named scriptName is attatched to form and each string in names is a property name (of it or its parents), each of them is put in properties and returns true. Returns false if anything goes wrong.
	// form can be any TESForm* or any form that inherits it (Actor*, TESQuest*, TESObjectREFR*, etc). Case of strings in propNames doesn't matter.
	// If a property is FormsHolder/ActiveEffects/Aliases/array use RE::BSScript::Variable's Unpack with the proper type pointer to get a usable FormsHolder/ActiveEffects/Aliases pointer/array.
	// For primitive types use RE::BSScript::Variable's getters (GetSInt, GetString, etc) as they are simpler. If you mess up the type you will get gibberish but game won't crash.
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, vector<RE::BSScript::Variable*>& properties_out, RE::BSScript::Internal::VirtualMachine* vm, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!form || scriptName.length() <= 0 || propNames.size() <= 0)
			return false;

		if (!object) {
			if (!vm)
				if (vm = RE::BSScript::Internal::VirtualMachine::GetSingleton(); !vm)
					return false;
			const auto policy = vm->GetObjectHandlePolicy();
			if (!policy)
				return false;
			const auto handle = policy->GetHandleForObject(static_cast<RE::VMTypeID>(form->GetFormType()), form);
			if (handle == policy->EmptyHandle())
				return false;
			if (!vm->FindBoundObject(handle, scriptName.c_str(), object) || !object)
				return false;
		}

		properties_out.resize(propNames.size());
		for (u64 i = 0; i < properties_out.size(); ++i) {
			if (properties_out[i] = object->GetProperty(propNames[i]); !properties_out[i]) {
				properties_out.clear();
				return false;
			}
		}

		return true;
	}
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, vector<RE::BSScript::Variable*>& properties_out, RE::BSScript::Internal::VirtualMachine* vm) {
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		return GetProperties(form, scriptName, propNames, properties_out, vm, object);
	}
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, vector<RE::BSScript::Variable*>& properties_out, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		return GetProperties(form, scriptName, propNames, properties_out, nullptr, object);
	}
	// Same as above but for variables instead of properties
	bool GetVariables(const void* src, RE::VMTypeID typeID, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		variables_out.clear();
		if (!object) {
			if (!vm)
				if (vm = RE::BSScript::Internal::VirtualMachine::GetSingleton(); !vm)
					return false;
			const auto policy = vm->GetObjectHandlePolicy();
			if (!policy)
				return false;
			const auto handle = policy->GetHandleForObject(typeID, src);
			if (handle == policy->EmptyHandle()) {
				Log::Error("GetVariables: Failed to get handle for {} (typeID={})"sv, scriptName, typeID);
				return false;
			}
			if (!vm->FindBoundObject(handle, scriptName.c_str(), object) || !object) {
				Log::Error("GetVariables: Failed to get object for {} (typeID={})"sv, scriptName, typeID);
				return false;
			}
		}

		variables_out.resize(varNames.size());
		for (u64 i = 0; i < variables_out.size(); ++i) {
			if (variables_out[i] = object->GetVariable(varNames[i]); !variables_out[i]) {
				variables_out.clear();
				return false;
			}
		}

		return true;
	}
	
	bool GetVariables(RE::TESForm* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm) {
		if (!src || scriptName.length() <= 0 || varNames.size() <= 0) { variables_out.clear(); return false; }
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		return GetVariables(src, static_cast<RE::VMTypeID>(src->GetFormType()), scriptName, varNames, variables_out, vm, object);
	}
	bool GetVariables(RE::TESForm* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!src || scriptName.length() <= 0 || varNames.size() <= 0) { variables_out.clear(); return false; }
		return GetVariables(src, static_cast<RE::VMTypeID>(src->GetFormType()), scriptName, varNames, variables_out, nullptr, object);
	}
	bool GetVariables(RE::BGSBaseAlias* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm) {
		if (!src || scriptName.length() <= 0 || varNames.size() <= 0) { variables_out.clear(); return false; }
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		return GetVariables(src, src->GetVMTypeID(), scriptName, varNames, variables_out, vm, object);
	}
	bool GetVariables(RE::BGSBaseAlias* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!src || scriptName.length() <= 0 || varNames.size() <= 0) { variables_out.clear(); return false; }
		return GetVariables(src, src->GetVMTypeID(), scriptName, varNames, variables_out, nullptr, object);
	}
	bool GetVariables(RE::ActiveEffect* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm) {
		if (!src || scriptName.length() <= 0 || varNames.size() <= 0) { variables_out.clear(); return false; }
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		return GetVariables(src, RE::ActiveEffect::VMTYPEID, scriptName, varNames, variables_out, vm, object);
	}
	bool GetVariables(RE::ActiveEffect* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!src || scriptName.length() <= 0 || varNames.size() <= 0) { variables_out.clear(); return false; }
		return GetVariables(src, RE::ActiveEffect::VMTYPEID, scriptName, varNames, variables_out, nullptr, object);
	}


	// Map versions of above functions. Maps are way more convenient for internal use but vectors are more convenient for returning to Papyrus.
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, std::map<string, RE::BSScript::Variable*>& properties_out, RE::BSScript::Internal::VirtualMachine* vm, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!form || scriptName.length() <= 0 || propNames.size() <= 0)
			return false;

		if (!object) {
			if (!vm)
				if (vm = RE::BSScript::Internal::VirtualMachine::GetSingleton(); !vm)
					return false;
			const auto policy = vm->GetObjectHandlePolicy();
			if (!policy)
				return false;
			const auto handle = policy->GetHandleForObject(static_cast<RE::VMTypeID>(form->GetFormType()), form);
			if (handle == policy->EmptyHandle())
				return false;
			if (!vm->FindBoundObject(handle, scriptName.c_str(), object) || !object)
				return false;
		}

		properties_out.clear();
		for (u64 i = 0; i < propNames.size(); ++i) {
			if (properties_out[propNames[i]] = object->GetProperty(propNames[i]); !properties_out[propNames[i]]) {
				properties_out.clear();
				return false;
			}
		}

		return true;
	}
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, std::map<string, RE::BSScript::Variable*>& properties_out, RE::BSScript::Internal::VirtualMachine* vm) {
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		return GetProperties(form, scriptName, propNames, properties_out, vm, object);
	}
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, std::map<string, RE::BSScript::Variable*>& properties_out, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		return GetProperties(form, scriptName, propNames, properties_out, nullptr, object);
	}

	bool GetVariables(RE::TESForm* form, string scriptName, vector<string> varNames, std::map<string, RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!form || scriptName.length() <= 0 || varNames.size() <= 0)
			return false;

		if (!object) {
			if (!vm)
				if (vm = RE::BSScript::Internal::VirtualMachine::GetSingleton(); !vm)
					return false;
			const auto policy = vm->GetObjectHandlePolicy();
			if (!policy)
				return false;
			const auto handle = policy->GetHandleForObject(static_cast<RE::VMTypeID>(form->GetFormType()), form);
			if (handle == policy->EmptyHandle())
				return false;
			if (!vm->FindBoundObject(handle, scriptName.c_str(), object) || !object)
				return false;
		}

		RE::BSScript::ObjectTypeInfo* info = object->GetTypeInfo();
		if (!info)
			return false;

		const auto it = info->GetVariableIter();
		if (!it)
			return false;

		variables_out.clear();
		auto numVars = info->GetNumVariables();
		for (u32 i = 0; (variables_out.size() < varNames.size()) && (i < numVars); ++i) {
			for (u64 k = 0; k < varNames.size(); ++k) {
				if (it[i].name == varNames[k].c_str()) {
					variables_out[varNames[k]] = std::addressof(object->variables[i]);
				}
			}
		}

		if (variables_out.size() != varNames.size()) {
			variables_out.clear();
			return false;
		}

		return true;
	}
	bool GetVariables(RE::TESForm* form, string scriptName, vector<string> varNames, std::map<string, RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm) {
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		return GetVariables(form, scriptName, varNames, variables_out, vm, object);
	}
	bool GetVariables(RE::TESForm* form, string scriptName, vector<string> varNames, std::map<string, RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		return GetVariables(form, scriptName, varNames, variables_out, nullptr, object);
	}


	// Works but I can't figure out how to get the return, so it's often useless.
	bool CallFunc(RE::BSTSmartPointer<RE::BSScript::Object>& object, string fnName, RE::BSScript::Internal::VirtualMachine* vm, RE::BSScript::IFunctionArguments* args) {
		if (!object || fnName.length() <= 0) {
			Log::ToConsole("CallFunc: Invalid object or function name!"sv);
			return false;
		}

		if (!vm)
			if (vm = RE::BSScript::Internal::VirtualMachine::GetSingleton(); !vm) {
				Log::ToConsole("CallFunc: Can't get vm!"sv);
				return false;
			}

		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
		bool success = vm->DispatchMethodCall1(object, fnName, args, result);
		delete args;
		return success;
	}
	
	RE::BSScript::Variable* GetVarFromFrame(RE::VMStackID stackID, u32 frameDepth, u32 varIdx, RE::BSScript::Internal::VirtualMachine* vm) {
		if (!vm)
			vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm || !vm->allRunningStacks.contains(stackID))
			return nullptr;

		RE::BSTSmartPointer<RE::BSScript::Stack> stack = (*vm->allRunningStacks.find(stackID)).second;	// stack
		auto frames = stack->frames;								// number of frames on stack
		if (frameDepth >= frames)
			return nullptr;

		auto frame = stack->top;
		for (u32 i = 0; i < frameDepth; i++)
			frame = frame->previousFrame;

		auto numParams = frame->owningFunction->GetParamCount();	// frame->size probably works too, but this looks clearer
		if (varIdx >= numParams)
			return nullptr;
		
		RE::BSScript::Variable& var = frame->GetStackFrameVariable(varIdx, stack->GetPageForFrame(frame));
		return std::addressof(var);
	}

	RE::BSScript::ObjectTypeInfo* VarGetInfo(RE::BSScript::Variable* var) {
		if (!var) {
			return nullptr;
		}
		static const u64 arr_end = static_cast<u64>(RE::BSScript::TypeInfo::RawType::kArraysEnd);
		auto info = *reinterpret_cast<u64*>(var);
		if ((info <= arr_end) or (info & 1)) {
			return nullptr;	// Primitive, or primitive array, or an array of objects
		}
		return reinterpret_cast<RE::BSScript::ObjectTypeInfo*>(info);
	}
	RE::BSScript::ObjectTypeInfo* VarGetArrInfo(RE::BSScript::Variable* var) {
		if (!var) {
			return nullptr;
		}
		static const u64 arr_end = static_cast<u64>(RE::BSScript::TypeInfo::RawType::kArraysEnd);
		auto info = *reinterpret_cast<u64*>(var);
		if ((info <= arr_end) or !(info & 1)) {
			return nullptr;	// Primitive, or primitive array, or not an array of objects
		}
		return reinterpret_cast<RE::BSScript::ObjectTypeInfo*>(info - 1); // -1 to the ui64 to unset the first bit, removing the array "flag"
	}
	bool VarIsIntArr(RE::BSScript::Variable* var) {
		return *(reinterpret_cast<u64*>(var)) == static_cast<u64>(RE::BSScript::TypeInfo::RawType::kIntArray);
	}
	bool VarIsFloatArr(RE::BSScript::Variable* var) {
		return *(reinterpret_cast<u64*>(var)) == static_cast<u64>(RE::BSScript::TypeInfo::RawType::kFloatArray);
	}
	bool VarIsBoolArr(RE::BSScript::Variable* var) {
		return *(reinterpret_cast<u64*>(var)) == static_cast<u64>(RE::BSScript::TypeInfo::RawType::kBoolArray);
	}
	bool VarIsStringArr(RE::BSScript::Variable* var) {
		return *(reinterpret_cast<u64*>(var)) == static_cast<u64>(RE::BSScript::TypeInfo::RawType::kStringArray);
	}

	bool TypeIsOrHasParent(const RE::BSScript::ObjectTypeInfo* type, const string& parent_classname) {
		auto typeParent = type; // Start from the type itself
		while (typeParent) {
			if (StringUtils::StrICmp(typeParent->GetName(), parent_classname.c_str())) {
				return true;
			}
			typeParent = typeParent->GetParent();
		}
		return false;
	}
	bool VarIsClass(RE::BSScript::Variable* var, const string& classname) {
		if (!var or classname.empty()) {
			return false;
		}
		return TypeIsOrHasParent(VarGetInfo(var), classname);
	}
	bool ArrVarIsClass(RE::BSScript::Variable* var, const string& classname) {
		if (!var or classname.empty()) {
			return false;
		}
		return TypeIsOrHasParent(VarGetArrInfo(var), classname);
	}

	// For debug purposes
	string VarInfo(RE::BSScript::Variable* var) {
		string str{};
		if (var->IsArray())
			if (auto arr = var->GetArray(); arr)
				str += "IsArray(" + to_string(arr->size()) + ") ";
			else
				str += "IsArray() ";
		if (var->IsInt())
			str += "IsInt(" + to_string(var->GetSInt()) + ") ";
		if (var->IsFloat())
			str += "IsFloat(" + to_string(var->GetFloat()) + ") ";
		if (var->IsBool())
			str += "IsBool(" + to_string(var->GetBool()) + ") ";
		if (var->IsString())
			str += "IsString(" + string{ var->GetString() } + ") ";
		if (var->IsObject())
			if (auto obj = var->GetObject(); obj)
				if (auto info = obj->GetTypeInfo(); info)
					str += "IsObject(" + string{ info->GetName() } + ") ";
				else
					str += "IsObject ";
			else
				str += "IsObject ";
		if (var->IsLiteralArray())
			if (auto arr = var->GetArray(); arr)
				str += "IsLiteralArray(" + to_string(arr->size()) + ") ";
			else
				str += "IsLiteralArray() ";
		if (var->IsNoneArray())
			if (auto arr = var->GetArray(); arr)
				str += "IsNoneArray(" + to_string(arr->size()) + ") ";
			else
				str += "IsNoneArray() ";
		if (var->IsNoneObject())
			if (auto obj = var->GetObject(); obj)
				if (auto info = obj->GetTypeInfo(); info)
					str += "IsNoneObject(" + string{ info->GetName() } + ") ";
				else
					str += "IsNoneObject ";
			else
				str += "IsNoneObject ";
		if (var->IsObjectArray())
			if (auto arr = var->GetArray(); arr)
				str += "IsObjectArray(" + to_string(arr->size()) + ") ";
			else
				str += "IsObjectArray() ";
		str.pop_back();
		return str;
	}
	bool PrintVars(RE::BSTSmartPointer<RE::BSScript::Object>& object) {
		if (!object) {
			return false;
		}

		RE::BSScript::ObjectTypeInfo* info = object->GetTypeInfo();
		if (!info)
			return false;

		const auto it = info->GetVariableIter();
		if (!it)
			return false;

		auto numVars = info->GetNumVariables();
		for (u32 i = 0; i < numVars; ++i) {
			Log::ToConsole("PrintVars: [{}] is {}"sv, i, it[i].name.c_str());
		}

		return true;
	}

	/*
	// Papyrus script property/variable manipulation
	// 		Notes
	// 				General
	// DispatchMethodCall1/2, as well as SetPropertyValue seem to be queued actions. Lines below them will run before Papyrus has time to process. 
	// Getting a var, then setting it with SetPropertyValue, and then getting it again will result in getting old value with the second get.
	// Getting a var, then setting it with its own setter (like SetSInt), and then getting it again will result in getting the proper value with the second get.
	// Papyrus will see whatever was set with own setters. No need for SetPropertyValue.
	// For proper sync with DispatchMethodCall1/2, GetPropertyValue would probably have to be used but I had trouble making that work.
	// As long as there is no need for calling Papyrus functions inbetween getting and setting properties then there is no reason not using own getters and setters.
	// 
	// If other parameters are ok, DispatchMethodCall1/2 will not crash and return true if the given argument list (args) does not match Papyrus function's, but the Papyrus function will not run.
	// 
	// 				Property/Variable Access
	// Variable and property names and types are stored in ObjectTypeInfo's "data" and can be found by iterating with its iterators.
	// PropertyInfo has properties only. VariableInfo has both properties and variables with property names are prefixed with "::" and posfixed with "_var".
	// Maybe because of that, when iterating VariableInfo, properties appear first in alphabetical order and variables come right after also in alphabetical order. eg In a script with:
	// 	Int Property a_num Auto
	// 	Int a_numA
	// 	Int Property b_num Auto
	// 	Int b_numA
	// their order of appearance when iterating VariableInfo would be:
	// 	a_num	(name = "::a_num_var")
	// 	b_num	(name = "::b_num_var")
	// 	a_numA	(name = "a_numA")
	// 	b_numA	(name = "b_numA")
	// So even though variables don't have a PropertyTypeInfo like properties, which provides a convenient autoVarIndex member to use as index for Object's "variables" array, that index can be found
	// by simply iterating up to GetNumVariables() times on Object's ObjectTypeInfo and checking the name of the variable/property on each iteration.
	// When the name wanted is found, the iteration# (from 0 to GetNumVariables()-1) is the index to be used on Object's "variables" array. In case of objects that inherit custom types, their variables array contains
	// the variables and properties of every inherited type (and some other overhead I think, not sure but not relevant) so to get the proper index we must add the total number of variables before this type's to the
	// iteration#. This offset can be found very easily with GetTotalNumVariables()-GetNumVariables(). On a type with no parents this will evaluate to 0 so no need to worry about checking whether the type is a child or not.
	// 
	// 				About local variables
	// From what I understand from MatshallAndDispatch in NativeFunction.h, and some testing, Papyrus function arguments are unpacked to values upon arrival to C++. Unpacking creates new data, effectively simulating
	// "pass by value". This means that unless they were pointers to begin with (aka Form, Alias, or ActiveMagicEffect), those arguments are not themselves accessible and hence not changeable.
	// Arrays are a pain on their own. They get unpacked to become reasonably usable, and get repacked when sent back to Papyrus. The problem with that is that we can't assign array pointers the way it's done in Papyrus
	// In Papyrus, simply assigning one array to another makes them point to the same data. This can't be done even with reference_array. Upon dispatch back to Papyrus, the container will simply get its values packed
	// to the Papyrus contain we never had access to. The only way I found to achieve that is to somehow find the RE::BSScript::Variable object of the source and target arrays, and copy the source's onto the target's.
	// Copying is what it takes. Those Variable objects are supercontainers for Papyrus variables/properties. Every variable/property is internally a Variable object, and the type differentiation is done via a union.
	// The Variable object of arrays and Form/Alias/ActiveMagicEffect Papyrus variables/properties has a pointer type in the union so copying one Variable object to another achieves both the source and the target to
	// have their variable/property which points to the same thing. This is what Papyrus assignments achieve. The problem is getting those Variable objects. It can be done reliably by providing the
	// Form (RE::TESForm*) / Alias (RE::BGSBaseAlias*) / ActiveMagicEffect (RE::ActiveEffect*) that has the source variable, the name of the script attatched to it in which the variable is (eg string),
	// and the name of the variable itself (eg also string), as well as those things for the target variable as well, and then finding the Variable objects as described above. It makes sense as to pinpoint a variable
	// one has to know it's name, the name of the script that defines it, and the Form/Alias/ActiveMagicEffect on which an instance of that script is attatched. The problem is that with what Papyrus can provide to C++
	// none of those 3 things can be deduced, meaning that Papyrus has to provide them all. So, a C++ function that achieves a variable assignment needs 6 parameters, which makes it ugly and kinda cumbersome to use.
	// My best shot was to get the StackFrame via the optional VMTypeID parameter CommonLibSSE provides but the Variable objects in the Stack/StackFrame are copies of the actual Variable objects in an Object. To reach an
	// end through that avenue I'd have to get a LOT deeper and modify how Stack and Stackframe objects are created upon function call which, apart from hard af, is obviously a terrible idea.
	// 
	// 				About Variable objects
	// Every single Papyrus variable/property is internally a Variable object. The values of Bool, Int, and Float variables/properties are stored directly in the Variable object, while strings, arrays, and non-primitives
	// (all of which, even strings, are pointers, even though Papyrus does not use different notation) have only a pointer to them stored in the Variable object. Variable objects have no identifying information.
	// An Int variable with value 10 in scriptA and an Int property with value 10 in scriptB would be identical Variable objects internally. Different objects, of course, but otherwise indistinguishable.
	// When a specific non-primitive is assigned via Papyrus, the pointer in the Variable object will be a specific one for that non-primitive. The Variable objects scriptA's Actor varA = game.getplayer() and
	// scriptB's Actor varB = game.getplayer() would be identical.
	// Bitwise, a Variable object has 128 bits. The first 64 are a TypeInfo object, and the last 64 are a Value union object (defined in the Variable class). The TypeInfo of 2 Variable objects with the same type are equal.
	// This seems unimportant, but with what CommonLibSSE provides it is difficult to compare types. There is no other good way to tell an Int[] from a Bool[], for example, and it gets worse with non-primitives.
	// If every Papyrus array was forced to have at least 1 element then internally that element could be checked, but arrays can be empty. But that's not the case so other methods have to be used.
	// It's important to note that 2 Variable objects can store the same non-primitive pointer but have different TypeInfo. The Variable objects of scriptA's Actor varA = game.getplayer() and scriptB's var2 Form = game.getplayer()
	// would have different TypeInfo but identical Value, so there would be differences in the first 64 bits but the last 64 would be the same. 


	// would produce two identical
	// 
	// 				UserFlags
	// Every info seems to have 2 UserFlagInfo. One's GetUserFlag() returns "hidden" and the other "conditional". The script itself being hidden and/or conditinal and/or having hidden/conditional properties seems irrelevant.
	// 
	*/





	
	
}

