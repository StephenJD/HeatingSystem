#pragma once
#include <I2C_Talk.h>
#include <I2C_RecoverRetest.h>
#include <RDB.h>
#include <MultiCrystal.h>
#include "HardwareInterfaces\LocalDisplay.h"
#include "HardwareInterfaces\ConsoleController_Thick.h"
#include <LocalKeypad.h>
#include "HardwareInterfaces\Console_Thin.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include <Mix_Valve.h>
#include <OLED_Thick_Display.h>
#include "Assembly\Initialiser.h"
#include "Assembly\TemperatureController.h"
#include "Assembly\HeatingSystem_Queries.h"
#include "Assembly\Datasets.h"
#include "Assembly\HeatingSystemEnums.h"
#include "Assembly\MainConsoleChapters.h"
#include "Assembly/Sequencer.h"

//////////////////////////////////////////////////////////
//    Single Responsibility is to connect the parts     //
//////////////////////////////////////////////////////////

namespace HeatingSystemSupport {
	void initialise_virtualROM();
	extern bool dataHasChanged;
}

class HeatingSystem {
public:
	HeatingSystem();
	void run_stateMachine();
	bool serviceConsoles_OK();

	/// <summary>
	/// Checks Zone Temps, then sets each zone.nextEvent to now.
	/// </summary>
	auto recoverObject() -> I2C_Recovery::I2C_Recover_Retest & { return _recover; }
	// For testing:...
	Assembly::MainConsoleChapters & mainConsoleChapters() { return _mainConsoleChapters; }
	RelationalDatabase::RDB<Assembly::TB_NoOfTables> & getDB();
	Assembly::HeatingSystem_Queries & getQueries() { return _hs_queries; }
	Assembly::TemperatureController & tempController() { return _tempController; }
	enum State {CHECK_I2C_COMS, TUNE_MIXV, START_NEW_DAY, SERVICE_SEQUENCER, SERVICE_BACKBOILER, SERVICE_TEMP_CONTROLLER, SERVICE_CONSOLES};
private: // data-member ordering matters!
	State _state = START_NEW_DAY;
	void updateChangedData();
	bool consoleDataHasChanged();
	bool serviceMainConsole();
	I2C_Talk_ZX i2C{ HardwareInterfaces::PROGRAMMER_I2C_ADDR, Wire, HardwareInterfaces::I2C_MAX_SPEED };
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
#ifdef ZPSIM
	State state() { return _state; }
	void set_MFB_temp(bool on);
#endif
	// Public Data Members
	HardwareInterfaces::LocalDisplay mainDisplay;
	HardwareInterfaces::LocalKeypad localKeypad;
	HardwareInterfaces::ConsoleController_Thick thickConsole_Arr[Assembly::NO_OF_REMOTE_DISPLAYS];
private: 
	friend Assembly::Initialiser;
	friend class HardwareInterfaces::TestDevices;
	// Run-time data arrays
	Assembly::MainConsoleChapters _mainConsoleChapters;
	HardwareInterfaces::Console_Thin _mainConsole;
	bool _dataHasChanged = true;
};
