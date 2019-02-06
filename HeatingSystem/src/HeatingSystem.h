#pragma once
#include "Assembly\HeatingSystemEnums.h"
#include "Assembly\TestDevices.h"
#include "Assembly\Initialiser.h"
#include "Assembly\MainConsolePageGenerator.h"
#include "Assembly\TemperatureController.h"

//#include "Client_DataStructures\Data_TempSensor.h"
//#include "Client_DataStructures\Data_MixValveControl.h"

#include "HardwareInterfaces\RemoteDisplay.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include "HardwareInterfaces\LocalDisplay.h"
#include "HardwareInterfaces\LocalKeypad.h"
#include "HardwareInterfaces\Console.h"
#include "HardwareInterfaces\LocalDisplay.h"
#include "Assembly/Sequencer.h"
#include <I2C_Helper.h>
#include <RDB.h>


//////////////////////////////////////////////////////////
//    Single Responsibility is to connect the parts     //
//////////////////////////////////////////////////////////

	class HeatingSystem {
	public:

		HeatingSystem();
		void serviceConsoles();
		void serviceProfiles();
		void serviceTemperatureController() { _tempController.checkAndAdjust(); }

	private:
		// Public Data Members
		RelationalDatabase::RDB<Assembly::TB_NoOfTables> db;
		Assembly::Initialiser _initialiser;
	public:	HardwareInterfaces::LocalDisplay mainDisplay;
		HardwareInterfaces::LocalKeypad localKeypad;
	private: I2C_Helper_Auto_Speed<27> i2C;

		// Run-time data arrays
		HardwareInterfaces::RemoteDisplay remDispl[Assembly::NO_OF_REMOTE_DISPLAYS];
		friend Assembly::Initialiser;
		friend HardwareInterfaces::TestDevices;
		RelationalDatabase::TableQuery _q_displays;
		Assembly::TemperatureController _tempController;
		Assembly::MainConsolePageGenerator _mainPages;
		Assembly::Sequencer _sequencer;
		HardwareInterfaces::Console _mainConsole;
	};
