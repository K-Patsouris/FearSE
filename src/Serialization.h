#pragma once
#include "Common.h"


namespace Serialization {
	constexpr u32 PluginSerializationType { 'FEAR' };

	void SaveCallback(SKSE::SerializationInterface* intfc);
	void LoadCallback(SKSE::SerializationInterface* intfc);
	void RevertCallback(SKSE::SerializationInterface*);

}















