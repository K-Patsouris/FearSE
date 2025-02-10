#pragma once
#include "Common.h"


namespace ScriptUtils {

	bool GetScriptObject(RE::TESForm* form, string scriptName, RE::BSTSmartPointer<RE::BSScript::Object>& object_out, RE::BSScript::Internal::VirtualMachine* vm = nullptr);
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, vector<RE::BSScript::Variable*> &properties_out, RE::BSScript::Internal::VirtualMachine * vm = nullptr);
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, vector<RE::BSScript::Variable*> &properties_out, RE::BSTSmartPointer<RE::BSScript::Object> &object);

	bool GetVariables(RE::TESForm* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine * vm = nullptr);
	bool GetVariables(RE::TESForm* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object> &object);
	bool GetVariables(RE::BGSBaseAlias* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine * vm = nullptr);
	bool GetVariables(RE::BGSBaseAlias* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object> &object);
	bool GetVariables(RE::ActiveEffect* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine * vm = nullptr);
	bool GetVariables(RE::ActiveEffect* src, string scriptName, vector<string> varNames, vector<RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object> &object);


	
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, std::map<string, RE::BSScript::Variable*> &properties_out, RE::BSScript::Internal::VirtualMachine * vm = nullptr);
	bool GetProperties(RE::TESForm* form, string scriptName, vector<string> propNames, std::map<string, RE::BSScript::Variable*> &properties_out, RE::BSTSmartPointer<RE::BSScript::Object> &object);
	bool GetVariables(RE::TESForm* form, string scriptName, vector<string> varNames, std::map<string, RE::BSScript::Variable*>& variables_out, RE::BSScript::Internal::VirtualMachine* vm);
	bool GetVariables(RE::TESForm* form, string scriptName, vector<string> varNames, std::map<string, RE::BSScript::Variable*>& variables_out, RE::BSTSmartPointer<RE::BSScript::Object>& object);

	bool CallFunc(RE::BSTSmartPointer<RE::BSScript::Object>& object, string fnName, RE::BSScript::Internal::VirtualMachine* vm, RE::BSScript::IFunctionArguments* args);
	template <class... Args>
	inline bool CallFunc(RE::BSTSmartPointer<RE::BSScript::Object>& object, string fnName, RE::BSScript::Internal::VirtualMachine* vm, Args&... args) {
		return CallFunc(object, fnName, vm, RE::MakeFunctionArguments(std::forward<Args>(args)...));
	}
	template <class... Args>
	inline bool CallFunc(RE::BSTSmartPointer<RE::BSScript::Object>& object, string fnName, Args&... args) {
		return CallFunc(object, fnName, nullptr, RE::MakeFunctionArguments(std::forward<Args>(args)...));
	}
	inline bool CallFunc(RE::BSTSmartPointer<RE::BSScript::Object>& object, string fnName, RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
		return CallFunc(object, fnName, vm, RE::MakeFunctionArguments());
	}

	RE::BSScript::Variable* GetVarFromFrame(RE::VMStackID stackID, u32 frameDepth, u32 varIdx, RE::BSScript::Internal::VirtualMachine* vm = nullptr);

	RE::BSScript::ObjectTypeInfo* VarGetInfo(RE::BSScript::Variable* var);
	RE::BSScript::ObjectTypeInfo* VarGetArrInfo(RE::BSScript::Variable* var);
	bool VarIsIntArr(RE::BSScript::Variable* var);
	bool VarIsFloatArr(RE::BSScript::Variable* var);
	bool VarIsBoolArr(RE::BSScript::Variable* var);
	bool VarIsStringArr(RE::BSScript::Variable* var);
	bool VarIsClass(RE::BSScript::Variable* var, const string& classname);
	bool ArrVarIsClass(RE::BSScript::Variable* var, const string& classname);
	
	string VarInfo(RE::BSScript::Variable* var);
	bool PrintVars(RE::BSTSmartPointer<RE::BSScript::Object>& object);
	
}
