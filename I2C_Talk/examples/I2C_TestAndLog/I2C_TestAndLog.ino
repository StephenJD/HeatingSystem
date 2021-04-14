#include <TempSensor.h>
#include <RemoteDisplay.h>
#include <LocalKeypad.h>
#include <RemoteKeypad.h>
#include <Relay_Bitwise.h>

#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_Scan.h>
#include <I2C_RecoverRetest.h>
#include <I2C_RecoverStrategy.h>
#include <PinObject.h>

#include <EEPROM.h>
#include <Clock_I2C.h>
#include <Logging.h>
#include <Wire.h>

#include <SD.h>

#include <MultiCrystal.h>
#include <Date_Time.h>
#include <Conversions.h>
#include <Timer_mS_uS.h>
#include <MemoryFree.h>

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;
using namespace HardwareInterfaces;

const uint8_t LOCAL_INT_PIN = 18;
const uint32_t SERIAL_RATE = 115200;
const uint8_t RTC_RESET_PIN = 4;

void ui_yield() {}

constexpr uint8_t ZERO_CROSS_PIN = 15; // active falling edge.
constexpr uint16_t ZERO_CROSS_DELAY = 690;
const uint8_t RESET_OUT_PIN = 14;  // active LOW.
const uint8_t RESET_LEDP_PIN = 16;  // high supply
const uint8_t RESET_LEDN_PIN = 19;  // low for on.
const uint8_t RTC_ADDR = 0x68;
const uint8_t EEPROM_CLOCK_SIZE = 0x8;
const uint8_t EEPROM_ADDRESS = 0x50;
const int EEPROM_CLOCK_ADDR = 0;
const int STRATEGY_EPPROM_ADDR = EEPROM_CLOCK_ADDR + EEPROM_CLOCK_SIZE;
const int EEPROM_LOG_START = STRATEGY_EPPROM_ADDR + S_NoOfStrategies + 1;
const int EEPROM_LOG_END = 4096;
const uint8_t RELAY_PORT_ADDRESS = 0x20;
const uint8_t MIX_VALVE_ADDRESS = 0x10;
// registers
const uint8_t DS75LX_HYST_REG = 2;
const uint8_t REG_8PORT_IODIR = 0x00;
const uint8_t REG_8PORT_PullUp = 0x06;
const uint8_t REG_8PORT_OPORT = 0x09;
const uint8_t REG_8PORT_OLAT = 0x0A;
const uint8_t DS75LX_Temp = 0x00; // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
const uint8_t DS75LX_Config = 0x01;

uint8_t BRIGHNESS_PWM = 5; // pins 5 & 6 are not best for PWM control.
uint8_t CONTRAST_PWM = 6;
uint8_t PHOTO_ANALOGUE = A0;

#if defined(__SAM3X8E__)
	#define KEYINT LOCAL_INT_PIN
	const uint8_t KEY_ANALOGUE = A1;
	const uint8_t KEYPAD_REF_PIN = A3;
	I2C_Talk rtc{ Wire1 }; // not initialised until this translation unit initialised.

	EEPROMClass & eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1,100000),rtc.extendTimeouts(5000, 5, 1000), EEPROM_ADDRESS) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}

	Clock & clock_() {
		static Clock_I2C<rtc> _clock((rtc.ini(Wire1, 100000), rtc.extendTimeouts(5000, 5, 1000), RTC_ADDR));
		return _clock;
	}

	const float megaFactor = 1;
	const char * LOG_FILE = "Due.txt"; // max: 8.3
	const char * EP_FILE = "EP.txt"; // max: 8.3
	auto rtc_reset_wag = Pin_Wag{ RTC_RESET_PIN, HIGH };

	uint8_t resetRTC(I2C_Talk & i2c) {
		logger() << "\nPower-Down RTC...\n";
		rtc_reset_wag.set();
		delay(200); // interrupts still serviced
		rtc_reset_wag.clear();
		i2c.begin();
		delay(200);
		return true;
	}
#else
	//Code in here will only be compiled if an Arduino Mega is used.

	#define NO_RTC
	#define EEPROM_CLOCK EEPROM_CLOCK_ADDR
	#include <Clock_EP.h>
	#define KEYINT 5
	const uint8_t KEY_ANALOGUE = 0;
	const uint8_t KEYPAD_REF_PIN = A2;

	EEPROMClass & eeprom() {
		return EEPROM;
	}
	const float megaFactor = 3.3 / 5;
	const char * LOG_FILE = "Meg.txt"; // max: 8.3
#endif

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	//static SD_Logger _log(LOG_FILE, SERIAL_RATE);
	return _log;
}

I_I2Cdevice_Recovery & getDevice(int addr);
Error_codes hardReset_Performed_(I2C_Talk & i2c, int addr);
uint8_t initialiseRemoteDisplaysFailed_();
uint8_t tryDeviceAt(int addrIndex);
bool yieldFor(int32_t delayMillis);
int deviceIndex(int addr);
void printSpeed();
void prepareDisplay();
void fullSpeedTest();
I_I2Cdevice_Recovery & getDevice(int addr);
uint8_t initialiseRemoteDisplaysFailed_();

I2C_Talk_ZX i2c{ Wire, 2300000 }; // Wrapper not required - doesn't solve Wire not being constructed yet.
//I2C_Talk i2c{ Wire, 2300000 }; // Wrapper not required - doesn't solve Wire not being constructed yet.

I2C_Recover i2c_recover{ i2c }; // Wrapper not required - doesn't solve Wire not being constructed yet.

I_I2C_SpeedTestAll i2c_test{ i2c, i2c_recover };

enum { e_secs, e_mins, e_hrs, e_spare, e_day, e_mnth, e_yr, e_size };
byte datetime[e_size];
const byte TEMP_REG = 0;
const byte TEMP_HYST_REG = 2;
const byte TEMP_LIMIT_REG = 3;
int tryCount = 0;
byte i2cAddrIndex = 0;
bool cycleDevices = true;
unsigned long timeOfReset_mS_;
bool initialisationRequired_ = false;
MultiCrystal * mainLCD(0);

unsigned int loopTime[] = { 0, 100,200,500,1000,2000,5000,10000,20000,40000 };
byte loopTimeIndex = 0;
enum { normal, resetBeforeTest, speedTestExistsOnly, NO_OF_MODES };
byte testMode = normal;

//const char relayName[][5] = { "F-TR", "HsTR", "MFSt", "Gas", "Down", "UpS", "Flat" };
uint8_t relaySequence[] = { 0x7d,0x7e,0x7b,0x77,0x6f,0x5f,0x3f, 0x7c,0x7a,0x73,0x67,0x4f,0x1f,0x7e, 0x79,0x76,0x6b,0x57,0x2f,0x69,0x56,0x2b };

//byte i2cAddr[] =   {0x24, 0x36 };
//byte i2cAddr[] =   {0x26, 0x74 };
//byte i2cAddr[] =   {0x10, 0x20, 0x28, 0x29, 0x4F, 0x74 };
//byte i2cAddr[] =   { 0x28,0x2B,0x2C,0x2D,0x2E,0x2F,0x35,0x36,0x37,0x48,0x4B,0x4F,0x70,0x71,0x74,0x75,0x76,0x77 };
byte i2cAddr[] = { 0x10,0x20,0x24,0x25,0x26,0x28,0x2B,0x2C,0x2D,0x2E,0x2F,0x35,0x36,0x37,0x48,0x4B,0x4F,0x70,0x71,0x74,0x75,0x76,0x77 };
int successCount[sizeof(i2cAddr)] = { 0 };
unsigned long lastTimeGood[sizeof(i2cAddr)] = { 0 };

//TempSensor tempSens[] = { {i2c_recover, 0x36}};
//TempSensor tempSens[] = { {i2c_recover, 0x74}};
//TempSensor tempSens[] = { {i2c_recover, 0x28}, 0x29, 0x4F, 0x74 };
TempSensor tempSens[] = { {i2c_recover, 0x28},0x2B,0x2C,0x2D,0x2E,0x2F,0x35,0x36,0x37,0x48,0x4B,0x4F,0x70,0x71,0x74,0x75,0x76,0x77 };
const byte FIRST_TEMP_SENS_ADDR = tempSens[0].getAddress();

class RelaysPortSequence : public RelaysPort {
public:
	using RelaysPort::RelaysPort;
	Error_codes testDevice() override;
	Error_codes testRelays();
	Error_codes nextSequence();
};

namespace HardwareInterfaces {
	Bitwise_RelayController & relayController() {
		static RelaysPortSequence _relaysPort(0x7F, i2c_recover, RELAY_PORT_ADDRESS);
		return _relaysPort;
	}
	RelaysPortSequence & rPort = static_cast<RelaysPortSequence &>(relayController());
}

class MixValveController : public I_I2Cdevice_Recovery {
public:
	enum Registers {
		// All registers are treated as two-byte, with second-byte un-used for most. Thus single-byte read-write works.
		/*Read Only Data*/		status,
		/*Read Only Data*/		mode, count, valve_pos, state = valve_pos + 2,
		/*Read/Write Data*/		flow_temp, request_temp, ratio, moveFromTemp, moveFromPos,
		/*Read/Write Config*/	temp_i2c_addr, traverse_time, wait_time, max_flow_temp, eeprom_OK1, eeprom_OK2,
		/*End-Stop*/			reg_size
	};
	using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
	// Virtual Functions
	uint8_t getPos(uint8_t & pos);
	Error_codes testDevice() override;
private:
	unsigned long * _timeOfReset_mS = 0;
	int _error = 0;
};

MixValveController mixValve{ i2c_recover, 0x10 };

Relay_B relays[] = { {1,0},{0,0},{2,0},{3,0},{4,0},{5,0},{6,0} };

RemoteDisplay rem_lcd[] = { {i2c_recover, 0x24},  0x25, 0x26 };
RemoteKeypad rem_keypad[] = { rem_lcd[0].displ(),  rem_lcd[1].displ(), rem_lcd[2].displ() };

LocalKeypad keypad{ KEYINT, KEY_ANALOGUE, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW } };
auto resetPin = Pin_Wag{ RESET_OUT_PIN , LOW};

Error_codes resetI2C_(I2C_Talk & i2c, int addr) { // addr == 0 forces hard reset
	static bool isInReset = false;
	if (isInReset) {
		logger() << "\nTest: Recursive Reset... for 0x" << L_hex << addr;
		return _OK;
	}
	isInReset = true;

	Error_codes hasFailed = _OK;
	logger() << "\n\tResetI2C for 0x" << L_hex << addr << L_endl;
	hardReset_Performed_(i2c, addr);
	isInReset = false;
	return hasFailed;
}

Error_codes hardReset_Performed_(I2C_Talk & i2c, int addr) {
	LocalKeypad::indicatorLED().set();
	resetPin.set();
	delay(128); // interrupts still serviced
	resetPin.clear();
	i2c.begin();
	timeOfReset_mS_ = millis();
	//if (i2c_recover.i2C_is_frozen())  logger() << "\n*** I2C is stuck *** Call arduino reset.\n";
	//else 
	logger() << "\tDone Hard Reset... for 0x" << L_hex << addr << L_endl;
	LocalKeypad::indicatorLED().clear();
	if (digitalRead(20) == LOW) 
		logger() << "\tData stuck after reset" << L_endl;
	//else 
		//logger() << "\tData recovered after reset" << L_endl;
	initialisationRequired_ = true;
	return _OK;
}

uint8_t dsBacklight = 14 * megaFactor;
uint8_t dsContrast = 50 * megaFactor;
uint8_t dsBLoffset = 14;

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(SERIAL_RATE); // required for test-all.
#if defined(__SAM3X8E__)	
	resetRTC(rtc);
#endif	
	logger() << L_allwaysFlush << F(" \n\n********** Logging Begun ***********") << L_endl;
	//logger().readAll();

	pinMode(RESET_LEDP_PIN, OUTPUT);
	digitalWrite(RESET_LEDP_PIN, HIGH);
	timeOfReset_mS_ = millis();

	logger() << "\nsetTimeoutFn";
	//i2c_recover.setTimeoutFn(resetI2C_);
	resetPin.begin();
	i2c.begin();
	i2c.extendTimeouts(5000, 4); // default 3
	i2c.setZeroCross({ ZERO_CROSS_PIN , LOW, INPUT_PULLUP });
	i2c.setZeroCrossDelay(ZERO_CROSS_DELAY);

	keypad.begin();

#if defined(__SAM3X8E__)
	uint8_t status = rtc.status(0x50);
	logger() << "\nEEPROM Status " << status << rtc.getStatusMsg(status);

	status = rtc.status(0x68);
	logger() << "\nClock Status " << status << rtc.getStatusMsg(status) << L_endl;
#endif

	if (analogRead(0) < 10) {// old board; 
		PHOTO_ANALOGUE = 1;
		BRIGHNESS_PWM = 6;
		CONTRAST_PWM = 7;
		mainLCD = new MultiCrystal(26, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28); // old board
	} else {
		mainLCD = new MultiCrystal(46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26); // new board
	}

	// set up the LCD's number of rows and columns:
	mainLCD->begin(20, 4);
	mainLCD->clear();
	mainLCD->print("Start");

	analogWrite(BRIGHNESS_PWM, 255);  // Brightness analogRead values go from 0 to 1023, analogWrite values from 0 to 255
	analogWrite(CONTRAST_PWM, dsContrast);  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255

#if defined(__SAM3X8E__)	
	mainLCD->setCursor(0, 2);
	if (rtc.status(0x68) != _OK) {
		mainLCD->print("RTC:fail");
		yieldFor(500);
	}
	else {
		 logger() << "\nRTC:OK";
		mainLCD->print("RTC:OK");
	}
#endif


	logger() << "\nSetup : fullSpeedTest()\n";
	testMode = speedTestExistsOnly;
	fullSpeedTest();
	testMode = normal;

	for (int i = 0; i < sizeof(i2cAddr); ++i) {
		lastTimeGood[i] = -1;
	}

	delay(200);
	prepareDisplay();
	for (auto & rem : rem_lcd) {
		rem.initialiseDevice();
	}

	logger() << "\n ** End of setup.\n";
}

void loop() {
	// [0] Ic2:0x28 Spd:400000
	// [1] Failed/Temp/Relay: 1
	// [2] One/All Manual Speed Exists
	// [3] NoOfDevices 24/25
	static bool showInstructions = true;
	constexpr auto LOOP_TIME = 750ul; // 10000ul; // 750
	static auto loopEndTime = millis() + LOOP_TIME;
	static uint8_t lastResult;
	static uint8_t lastHours;

	if (showInstructions) {
		mainLCD->setCursor(0, 0);
		mainLCD->print("^v: Speed, <>:Device");
		mainLCD->setCursor(0, 1);
		mainLCD->print("Back: Cycle/One");
		mainLCD->setCursor(0, 2);
		//mainLCD->print("Sel: Full/Exists/Rst");
		mainLCD->print("Sel: ErrMode");
		mainLCD->setCursor(0, 3);
		mainLCD->print("^+v: SpeedTest");
		yieldFor(1000);
		//logger() << "\n***** New loop... *****" << digitalRead(LOCAL_INT_PIN) << L_endl;
	}

	auto nextLocalKey = keypad.getKey();
	//logger() << "loop...showInstructions: " << showInstructions << " Key: " << (int)nextLocalKey << L_endl;
//	signed char nextLocalKey = -1;
	signed char nextRemKey = -1;
	bool displayNeedsRefreshing = false;

	while (nextLocalKey >= 0) { // only do something if key pressed
		displayNeedsRefreshing = true;
		if (showInstructions) {
			showInstructions = false;
			prepareDisplay();
			keypad.clearKeys();
			nextLocalKey = -1;
		}
		logger() << "\n KeyPressed:" << nextLocalKey;

		mainLCD->setCursor(0, 0);
		mainLCD->print("                    ");
		switch (nextLocalKey) {
		case 0:
			fullSpeedTest();
			break;
		case 1: // Up - Set Frequency
			loopTimeIndex = (loopTimeIndex == 0) ? 0 : loopTimeIndex - 1;
			logger() << "Up : " << loopTimeIndex << L_endl;
			break;
		case 2: // Left - Set address
			i2cAddrIndex = (i2cAddrIndex == 0) ? sizeof(i2cAddr) - 1 : i2cAddrIndex - 1;
			break;
		case 3: // Right - Set address
			i2cAddrIndex = (i2cAddrIndex == sizeof(i2cAddr) - 1) ? 0 : i2cAddrIndex + 1;
			break;
		case 4: // Down - Set Frequency
			loopTimeIndex = (loopTimeIndex == sizeof(loopTime) / sizeof(loopTime[0]) - 1) ? loopTimeIndex : loopTimeIndex + 1;
			break;
		case 5: // Back
			cycleDevices = !cycleDevices;
			mainLCD->setCursor(0, 2);
			if (cycleDevices) mainLCD->print("All"); else mainLCD->print("One");
			break;
		case 6: // Select
			++testMode;
			if (testMode >= NO_OF_MODES) testMode = 0;
			switch (testMode) {
			case normal:  logger() << "\n*** normal ***"; break;
			case resetBeforeTest:   logger() << "\n*** resetBeforeTest *** "; break;
			case speedTestExistsOnly:   logger() << "\n*** speedTestExistsOnly *** "; break;
			}
			yieldFor(500);
			logger() << "\n*** User Requested Hard Reset ***";
			hardReset_Performed_(i2c, 0);
			break;
		default:
			;
		}
	
		mainLCD->setCursor(4, 2);
		if (loopTimeIndex != 0) {
			auto & device = getDevice(i2cAddr[i2cAddrIndex]);
			device.set_runSpeed(100000000 / loopTime[loopTimeIndex]);
			logger() << "Max I2C Speed: " << i2c.max_i2cFreq();
			//i2c.setI2CFrequency(100000000 / loopTime[loopTimeIndex]);
			logger() << "\n Loop set speed to " << i2c.getI2CFrequency() << L_endl;
			mainLCD->print("Man freq  ");
		}
		else {
			mainLCD->print("Auto freq ");
			auto & device = getDevice(i2cAddr[i2cAddrIndex]);
			i2c.setI2CFrequency(device.runSpeed());
			logger() << "\n Loop set speed: Optimal for 0x" << L_hex << i2cAddr[i2cAddrIndex] << " is " << L_dec << device.runSpeed() << L_endl;
		}
		printTestMode();
		nextLocalKey = keypad.getKey();
	}
	uint8_t lastKey = 0;
	int repeatKeyCount = 0;
	//while (true) {
		for (int displayID = 0; displayID < 3 ; ++displayID) {
			//int displayID = 2;
			nextRemKey = rem_keypad[displayID].getKey();
			if (nextRemKey >= 0) {
				if (nextRemKey == lastKey) ++repeatKeyCount; 
				else {
					repeatKeyCount = 0;
					lastKey = nextRemKey;
				}
				logger() << "Display[]" << displayID << " key: " << nextRemKey << " Repeat: " << repeatKeyCount << L_endl;
				switch (nextRemKey) {
				case 1: // Up
					rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Up"); break;
				case 2: // Left
					rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Left"); break;
				case 3: // Right
					rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Right"); break;
				case 4: // Down
					rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Down"); break;
				}
				rem_lcd[displayID].displ().print(repeatKeyCount);
			} 
		}
		//delay(500);
	//}

	if (!showInstructions) {
		//logger() << "NoShowInstr" << L_endl;
		if (millis() > loopEndTime) {
			//logger() << "loopendtime" << L_endl;
			loopEndTime = millis() + LOOP_TIME;
			displayNeedsRefreshing = true;
			if (cycleDevices) { // get next device
				//readRemoteKeyboard();
				//logger() << "cycleDevices" << L_endl;

				++i2cAddrIndex;
				//logger() << "\nMoved to next device pos " << i2cAddrIndex << " Sizeof: " << sizeof(i2cAddr) << L_endl;
				if (i2cAddrIndex >= sizeof(i2cAddr)) {
					i2cAddrIndex = 0;
					//logger() << "Next Cycle" << L_endl;
					++tryCount;
					auto status = rPort.nextSequence();
				}
				//logger() << "\nMoved to next device pos" << i2cAddrIndex << L_endl;
				//logger() << "\nMoved to next device 0x" << L_hex << i2cAddr[i2cAddrIndex]  << L_endl;
			}
		}

		if (displayNeedsRefreshing) {
			//logger() << "displayNeedsRefreshing" << L_endl;
			mainLCD->setCursor(0, 0);
			mainLCD->print("I2C:0x");
			mainLCD->print(i2cAddr[i2cAddrIndex], HEX);
			mainLCD->print(" Spd:");
			printSpeed();
			mainLCD->setCursor(0, 1);
			mainLCD->print("       ");
		}

		logger() << "tryDevice index:" << i2cAddrIndex << L_endl;
		uint8_t result = tryDeviceAt(i2cAddrIndex);
		//uint8_t result = 1;

		if (result != lastResult) {
			displayNeedsRefreshing = true;
		}

		lastResult = result;

		mainLCD->setCursor(0, 1);
		if (result == 0) {

			mainLCD->print("Failed");
			if (lastTimeGood[i2cAddrIndex] == 0) {
				lastTimeGood[i2cAddrIndex] = millis();
			}
			logger() << "\n Failed for 0x" << L_hex << i2cAddr[i2cAddrIndex];
			logger() << L_endl << L_endl;
			//loopEndTime = millis() + LOOP_TIME;
		}
		else {
			if (lastTimeGood[i2cAddrIndex] != 0) {
				logger() << "\n Recovered for 0x" << L_hex << i2cAddr[i2cAddrIndex] << " after mS: " << L_dec << millis() - lastTimeGood[i2cAddrIndex];
				lastTimeGood[i2cAddrIndex] = 0;
				// logger() << "\n Success :", min((successCount[i2cAddrIndex] * 100L) / tryCount, 100), "%");
				logger() << L_endl;
			}
			++successCount[i2cAddrIndex];
			if (displayNeedsRefreshing) {
				mainLCD->print((int)result, DEC);
				logger() << "*** 0x" << L_hex << i2cAddr[i2cAddrIndex] << " at " << L_dec << getDevice(i2cAddr[i2cAddrIndex]).runSpeed() << " Result: " << result << "***\n" << L_endl;
			}
		}

		if (displayNeedsRefreshing) {
			int noOfGood = 0;
			for (int i = 0; i < sizeof(i2cAddr); ++i) {
				if (lastTimeGood[i] == 0) ++noOfGood;
			}
			mainLCD->setCursor(0, 3);

			bool sd_ok = logger().open();

			bool rtc_ok = clock_().ok();

			if (!sd_ok)       mainLCD->print("SD Card Failed.     ");
			else if (!rtc_ok) mainLCD->print("RTC Failed.         ");
			else		      mainLCD->print("No of devices:      ");
			mainLCD->setCursor(15, 3);
			mainLCD->print(noOfGood);
			mainLCD->print("/");
			mainLCD->print(sizeof(i2cAddr), DEC);
		}

		yieldFor(200);

		if (clock_().hrs() != lastHours) {
			lastHours = clock_().hrs();
			logger() << "\n Still looping...";
		}

	}
} 

void printTestMode() {
	mainLCD->setCursor(14, 2);
	switch (testMode) {
	case normal: mainLCD->print("Norm  "); break;
	case resetBeforeTest:  mainLCD->print("Reboot"); break;
	case speedTestExistsOnly:  mainLCD->print("Exists"); break;
	}
}

void printSpeed() {
	mainLCD->setCursor(13, 0);
	mainLCD->print("       ");
	mainLCD->setCursor(13, 0);
	mainLCD->print(getDevice(i2cAddr[i2cAddrIndex]).runSpeed(), DEC);
}

void prepareDisplay() {
	mainLCD->clear();
	mainLCD->setCursor(0, 0);
	mainLCD->print("I2C:0x"); mainLCD->print(i2cAddr[i2cAddrIndex], HEX);
	mainLCD->print(" Spd:"); mainLCD->print(i2c.getI2CFrequency(), DEC);
	mainLCD->setCursor(0, 2);
	mainLCD->print("All Auto freq");
	printTestMode();
	mainLCD->setCursor(0, 3);
	mainLCD->print("No of devices:      ");
	mainLCD->setCursor(15, 3);
	mainLCD->print("0/");
	mainLCD->print(sizeof(i2cAddr), DEC);
}

int deviceIndex(int addr) {
	for (int i = 0; i < sizeof(i2cAddr) / sizeof(i2cAddr[0]); ++i) {
		if (i2cAddr[i] == addr) return i;
	}
	logger() << "Error in deviceIndex for 0x" << L_hex << addr << L_endl;
	return 0;
}

void fullSpeedTest() {
	keypad.clearKeys();
	mainLCD->clear();
	logger() << "\n Speedtest I2C.\n";
	mainLCD->setCursor(0, 2);
	mainLCD->print("Speed: 0x");
	bool firstFound = false;
	unsigned long showTill = millis() + 500L;
	unsigned long startTestTime = millis();
	i2c_test.prepareTestAll();
	for (int i = 0; i < sizeof(i2cAddr); ++i) {
		uint8_t addr = i2cAddr[i];
		auto & dev = getDevice(addr);
		logger() << L_endl << "device 0x" << L_hex << dev.getAddress() << L_endl;
		i2c_recover.registerDevice(dev);
		i2c_test.show_fastest(dev);
		if (yieldFor(showTill - millis())) break;
		mainLCD->setCursor(9, 2);
		mainLCD->print(addr, HEX);
		mainLCD->print(" :       ");
		mainLCD->setCursor(13, 2);
		if (i2c_test.error() == 0) {
			mainLCD->print(dev.runSpeed());
			logger() << "\t Speed device 0x" << L_hex << dev.getAddress() << L_dec << F(" : ") << dev.runSpeed() << L_endl;
			++(i2c_test.totalDevicesFound);
		}
		else {
			mainLCD->print("Failed");
			logger() << "\tFailed Speed-test\n";
			lastTimeGood[i] = millis();
		}
		showTill = millis() + 500L;
	}
	i2c.setTimeouts(5000, I2C_Talk::WORKING_STOP_TIMEOUT);
	logger() << "\tTest took mS: " << millis() - startTestTime;
	mainLCD->setCursor(0, 3);
	mainLCD->print("Total:");
	mainLCD->print(i2c_test.totalDevicesFound);
	mainLCD->setCursor(9, 3);
	mainLCD->print("Spd:");
	mainLCD->print(i2c_test.maxSafeSpeed());
	logger() << "\nNo of Devices: " << i2c_test.totalDevicesFound << L_endl;

	yieldFor(2000);
	mainLCD->clear();
}

uint8_t initialiseRemoteDisplaysFailed_() {
	uint8_t rem_error;
	logger() << "\nInitialiseRemoteDisplays\n";
	rem_error = 0;
	for (auto & rmPtr : rem_lcd) {
		rem_error |= rmPtr.initialiseDevice();
	}
	return rem_error;
}

I_I2Cdevice_Recovery & getDevice(int addr) {
	if (addr == 0x10) {
		return mixValve;
	}
	else 
		if (addr == 0x20) {
		return rPort;
	}
	else if (addr >= 0x24 && addr <= 0x26) {
		return rem_lcd[addr - 0x24];
	}
	else {
		return tempSens[deviceIndex(addr) - deviceIndex(FIRST_TEMP_SENS_ADDR)];
	}
}

uint8_t tryDeviceAt(int addrIndex) {
	constexpr uint8_t GPPU = 0x0C;    // GPio Pull Up
	constexpr uint8_t GPIOA = 0x12;
	uint8_t dataBuffa;
	uint8_t hasFailed = 0;
	uint8_t addr = i2cAddr[addrIndex];
	//logger() << "TryDevice 0x" << L_hex << addr << L_endl;
	auto & device = getDevice(addr);
	if (!device.isEnabled()) device.reset();
	mainLCD->setCursor(10, 1);
	printSpeed();
	dataBuffa = 0; // 0 == failed
	//logger() << "0x" << L_hex << addr << " Exists? " << L_endl;
	device.I_I2Cdevice::getStatus();
	if (addr == 0) {
	}
	else if (addr == 0x10) { // mix arduino
		hasFailed = mixValve.getPos(dataBuffa);
	}
	else if (addr == 0x20) { // relay box
		hasFailed = rPort.testRelays();
	}
	else if (addr <= 0x26) {
		hasFailed = i2c.read(addr, GPIOA, 1, &dataBuffa);
	}
	else {
		auto & ts = static_cast<TempSensor &>(device);
		hasFailed = ts.readTemperature();
		if (!hasFailed) dataBuffa = ts.get_temp();
		else dataBuffa = 0;
	}

	if (hasFailed == _OK && dataBuffa == 0) dataBuffa = 1;
	return dataBuffa;
}

bool yieldFor(int32_t delayMillis) {
	auto timeout = Timer_mS{ delayMillis };
	while (!timeout && !keypad.keyIsWaiting());
	return keypad.keyIsWaiting();
//	return false;
}

byte fromBCD(uint8_t value) {
	uint8_t decoded = value & 127;
	decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
	return decoded;
}

// ************** Relay Port *****************

Error_codes RelaysPortSequence::testDevice() { // non-recovery test
	//logger() << "RelaysPortSequence::testDevice..." << L_endl;
	Error_codes status = I_I2Cdevice::testDevice();
	mainLCD->setCursor(7, 1);
	mainLCD->print("Relay OK     ");
	if (status != _OK) {
		mainLCD->setCursor(13, 1);
		mainLCD->print  ("Ini Bad");
	}
	return status;
}

Error_codes RelaysPortSequence::testRelays() {
	Error_codes status = initialiseDevice();
	mainLCD->setCursor(7, 1);
	mainLCD->print("Relay        ");
	mainLCD->setCursor(13, 1);
	if (status != _OK) mainLCD->print("Ini Bad");
	else /*if (testMode != speedTestExistsOnly)*/ {
		const int noOfRelays = sizeof(relays) / sizeof(relays[0]);
		uint8_t numberFailed = 0;
		//uint8_t relayNo = 2;
		for (uint8_t relayNo = 0; relayNo < noOfRelays; ++relayNo) {
			auto onStatus = _OK;
			auto offStatus = _OK;
			mainLCD->setCursor(13, 1);
			mainLCD->print(relayNo, DEC);
			logger() << "* Relay *" << relayNo << L_endl;
			relays[relayNo].set(1);
			onStatus = updateRelays();
			if (onStatus == _OK) logger() << " ON OK\n"; else logger() << " ON Failed\n";
			if (yieldFor(300)) return status;
			relays[relayNo].set(0);
			offStatus = updateRelays();
			if (offStatus == _OK) logger() << " OFF OK\n"; else logger() << " OFF Failed\n";
			offStatus |= onStatus;
			numberFailed = numberFailed + (offStatus != _OK);
			if (offStatus != _OK) {
				mainLCD->print(" Bad  ");
				logger() << "Test Failed: " << relayNo << L_endl;
			}
			else {
				mainLCD->print(" OK   ");
				logger() << "Test OK\n";
			}
			if (yieldFor(300)) return status;
			status |= offStatus;
		}
	}
	return status;
}

Error_codes RelaysPortSequence::nextSequence() {
	static int sequenceIndex = 0;
	logger() << "nextSequence: " << sequenceIndex << L_endl;
	_relayRegister = relaySequence[sequenceIndex];
	logger() << "nextReg: 0x" << L_hex << ~_relayRegister << L_endl;
	++sequenceIndex;
	if (sequenceIndex >= sizeof(relaySequence)) sequenceIndex = 0;
	auto status = updateRelays();
	mainLCD->setCursor(7, 1);
	mainLCD->print("Relay ");
	mainLCD->setCursor(13, 1);
	//mainLCD->print(~_relayRegister, BIN);
	if (status != _OK) {
		logger() << "RelaysPort::nextSequence failed first write_verify" << L_endl;
		initialiseDevice();
		status = updateRelays();
		if (status != _OK) {
			logger() << "RelaysPort::nextSequence failed second write_verify" << L_endl;
			mainLCD->setCursor(13, 1);
			mainLCD->print("Bad   ");
		}
	}
	return status;
}

Error_codes MixValveController::testDevice() { // non-recovery test
	//logger() << "\n MixValveController::testDevice start" << L_endl;
	auto timeOut = Timer_mS(timeOfReset_mS_ + 3000UL - millis());
	Error_codes status;
	uint8_t dataBuffa[1] = { 1 };
	while (!timeOut && !keypad.keyIsWaiting()) {
		status = I_I2Cdevice::write_verify(7, 1, dataBuffa); // non-recovery write request temp
		if (status == _OK) { timeOfReset_mS_ = millis() - 3000; break; }
		else if (status >= _BusReleaseTimeout) break;
	}
	dataBuffa[0] = i2c.getI2CFrequency()/2000;
	status = I_I2Cdevice::write_verify(7, 1, dataBuffa); // non-recovery write request temp
	
	//if (status)  logger() << "\n MixValveController::testDevice failed at " << i2c.getI2CFrequency() << I2C_Talk::getStatusMsg(status) << L_endl;
	//else  logger() << "\n MixValveController::testDevice at " << i2c.getI2CFrequency() << " OK after mS " <<  tryAgainTime << L_endl;
	return status;
}

uint8_t MixValveController::getPos(uint8_t & pos) {
	auto timeOut = Timer_mS(timeOfReset_mS_ + 3000UL - millis());
	do {
	} while (!timeOut && !keypad.keyIsWaiting());

	uint8_t status = read(valve_pos, 1, &pos);
	uint8_t dataBuffa = 1;
	if (status == _OK) status = write(7, 1, &dataBuffa); // debug try writing as well.
	if (status)  logger() << "\n MixValveController::getPos failed. device:" << getAddress() << I2C_Talk::getStatusMsg(status) << L_endl;
	//else  logger() << "\n MixValveController::getPos OK after mS", tryAgainTime);
	return status;
}