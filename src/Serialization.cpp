#include "Serialization.h"
#include "Logger.h"
#include "Utils/PrimitiveUtils.h"
#include "Data.h"
#include "ExtraKeywords.h"
#include "MCM.h"
#include "Periodic.h" // Hacky stop on Revert call back


namespace Serialization {

	void SaveCallback(SKSE::SerializationInterface* intfc) {
		if (!intfc) {
			return;
		}
		if (!Data::Shared::Save(*intfc)) { Log::Critical("Failed to serialize Actor data!"sv); }
		if (!ExtraKeywords::Data::Save(*intfc)) { Log::Critical("Failed to serialize ExtraKeywords data!"sv); }

		Log::Info("Serialized data"sv);
	}

	void LoadCallback(SKSE::SerializationInterface* intfc) {
		if (!intfc) {
			return;
		}
		u32 type;
		u32 version;
		u32 length;
		while (intfc->GetNextRecordInfo(type, version, length)) {
			switch (type) {
			case (Data::Shared::SerializationType):
				Data::Shared::Load(*intfc, version);
				continue;
			case (ExtraKeywords::Data::SerializationType):
				if (version != ExtraKeywords::Data::SerializationVersion) { Log::Critical("ExtraKeywords data is out of date! Read {}, expected {}"sv, version, ExtraKeywords::Data::SerializationVersion); }
				else if (!ExtraKeywords::Data::Load(*intfc)) { Log::Critical("Failed to deserialize ExtraKeywords data!"sv); }
				continue;
			default: Log::Critical("Unrecognized type of deserialized record {}!"sv, PrimitiveUtils::u32_str(type));
			}
		}
		Log::Info("Deserialized data"sv);
	}

	void RevertCallback(SKSE::SerializationInterface*) {
		// Happens every time the game tries to load a save. So: Load a save -> This fires -> (from ingame)"Back to Main Menu" -> This fires -> Main Menu loads,  or  Load a save -> This fires -> (from ingame)Load any save -> This fires -> New save loads
		// Basically it fires once from Main Menu to Game, from Game to Main Menu, and from Game to Game. SKSE hooks a native function that fires in these events and calls all Revert callbacks registered in Serialization.
		// Since I can't see any other way to listen to those events I'll be hitchhiking on this to handle them. After all, it is SKSE that specializes this callback for Serialization. The main function seems to be discarding any already loaded data before loading a save.
		Periodic::Timer::Stop(); // No prob even if they are not running

		Data::Shared::Revert();
		ExtraKeywords::Data::Revert();

		Log::Info("Reverted serialization"sv);
	}


}


