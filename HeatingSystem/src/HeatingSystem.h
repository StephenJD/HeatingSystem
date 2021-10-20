#pragma once
#include <I2C_Talk.h>
#include <I2C_RecoverRetest.h>
#include <RDB.h>
#include <MultiCrystal.h>
#include "HardwareInterfaces\LocalDisplay.h"
#include "HardwareInterfaces\RemoteConsole.h"
#include <LocalKeypad.h>
#include <RemoteKeypad.h>
#include <RemoteDisplay.h>
#include "HardwareInterfaces\Console.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include <Mix_Valve.h>
#include <OLED_Master_Display.h>
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

constexpr bool OLED_DS = true;
constexpr bool ALL_OLD_REMOTES = true;

namespace HeatingSystemSupport {
	void initialise_virtualROM();
	extern bool dataHasChanged;
}

class HeatingSystem {
public:
	HeatingSystem();
	void serviceConsoles();
	void serviceTemperatureController();
	/// <summary>
	/// Checks Zone Temps, then sets each zone.nextEvent to now.
	/// </summary>
	void updateChangedData();
	auto recoverObject() -> I2C_Recovery::I2C_Recover_Retest & { return _recover; }
	// For testing:...
	Assembly::MainConsoleChapters & mainConsoleChapters() { return _mainConsoleChapters; }
	RelationalDatabase::RDB<Assembly::TB_NoOfTables> & getDB();
	Assembly::HeatingSystem_Queries & getQueries() { return _hs_queries; }
	Assembly::TemperatureController & tempController() { return _tempController; }
private: // data-member ordering matters!
	I2C_Talk_ZX i2C{ HardwareInterfaces::PROGRAMMER_I2C_ADDR };
	I2C_Recovery::I2C_Recover_Retest _recover;
	RelationalDatabase::RDB<Assembly::TB_NoOfTables> db;
#ifdef ZPSIM
	i2c_registers::Registers<HardwareInterfaces::SIZE_OF_ALL_REGISTERS, HardwareInterfaces::PROGRAMMER_I2C_ADDR> _prog_register_set{i2C};
#else
	i2c_registers::Registers<HardwareInterfaces::SIZE_OF_ALL_REGISTERS> _prog_register_set{i2C};
#endif
	i2c_registers::I_Registers& _prog_registers = _prog_register_set;
	Assembly::Initialiser _initialiser; // Checks db
	Assembly::HeatingSystem_Queries _hs_queries;
	Assembly::HeatingSystem_Datasets _hs_datasets;
	Assembly::Sequencer _sequencer;
	Assembly::TemperatureController _tempController;
public:	
	// Temporary Remote TS
	HardwareInterfaces::UI_TempSensor temporary_remoteTSArr[3];
	// Public Data Members
	HardwareInterfaces::LocalDisplay mainDisplay;
	HardwareInterfaces::LocalKeypad localKeypad;
	HardwareInterfaces::RemoteDisplay remDispl[Assembly::NO_OF_REMOTE_DISPLAYS];
	HardwareInterfaces::RemoteKeypad remoteKeypadArr[Assembly::NO_OF_REMOTE_DISPLAYS];
	HardwareInterfaces::RemoteConsole remOLED_ConsoleArr[Assembly::NO_OF_REMOTE_DISPLAYS];
private: 
	friend Assembly::Initialiser;
	friend class HardwareInterfaces::TestDevices;

	// Run-time data arrays
	Assembly::MainConsoleChapters _mainConsoleChapters;
	Assembly::RemoteConsoleChapters _remoteConsoleChapters;
	HardwareInterfaces::Console _mainConsole;
	HardwareInterfaces::Console _remoteLCDConsole[Assembly::NO_OF_REMOTE_DISPLAYS];
	bool _dataHasChanged = true;
};
