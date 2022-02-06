#pragma once

#include <I2C_RecoverStrategy.h>

#include <Arduino.h>
#include <Date_Time.h>

#define EEPROM_CLOCK_ADDR 0
static constexpr int EEPROM_CLOCK_SIZE = sizeof(Date_Time::DateTime) * 2 + 4;

namespace HardwareInterfaces {

	extern long actualLoopTime;
	extern long mixCheckTime;
	extern long zoneCheckTime;
	extern long storeCheckTime;

	typedef int8_t S1_err;

	////////////////// Program Version /////////////////////
	constexpr uint8_t VERSION = 71;

	/////////////////// Pin assignments & misc. ///////////////////
	constexpr uint8_t RESET_OUT_PIN = 14;  // active low.
	constexpr uint8_t RESET_LEDP_PIN = 16;
	constexpr uint8_t RESET_LEDN_PIN = 19;

	constexpr uint8_t KEYPAD_INT_PIN = 18;
	constexpr uint8_t KEYPAD_ANALOGUE_PIN = A1;
	constexpr uint8_t BATT_ANALOGUE = A2;
	constexpr uint8_t KEYPAD_REF_PIN = A3;
	constexpr uint8_t RESET_5vREF_PIN = A4;

	constexpr uint8_t ZERO_CROSS_PIN = 15; // active falling edge.
	constexpr uint16_t ZERO_CROSS_DELAY = 690;

	/////////////////// No Of Registers ///////////////////////////////
	constexpr int NO_OF_MIXERS = 2;
	constexpr int NO_OF_REMOTES = 3;

	/////////////////// Mixing Valves ///////////////////////////////
	constexpr uint8_t VALVE_WAIT_TIME = 60;
	constexpr uint8_t VALVE_TRANSIT_TIME = 255;
	////////////////// EEPROM USAGE /////////////////////
	// clock : 6 bytes
	// strategy : 11 bytes
	// RDB : 1000 bytes
	// EEPROM_Log : top of EEPROM
#if defined(__SAM3X8E__)
	constexpr uint16_t EEPROM_SIZE = 32000;
	constexpr uint16_t EEPROM_LOG_SIZE = 2000;
#else
	constexpr uint16_t EEPROM_SIZE = 4096;
	constexpr uint16_t EEPROM_LOG_SIZE = 2000;
#endif
	constexpr uint16_t STRATEGY_EPPROM_ADDR = EEPROM_CLOCK_ADDR + EEPROM_CLOCK_SIZE;
	constexpr uint16_t RDB_START_ADDR = STRATEGY_EPPROM_ADDR + I2C_Recovery::S_NoOfStrategies + 1;
	constexpr uint16_t RDB_MAX_SIZE = 2000;
	constexpr uint16_t EEPROM_LOG_END = EEPROM_SIZE;
	constexpr uint16_t EEPROM_LOG_START = EEPROM_LOG_END - EEPROM_LOG_SIZE;

	////////////////// EVENT CODES ////////////////////////
	//extern bool temp_sense_hasError;
	//const S1_err TEMP_SENS_ERR_TEMP		= -30;

	constexpr S1_err ERR_PORTS_FAILED = 1;
	constexpr S1_err ERR_READ_VALUE_WRONG = 2;
	constexpr S1_err ERR_I2C_RESET = 3;
	constexpr S1_err ERR_I2C_SPEED_FAIL = 4;
	constexpr S1_err ERR_I2C_READ_FAILED = 5;
	constexpr S1_err ERR_I2C_SPEED_RED_FAILED = 6;
	constexpr S1_err EVT_I2C_SPEED_RED = 7;
	constexpr S1_err ERR_MIX_ARD_FAILED = 8;
	constexpr S1_err ERR_OP_NOT_LAT = 9;
	constexpr S1_err EVT_THS_COND_CHANGE = 10;
	constexpr S1_err EVT_TEMP_REQ_CHANGED = 11;
	constexpr S1_err ERR_TEMP_SENS_READ = 12;
	constexpr S1_err ERR_RTC_FAILED = 13;
	constexpr S1_err EVT_GAS_TEMP = 15;

	//*******************************
	enum modes { e_onChange_read_LATafter, e_onChange_read_OUTafter, e_alwaysWrite_read_LATafter, e_alwaysCheck_read_OUTafter, e_alwaysCheck_read_latBeforeWrite, e_alwaysCheck_read_OutBeforeWrite, noOfSwModes };

	////////////////////////// I2C Setup /////////////////////////////////////
	//  I2C device address is 0 0 1 0   0 A2 A1 A0
	//  Mine are at 010 0000 and 010 0111
	// IO Device Addresses

	constexpr int32_t I2C_MAX_SPEED = 100000;
	constexpr uint8_t EEPROM_I2C_ADDR = 0x50;
	constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;
	constexpr uint8_t MIX_VALVE_I2C_ADDR = 0x10;
	constexpr uint8_t US_CONSOLE_I2C_ADDR = 0x12;
	constexpr uint8_t DS_CONSOLE_I2C_ADDR = 0x13;
	constexpr uint8_t FL_CONSOLE_I2C_ADDR = 0x14;
	constexpr uint8_t REMOTE_CONSOLE_ADDR[] = {US_CONSOLE_I2C_ADDR, DS_CONSOLE_I2C_ADDR, FL_CONSOLE_I2C_ADDR };
	constexpr uint8_t US_FLOW_TEMPSENS_ADDR = 0x2C;
	constexpr uint8_t DS_FLOW_TEMPSENS_ADDR = 0x4f;
	constexpr uint8_t US_ROOM_TEMPSENS_ADDR = 0x36;
	constexpr uint8_t DS_ROOM_TEMPSENS_ADDR = 0x74;
	constexpr uint8_t FL_ROOM_TEMPSENS_ADDR = 0x70;
	constexpr uint8_t REMOTE_ROOM_TS_ADDR[] = { US_ROOM_TEMPSENS_ADDR, DS_ROOM_TEMPSENS_ADDR, FL_ROOM_TEMPSENS_ADDR };

	//constexpr uint8_t US_REMOTE_ADDRESS = 0x24;
	//constexpr uint8_t FL_REMOTE_ADDRESS = 0x25;
	//constexpr uint8_t DS_REMOTE_ADDRESS = 0x26;
	constexpr uint8_t IO8_PORT_OptCoupl = 0x20;
	constexpr uint8_t IO8_PORT_MixValves = 0x27;
	constexpr uint8_t IO8_PORT_OptCouplActive = 0;
	constexpr uint8_t IO8_PORT_MixValvesActive = 0;
	constexpr uint8_t MIX_VALVE_USED = 128 | 8 | 2 | 1;

	// Hardware Registers
	constexpr uint8_t REG_16PORT_INTCAPA = 0x10;
	constexpr uint8_t REG_16PORT_GPIOA = 0x12;

	constexpr uint8_t DS75LX_Temp = 0x00; // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
	constexpr uint8_t DS75LX_Config = 0x01;
	constexpr uint8_t DS75LX_HYST_REG = 0x02;
	constexpr uint8_t DS75LX_LIMIT_REG = 0x03;

	//////////////////// Display ////////////////////////////////
	//constexpr uint8_t SECONDS_ON_VERSION_PAGE = 5;
	constexpr char NEW_LINE_CHR = '~'; // character used to indicate this field to start on a new line
	constexpr char CONTINUATION = '_'; // character used to indicate where a field can split over lines
	constexpr uint8_t MAX_LINE_LENGTH = 20;
	constexpr uint8_t MAX_NO_OF_ROWS = 4;
	constexpr uint8_t ADDITIONAL_BUFFER_SPACE = 3; // 1st char is cursor position, allow for space following string, last is a null.
	constexpr uint8_t DISPLAY_BUFFER_LENGTH = (MAX_LINE_LENGTH + ADDITIONAL_BUFFER_SPACE) * MAX_NO_OF_ROWS;
	extern char SPACE_CHAR;// = ' ';  // temporarily made extern to Collection_Utilities.cpp so it can be redefined by gtest
	constexpr char LIST_LEFT_CHAR = '<';
	constexpr char LIST_RIGHT_CHAR = '>';
	constexpr char EDIT_SPC_CHAR = '|';
	constexpr uint8_t MIN_BL = 125;
	constexpr uint8_t MAX_BL = 255;
	constexpr int8_t EDIT_TIME = 60;

	////////////////////// Relay testing order ///////////////////
	//								  0		 1		 2		3	   4	  5		6			
	//name[RelayCnt][RELAY_N+1] =	{"Flat","FlTR",	"HsTR","UpSt","MFSt","Gas","DnSt"};
	//port[RelayCnt] =			    { 6,	 1,		 0,		5,	   2,	  3,	4};

	constexpr uint8_t RELAY_ORDER[] = { 1,2,4,5,6,3,0 }; // relay indexes

	////////////////////// Thermal Store Time-Temps ///////////////////
	constexpr uint8_t THERM_STORE_HYSTERESIS = 5;
	constexpr uint8_t TT_SHORTEN_TIME = 20;
	constexpr uint8_t TT_LOWER_TEMP = 5;
	constexpr uint8_t TT_NEW_TT_LENGTH = 10;
	constexpr uint8_t TT_PROXIMITY = 30; // should be more than TT_NEW_TT_LENGTH

	////////////////////// Zones ///////////////////
	constexpr uint8_t MIN_ON_TEMP = 10;
	constexpr uint8_t MAX_REQ_TEMP = 80;
	constexpr uint8_t FLOW_TEMP_SENSITIVITY = 32; // no of degrees flow temp changes per degree room-temp error
	constexpr uint8_t MIN_FLOW_TEMP = 25; // Flow temp below which pump turns off.
	constexpr int16_t MAX_MIN_TEMP_ERROR = 256;

#if defined (ZPSIM)
	constexpr long AVERAGING_PERIOD = 100; // seconds
#else
	constexpr long AVERAGING_PERIOD = 600; // seconds
#endif 

////////////////////// Towel Rails ///////////////////
	constexpr uint16_t TWL_RAD_RISE_TIME = 180; // seconds
	constexpr uint16_t TWL_RAD_RISE_TEMP = 5; // degrees

	////////////////////// Argument Constants ///////////////////
	//const int8_t NOT_A_LIST = -2;
	//const int8_t LIST_NOT_ACTIVE = -1;
	//const S1_newPage DO_EDIT = -1;
	//const S1_newPage VIEWED_DT = -2;
	//const int8_t CHECK_FROM = -2;
	//const int8_t CURRENT_DT = -3;
	//const uint8_t SPARE_TT = 200;
	//// Binary OR'ed flags set in Field Count to indicate field mode.
	//const uint8_t HIDDEN = 1;	// %2=1
	//const uint8_t NEW_LINE = 2;   // &2=1		
	//const uint8_t SELECTABLE = 4;
	//const uint8_t CHANGEABLE = 8;
	//const uint8_t EDITABLE = 16;

	////////////////// Strings //////////////////
	//const char NEW_DP_NAME[] = "New Prog";

	//extern MultiCrystal * mainLCD;
	//extern MultiCrystal * hallLCD;
	//extern MultiCrystal * usLCD;
	//extern MultiCrystal * flatLCD;
	//extern bool processReset;

	//////////// here to avoid circular header dependancy
	//enum noItemsVals { noDisplays, noTempSensors, noRelays, noMixValves, noOccasionalHts, noZones, noTowelRadCcts, noAllDPs, noAllDTs, noAllTTs, NO_COUNT };

	//enum StartAddr {
	//	INI_ST, INFO_ST, EVENT_ST,
	//	NO_OF_ST = 6,
	//	NO_OF_DTA = NO_OF_ST + 2, // = 8
	//	THM_ST_ST = NO_OF_DTA + NO_COUNT, // = 18
	//	DISPLAY_ST = THM_ST_ST + 2,
	//	RELAY_ST = DISPLAY_ST + 2, // = 22
	//	MIXVALVE_ST = RELAY_ST + 2,
	//	TEMP_SNS_ST = MIXVALVE_ST + 2,
	//	OCC_HTR_ST = TEMP_SNS_ST + 2,
	//	ZONE_ST = OCC_HTR_ST + 2,
	//	TWLRAD_ST = ZONE_ST + 2, // = 32
	//	DATE_TIME_ST = TWLRAD_ST + 2,
	//	DAYLY_PROG_ST = DATE_TIME_ST + 2, // =36
	//	TIME_TEMP_ST = DAYLY_PROG_ST + 2
	//}; // = 38

/* Globals
Name	Meaning									Declared			Initialised
rtc		I2C_Talk for Real-Time_Clock/EEprom	Setup_Loop			setup()
i2C		I2C_Talk for Relays/Temp Sensors		Relay_Run			Factory::initialiseProgrammer()
mainLCD	MultiCrystal for LCD					A__Constants		A__Constants
f		factoryObjects							D_Factory			createFactObject()
*/
/* Helper macros */
//
//#define HEX__(n) 0x##n##LU
//#define B8__(x) ((x&0x0000000FLU)?1:0) \
//+((x&0x000000F0LU)?2:0) \
//+((x&0x00000F00LU)?4:0) \
//+((x&0x0000F000LU)?8:0) \
//+((x&0x000F0000LU)?16:0) \
//+((x&0x00F00000LU)?32:0) \
//+((x&0x0F000000LU)?64:0) \
//+((x&0xF0000000LU)?128:0)
//
///* User macros */
//#define B8(d) ((unsigned char)B8__(HEX__(d)))
//

}