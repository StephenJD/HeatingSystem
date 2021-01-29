#pragma once
#include <I2C_Talk.h>
#include <I2C_RecoverRetest.h>
#include <RDB.h>
#include <MultiCrystal.h>
#include "HardwareInterfaces\LocalDisplay.h"
#include <LocalKeypad.h>
#include <RemoteKeypad.h>
#include <RemoteDisplay.h>
#include "HardwareInterfaces\Console.h"

#include "Assembly\Initialiser.h"
#include "Assembly\TemperatureController.h"
#include "Assembly\HeatingSystem_Queries.h"
#include "Assembly\Datasets.h"
#include "Assembly\HeatingSystemEnums.h"
#include "Assembly\MainConsoleChapters.h"
#include "Assembly\RemoteConsoleChapters.h"
#include "Assembly/Sequencer.h"

//////////////////////////////////////////////////////////
//    Single Responsibility is to connect the parts     //
//////////////////////////////////////////////////////////
namespace HeatingSystemSupport {
	void initialise_virtualROM();
}

class HeatingSystem {
public:
	HeatingSystem();
	void serviceConsoles();
	void serviceTemperatureController();
	/// <summary>
	/// Checks Zone Temps, then sets each zone.nextEvent to now.
	/// </summary>
	void notifyDataIsEdited();
	auto recoverObject() -> I2C_Recovery::I2C_Recover_Retest & { return _recover; }
	// For testing:...
	Assembly::MainConsoleChapters & mainConsoleChapters() { return _mainConsoleChapters; }
	RelationalDatabase::RDB<Assembly::TB_NoOfTables> & getDB();
	Assembly::HeatingSystem_Queries & getQueries() { return _hs_queries; }

private: // data-member ordering matters!
	I2C_Talk_ZX i2C;
	I2C_Recovery::I2C_Recover_Retest _recover;
	RelationalDatabase::RDB<Assembly::TB_NoOfTables> db;
	Assembly::Initialiser _initialiser; // Checks db
	Assembly::HeatingSystem_Queries _hs_queries;
	Assembly::HeatingSystem_Datasets _hs_datasets;
	Assembly::Sequencer _sequencer;
	Assembly::TemperatureController _tempController;
public:	
	// Public Data Members
	HardwareInterfaces::LocalDisplay mainDisplay;
	HardwareInterfaces::LocalKeypad localKeypad;
	HardwareInterfaces::RemoteDisplay remDispl[Assembly::NO_OF_REMOTE_DISPLAYS];
	HardwareInterfaces::RemoteKeypad remoteKeypad[Assembly::NO_OF_REMOTE_DISPLAYS];
private: 
	friend Assembly::Initialiser;
	friend class HardwareInterfaces::TestDevices;
	// Run-time data arrays
	Assembly::MainConsoleChapters _mainConsoleChapters;
	Assembly::RemoteConsoleChapters _remoteConsoleChapters;
	HardwareInterfaces::Console _mainConsole;
	HardwareInterfaces::Console _remoteConsole[Assembly::NO_OF_REMOTE_DISPLAYS];
};
