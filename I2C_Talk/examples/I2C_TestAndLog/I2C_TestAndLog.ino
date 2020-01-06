#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_Scan.h>
#include <I2C_RecoverRetest.h>

#include <EEPROM.h>
#include <Clock.h>
#include <Logging.h>
#include <Wire.h>

#include <SPI.h>
#include <SD.h>

#include <MultiCrystal.h>
#include <Date_Time.h>
#include <Conversions.h>

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;

#define LOCAL_INT_PIN 18
#define SERIAL_RATE 115200
#define DISPLAY_WAKE_TIME 15
#define I2C_RESET 14
#define RTC_RESET 4
#define I2C_DATA 20

const char * LOG_FILE = "zp_log.txt";
const signed char ZERO_CROSS_PIN = -15; // -ve = active falling edge.
const signed char RESET_OUT_PIN = -14;  // -ve = active low.
const signed char RESET_LEDP_PIN = 16;  // high supply
const signed char RESET_LEDN_PIN = 19;  // low for on.
const signed char RTC_ADDRESS = 0x68;
const signed char RelayPort_ADDRESS = 0x20;
const signed char MixValve_ADDRESS = 0x10;
// registers
const unsigned char DS75LX_HYST_REG = 2;
const unsigned char REG_8PORT_IODIR = 0x00;
const unsigned char REG_8PORT_PullUp = 0x06;
const unsigned char REG_8PORT_OPORT = 0x09;
const unsigned char REG_8PORT_OLAT = 0x0A;
const unsigned char DS75LX_Temp = 0x00; // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
const unsigned char DS75LX_Config = 0x01;
int st_index = -1;
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
//Code in here will only be compiled if an Arduino Mega is used.
#define KEYINT 5
#define DIM_LCD 135

#elif defined(__SAM3X8E__)
#define KEYINT LOCAL_INT_PIN
#define DIM_LCD 200
#endif

#define REMKEYINT 19

I2C_Talk rtc{ Wire1, 100000 };
I_I2C_Scan rtc_scan{ rtc };

EEPROMClass_T<rtc> _eeprom_obj{ 0x50 };

EEPROMClass & EEPROM = _eeprom_obj;

Clock & clock_() {
	static Clock_I2C<rtc> _clock(RTC_ADDRESS);
	return _clock;
}

Logger & logger() {
	//static Serial_Logger _log(SERIAL_RATE, clock_());
	static SD_Logger _log(LOG_FILE, SERIAL_RATE, clock_());
	return _log;
}

I2C_Talk i2C;
auto i2c_recover = I2C_Recover_Retest{i2C, 0};
I_I2C_SpeedTestAll i2c_test{ i2C, i2c_recover };

class RelaysPort : public I_I2Cdevice_Recovery {
public:
	RelaysPort(I2C_Recover & recovery, int addr, int8_t zeroCrossPin, int8_t resetPin);
	//uint8_t setAndTestRegister();
	// Virtual Functions
	uint8_t initialiseDevice() override;
	uint8_t testDevice() override;
	static uint8_t relayRegister;
private:
	int8_t _zeroCrossPin = 0;
	int8_t _resetPin = 0;
};

uint8_t RelaysPort::relayRegister = 0xFF;

class I2C_Temp_Sensor : public I_I2Cdevice_Recovery {
public:
	using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
	// Queries
	int8_t get_temp() const;
	int16_t get_fractional_temp() const;
	static bool hasError() { return _error != _OK; }

	// Modifiers
	int8_t readTemperature();
	uint8_t setHighRes();
	// Virtual Functions
	uint8_t testDevice() override;

protected:
	int16_t _lastGood = 20 * 256;
	static int _error;
private:
};

class RemoteDisplay : public I_I2Cdevice_Recovery {
public:
	//using I2Cdevice::I2Cdevice;
	RemoteDisplay(I2C_Recover & recovery, int addr);
	RemoteDisplay(int addr);
	// Virtual Functions
	uint8_t testDevice() override;
	uint8_t sendMessage(const char * msg);
	uint8_t initialiseDevice() override;
	MultiCrystal & displ() { return _lcd; }

private:
	MultiCrystal _lcd;
};

class MixValveController : public I_I2Cdevice_Recovery {
public:
	enum Registers {
		/*Read Only Data*/		status, //Has new data, Error code
		/*Read Only Data*/		mode, count, valve_pos, state = valve_pos + 2,
		/*Read/Write Data*/		flow_temp, request_temp, ratio, control,
		/*Read/Write Config*/	temp_i2c_addr, max_ontime, wait_time, max_flow_temp, eeprom_OK1, eeprom_OK2,
		/*End-Stop*/			reg_size
	};
	using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
	// Virtual Functions
	uint8_t getPos(uint8_t & pos);
	uint8_t testDevice() override;
private:
	unsigned long * _timeOfReset_mS = 0;
	int _error = 0;
};

enum { e_secs, e_mins, e_hrs, e_spare, e_day, e_mnth, e_yr, e_size };
byte datetime[e_size];
const byte TEMP_REG = 0;
const byte TEMP_HYST_REG = 2;
const byte TEMP_LIMIT_REG = 3;

byte i2cAddr[] = { 0x10,0x20,0x24,0x25,0x26,0x28,/*0x29,*/0x2B,0x2C,0x2D,0x2E,0x2F,0x35,0x36,0x37,0x48,0x49,0x4B,0x4F,0x70,0x71,0x72,0x74,0x75,0x76,0x77 };
int successCount[sizeof(i2cAddr)] = { 0 };
unsigned long lastTimeGood[sizeof(i2cAddr)] = { 0 };

MixValveController mixValve{ i2c_recover, 0x10 };
RelaysPort rPort(i2c_recover, 0x20, ZERO_CROSS_PIN, RESET_OUT_PIN);
RemoteDisplay rem_lcd[] = { {i2c_recover, 0x24},  0x25, 0x26 };
I2C_Temp_Sensor tempSens[] = { {i2c_recover, 0x28}, /*0x29,*/0x2B,0x2C,0x2D,0x2E,0x2F,0x35,0x36,0x37,0x48,0x49,0x4B,0x4F,0x70,0x71,0x72,0x74,0x75,0x76,0x77 };

int deviceIndex(int addr) {
	for (int i = 0; i < sizeof(i2cAddr) / sizeof(i2cAddr[0]); ++i) {
		if (i2cAddr[i] == addr) return i;
	}
	Serial.print("Error in deviceIndex for 0x"); Serial.println(addr); Serial.flush();
	return 0;
}

int tryCount = 0;
byte i2cAddrIndex = 0;
unsigned int loopTime[] = { 0, 100,200,500,1000,2000,5000,10000,20000,40000 };
byte loopTimeIndex = 0;
enum { normal, resetBeforeTest, speedTestExistsOnly, NO_OF_MODES };
byte testMode = resetBeforeTest;

MultiCrystal * mainLCD(0);

I_I2Cdevice_Recovery & getDevice(int addr);

bool cycleDevices = true;
byte firstAddress;
unsigned long timeOfReset_mS_;
bool initialisationRequired_ = false;
uint8_t hardReset_Performed_(I2C_Talk & i2c, int addr);
uint8_t initialiseRemoteDisplaysFailed_();
uint8_t tryDeviceAt(int addrIndex);
int8_t getKeyCode(uint16_t gpio);

uint8_t resetI2C_(I2C_Talk & i2c, int addr) { // addr == 0 forces hard reset
	static bool isInReset = false;
	if (isInReset) {
		logger() << "\nTest: Recursive Reset... for " << addr;
		return 0;
	}
	isInReset = true;

	uint8_t hasFailed = 0;
	auto origFn = i2c_recover.getTimeoutFn();
	i2c_recover.setTimeoutFn(hardReset_Performed_);
	logger() << "\n\tResetI2C for " << addr;
	hardReset_Performed_(i2c, addr);
	if (!i2c_recover.isRecovering()) {
		hasFailed = getDevice(addr).testDevice();

		//if (hasFailed) {
			// logger() << "\n Re-test Speed for", addr, " Started at: ", i2c.getI2CFrequency());
			//hasFailed = speedTestDevice_(addr);
			// logger() << "\n Re-test Speed Done at:", i2c.getThisI2CFrequency(addr));
		//}

		if (!hasFailed && initialisationRequired_) {
			auto iniFailed = initialiseRemoteDisplaysFailed_();
			iniFailed = iniFailed | rPort.initialiseDevice();
			initialisationRequired_ = (iniFailed == 0);
		}
	}
	i2c_recover.setTimeoutFn(origFn);
	isInReset = false;
	return hasFailed;
}

uint8_t hardReset_Performed_(I2C_Talk & i2c, int addr) {
	digitalWrite(RESET_LEDN_PIN, LOW);
	digitalWrite(I2C_RESET, LOW);
	delayMicroseconds(128000);
	digitalWrite(I2C_RESET, HIGH);
	i2c.restart();
	timeOfReset_mS_ = millis();
	//if (i2c_recover.i2C_is_frozen())  logger() << "\n*** Reset I2C is stuck at I2c freq:", i2c.getI2CFrequency());
	//else 
	logger() << "\n\t\tDone Hard Reset... for " << addr;
	digitalWrite(RESET_LEDN_PIN, HIGH);
	initialisationRequired_ = true;
	return true;
}

uint8_t resetRTC(I2C_Talk & i2c, int addr) {
	logger() << "\nPower-Down RTC...";
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(200000);
	digitalWrite(RTC_RESET, LOW);
	i2c.restart();
	delayMicroseconds(200000);
	return true;
}

bool checkEEPROM(const char * msg) {
	bool OK = true;
	if (EEPROM.getStatus() != _OK) {
		resetRTC(rtc, 0x50);
		if (EEPROM.getStatus() != _OK) {
			logger() << "\nEEPROM error after reset " << msg;
		} else logger() << "\nEEPROM recovered after reset: " << msg;
		OK = false;
	}
	return OK;
}

uint8_t testEEPROM() {
	const char test[] = { "Test string for EEPROM OK" };
	auto status = EEPROM.writeEP(1000, sizeof(test), test);
	char verify[sizeof(test)] = { 0 };
	if (status == _OK) {
		status = EEPROM.readEP(1000, sizeof(verify), verify);
		if (status == _OK) {
			logger().set(L_endl) << verify;
			return status;
		}
	}
	logger() << "\ntestEEPROM failed with " << status << EEPROM.getStatusMsg(status);
	return status;
}

uint8_t initialiseRemoteDisplaysFailed_() {
	uint8_t rem_error;
	logger() << "\nInitialiseRemoteDisplays";
	rem_error = 0;
	for (auto & rmPtr : rem_lcd) {
		rem_error |= rmPtr.initialiseDevice();
	}
	return rem_error;
}

I_I2Cdevice_Recovery & getDevice(int addr) {
	//if (testMode == speedTestExistsOnly) return 0;
	if (addr == 0x10) {
		return mixValve;
	}
	else if (addr == 0x20) {
		return rPort;
	}
	else if (addr >= 0x24 && addr <= 0x26) {
		return rem_lcd[addr - 0x24];
	}
	else {
		return tempSens[deviceIndex(addr) - deviceIndex(0x28)];
	}
}

void fullSpeedTest() {
	mainLCD->clear();
	logger() << "\n Speedtest I2C.";
	mainLCD->setCursor(0, 2);
	mainLCD->print("Speed: 0x");
	bool firstFound = false;
	//hardReset_Performed_(i2C, 0);
	unsigned long showTill = millis();
	unsigned long startTestTime = millis();
	i2c_test.prepareTestAll();
	for (int i = 0; i < sizeof(i2cAddr); ++i) {
		uint8_t addr = i2cAddr[i];
		auto & dev = getDevice(addr);
		logger() << "\nDevice addr: 0x" << L_hex << dev.getAddress() << L_endl;
		i2c_recover.basicTestsBeforeScan();
		//if (testMode == resetBeforeTest) hardReset_Performed_(i2C, addr);
		//i2c_recover.foundDeviceAddr = addr;
		i2c_test.show_fastest(dev);
		while (millis() < showTill);
		mainLCD->setCursor(9, 2);
		mainLCD->print(addr, HEX);
		mainLCD->print(" :       ");
		mainLCD->setCursor(13, 2);
		if (i2c_test.error() == 0) {
			mainLCD->print(dev.runSpeed());
			logger() << "\t Speed: " << dev.runSpeed() << L_endl;
			++(i2c_test.totalDevicesFound);
		}
		else {
			mainLCD->print("Failed");
			logger() << "\tFailed Speed-test\n";
			lastTimeGood[i] = millis();
		}
		showTill = millis() + 500L;
	}
	logger() << "\tTest took mS: " << millis() - startTestTime;
	mainLCD->setCursor(0, 3);
	mainLCD->print("Total:");
	mainLCD->print(i2c_test.totalDevicesFound);
	mainLCD->setCursor(9, 3);
	mainLCD->print("Spd:");
	mainLCD->print(i2c_test.maxSafeSpeed());
	logger() << "\nFinal Max speed: " << i2c_test.maxSafeSpeed();
	logger() << "\nNo of Devices: " << i2c_test.totalDevicesFound << L_endl;

	delay(2000);
	mainLCD->clear();
}

void readLocalKeyboard();
void shiftKeyQue();

byte PHOTO_ANALOGUE = A0;
byte KEY_ANALOGUE = A1;
byte SIGNAL = A5;
byte BRIGHNESS_PWM = 5; // pins 5 & 6 are not best for PWM control.
byte CONTRAST_PWM = 6;

volatile signed char keyQuePos = -1; // points to last entry in KeyQue
unsigned int adc_LKey_val[] = { //Setup,Up,Lft,Rght,Dwn,Bak,Sel
  874, 798, 687, 612, 551, 501, 440
}; // for analogue keypad

byte dsBacklight = 14;
byte dsContrast = 50;
byte dsBLoffset = 14;

byte NUM_LKEYS = 7;
// initialize key queue. First column is display no. Second is key no.
volatile signed char keyQue[10][2] = {
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1,
  -1, -1
};

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	logger() << " Serial Begun";
	pinMode(I2C_RESET, OUTPUT);
	pinMode(PHOTO_ANALOGUE, INPUT);
	pinMode(REMKEYINT, INPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	digitalWrite(PHOTO_ANALOGUE, LOW); // turn off pull-up
	digitalWrite(LOCAL_INT_PIN, HIGH); // turn ON pull-up
	// set I2C pins low to allow reset
	pinMode(I2C_DATA, INPUT);
	pinMode(RESET_LEDP_PIN, OUTPUT);
	pinMode(RESET_LEDN_PIN, OUTPUT);
	digitalWrite(RESET_LEDP_PIN, HIGH);
	digitalWrite(RESET_LEDN_PIN, HIGH);

	timeOfReset_mS_ = millis();
	resetRTC(rtc, 0x50);
	while (testEEPROM()) {
		resetRTC(rtc, 0x50);
		clock_().loadTime();
	};

	//rtc_scan.show_all();
	logger() << "\n***** Start setup *****";
	 logger() << "\nRTC at: " << (long)&rtc;
	 logger() << "\nI2C_Talk at: " << (long)&i2C;
	 logger() << "\nRecoveryObj at: " << (long)&i2c_recover;
	 logger() << "\nMixV I2C_Talk at: " << (long)&mixValve.i2C();
	 logger() << "\nMixV RecoveryObj at: " << (long)&mixValve.recovery();
	 logger() << "\nRem Disp[0] at: " << (long)&rem_lcd[0];
	 logger() << "\nRem Disp[1] at: " << (long)&rem_lcd[1];
	 logger() << "\nRem Disp[2] at: " << (long)&rem_lcd[2];
	 logger() << "\nsetTimeoutFn";
	i2c_recover.setTimeoutFn(resetI2C_);

	char stackTrace[32];
	EEPROM.readEP(0, 10, stackTrace);
	logger() << "\nScore[0] " << int(stackTrace[0]);

	i2C.restart();

	uint8_t status = rtc.status(0x50);
	logger() << "\nEEPROM Status " << status << rtc.getStatusMsg(status);

	status = rtc.status(0x68);
	logger() << "\nClock Status " << status << rtc.getStatusMsg(status);

	mainLCD = new MultiCrystal(46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26); // new board

	// set up the LCD's number of rows and columns:
	mainLCD->begin(20, 4);
	mainLCD->clear();
	mainLCD->print("Start");

	analogWrite(BRIGHNESS_PWM, 255);  // Brightness analogRead values go from 0 to 1023, analogWrite values from 0 to 255
	analogWrite(CONTRAST_PWM, dsContrast);  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255

	mainLCD->setCursor(0, 2);
	if (rtc.status(0x68) != _OK) {
		mainLCD->print("RTC:fail");
		delay(500);
	}
	else {
		 logger() << "\nRTC:OK";
		mainLCD->print("RTC:OK");
	}

	//for (auto & ltg : lastTimeGood) ltg = 0;
	i2c_recover.strategy().log_stackTrace();
	 logger() << "\n0x28 readTemperature()";
	tempSens[0].readTemperature();

	fullSpeedTest();
	testMode = normal;

	if (!initialiseRemoteDisplaysFailed_())  logger() << "\n Remote Display begin() all OK";
	else  logger() << "\n Remote Displ begin() failed";

	mainLCD->setCursor(0, 0);
	mainLCD->print("                    ");
	mainLCD->setCursor(0, 1);
	mainLCD->print("                    ");
	mainLCD->setCursor(0, 0);
	mainLCD->print("I2C:0x"); mainLCD->print(i2cAddr[i2cAddrIndex], HEX);

	mainLCD->print(" Spd:"); mainLCD->print(i2C.getI2CFrequency(), DEC);
	mainLCD->setCursor(0, 2);
	mainLCD->print("Optimal Speed ");
	 logger() << "\nattachInterrupt";
	attachInterrupt(KEYINT, readLocalKeyboard, FALLING);
	pinMode(SIGNAL, OUTPUT);

	mainLCD->setCursor(14, 2);
	switch (testMode) {
	case normal: mainLCD->print("Norm  "); break;
	case resetBeforeTest:  mainLCD->print("Reboot"); break;
	case speedTestExistsOnly:  mainLCD->print("Exists"); break;
	}
	//i2c_test.show_fastest(0x28);
	 logger() << "\n ** End of setup.\n";
}

bool keyIsWaiting() {
	return keyQue[0][1] != -1;
}

void printSpeed() {
	//checkEEPROM(" EP Failed before printSpeed() ");
	mainLCD->setCursor(13, 0);
	mainLCD->print("       ");
	mainLCD->setCursor(13, 0);
	mainLCD->print(getDevice(i2cAddr[i2cAddrIndex]).runSpeed(), DEC);
	//checkEEPROM(" EP Failed after printSpeed() ");
}

void loop() {
	//logger() << "\n***** New loop... *****";
	//i2c_recover.show_fastest(0x28);
	//Serial.println("loop...");
	checkEEPROM(" EP Failed new Loop");
	//return;
	constexpr auto LOOP_TIME = 750ul; // 10000ul; // 750
	static auto loopEndTime = millis() + LOOP_TIME;
	static uint8_t lastResult;
	static uint8_t lastHours;
	//checkEEPROM(" EP Failed after statics");
	signed char nextKey = keyQue[0][1];
	uint8_t displayID = keyQue[0][0];
	bool displayNeedsRefreshing = false;
	st_index = -1;
	//checkEEPROM(" EP Failed before check key");
	i2c_recover.strategy().stackTrace(++st_index, "Loop-Start");
	if (nextKey >= 0) { // only do something if key pressed
		displayNeedsRefreshing = true;
		 logger() << "\n KeyPressed:", nextKey);

		mainLCD->setCursor(0, 0);
		mainLCD->print("                    ");
		if (displayID == 0) {
			switch (nextKey) {
			case 0:
				fullSpeedTest();
				break;
			case 1: // Up - Set Frequency
				loopTimeIndex = (loopTimeIndex == 0) ? 0 : loopTimeIndex - 1;
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
				break;
			case 6: // Select
				++testMode;
				if (testMode >= NO_OF_MODES) testMode = 0;
				mainLCD->setCursor(13, 2);
				switch (testMode) {
				case normal:  logger() << "\n*** normal ***"; break;
				case resetBeforeTest:   logger() << "\n*** resetBeforeTest *** "; break;
				case speedTestExistsOnly:   logger() << "\n*** speedTestExistsOnly *** "; break;
				}
				delay(500);
				logger() << "\n*** User Requested Hard Reset ***";
				hardReset_Performed_(i2C, 0);
				break;
			default:
				;
			}
		}
		else {
			switch (nextKey) {
			case 1: // Up
				rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Up"); break;
			case 2: // Left
				rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Left"); break;
			case 3: // Right
				rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Right"); break;
			case 4: // Down
				rem_lcd[displayID].displ().clear(); rem_lcd[displayID].displ().print("Down"); break;
			}
		}

		mainLCD->setCursor(0, 2);
		if (loopTimeIndex != 0) {
			auto & device = getDevice(i2cAddr[i2cAddrIndex]);
			device.set_runSpeed(100000000 / loopTime[loopTimeIndex]);
			//i2C.setI2CFrequency(100000000 / loopTime[loopTimeIndex]);
			 logger() << "\n Loop set speed for", i2cAddr[i2cAddrIndex], "is", (int)device.runSpeed());
			mainLCD->print("Manual Speed ");
		}
		else {
			mainLCD->print("Optimal Speed ");
			auto & device = getDevice(i2cAddr[i2cAddrIndex]);
			 logger() << "\n Loop set speed: Optimal for", i2cAddr[i2cAddrIndex], "is", device.runSpeed());
		}
		mainLCD->setCursor(14, 2);
		switch (testMode) {
		case normal: mainLCD->print("Norm  "); break;
		case resetBeforeTest:  mainLCD->print("Reboot"); break;
		case speedTestExistsOnly:  mainLCD->print("Exists"); break;
		}
		shiftKeyQue();
	} // end nextKey >= 0
	//checkEEPROM(" EP Failed after KeyCheck");

	if (cycleDevices && (millis() > loopEndTime)) { // get next device
		//readRemoteKeyboard();
		//checkEEPROM(" EP Failed before Next-Device");
		i2c_recover.strategy().stackTrace(++st_index, "Next-Device");
		i2cAddrIndex = ++i2cAddrIndex;
		if (i2cAddrIndex >= sizeof(i2cAddr)) {
			i2cAddrIndex = 0;
			logger() << "\n Next Cycle\n";
			++tryCount;
		}
		loopEndTime = millis() + LOOP_TIME;
		displayNeedsRefreshing = true;
		// logger() << "\nMoved to next device", i2cAddr[i2cAddrIndex]);
	}

	//checkEEPROM(" EP Failed before tryDevice");
	uint8_t result = tryDeviceAt(i2cAddrIndex);
	//checkEEPROM(" EP Failed after tryDevice");

	if (result != lastResult) {
		//checkEEPROM(" EP Failed before result changed");
		displayNeedsRefreshing = true;
	}

	lastResult = result;
	//checkEEPROM(" EP Failed after assigning result");
	i2c_recover.strategy().stackTrace(++st_index, "Got Result");

	if (displayNeedsRefreshing) {
		mainLCD->setCursor(0, 0);
		mainLCD->print("I2C:0x");
		mainLCD->print(i2cAddr[i2cAddrIndex], HEX);
		mainLCD->print(" Spd:");
		printSpeed();
		mainLCD->setCursor(0, 1);
		mainLCD->print("      ");
	}
	mainLCD->setCursor(0, 1);

	if (result == 0) {
		//checkEEPROM(" EP Failed after result == 0");
		mainLCD->print("Failed");
		if (lastTimeGood[i2cAddrIndex] == 0) {
			lastTimeGood[i2cAddrIndex] = millis();
		}
		 logger() << "\n Failed for ", i2cAddr[i2cAddrIndex]);
		logger() << L_endl;
		i2c_recover.strategy().stackTrace(++st_index, "Printed Failed");
		//loopEndTime = millis() + LOOP_TIME;
	}
	else {
		if (lastTimeGood[i2cAddrIndex] != 0) {
			 logger() << "\n Recovered for ", i2cAddr[i2cAddrIndex], "after mS:", millis() - lastTimeGood[i2cAddrIndex]);
			lastTimeGood[i2cAddrIndex] = 0;
			// logger() << "\n Success :", min((successCount[i2cAddrIndex] * 100L) / tryCount, 100), "%");
			logger() << L_endl;
			i2c_recover.strategy().stackTrace(++st_index, "Printed Recovered");
		}
		++successCount[i2cAddrIndex];
		if (displayNeedsRefreshing) {
			mainLCD->print((int)result, DEC);
			mainLCD->print("     ");
		}
	}

	//checkEEPROM(" EP Failed after show result");

	if (displayNeedsRefreshing) {
		//checkEEPROM(" EP Failed before Counted good devices");
		int noOfGood = 0;
		for (int i = 0; i < sizeof(i2cAddr); ++i) {
			if (lastTimeGood[i] == 0)++noOfGood;
		}
		mainLCD->setCursor(0, 3);
		//checkEEPROM(" EP Failed after Counted good devices");
		i2c_recover.strategy().stackTrace(++st_index, "Counted good devices");
		bool sd_ok = logger().isWorking();
		//checkEEPROM(" EP Failed after Tested SD");
		i2c_recover.strategy().stackTrace(++st_index, "Tested SD");
		bool rtc_ok = clock_().ok();
		//checkEEPROM(" EP Failed after Tested clock");
		i2c_recover.strategy().stackTrace(++st_index, "Tested RTC");
		if (!sd_ok)       mainLCD->print("SD Card Failed.     ");
		else if (!rtc_ok) mainLCD->print("RTC Failed.         ");
		else		      mainLCD->print("No of devices:      ");
		mainLCD->setCursor(15, 3);
		mainLCD->print(noOfGood);
		mainLCD->print("/");
		mainLCD->print(sizeof(i2cAddr), DEC);
	}

	//delay(200);
	//checkEEPROM(" EP Failed before Get Clock-hours");
	i2c_recover.strategy().stackTrace(++st_index, "Get Clock-hours");
	if (clock_().hrs() != lastHours) {
		lastHours = clock_().hrs();
		 logger() << "\n Still looping...");
		//checkEEPROM(" EP Failed after getting time");
	}
	//checkEEPROM(" EP Failed at end of loop");
}

uint8_t tryDeviceAt(int addrIndex) {
	const uint8_t GPPU = 0x0C;    // GPio Pull Up
	uint8_t dataBuffa;
	const uint8_t NO_OF_ATTEMPTS = 5;
	uint8_t hasFailed = 0, attempts = NO_OF_ATTEMPTS;
	uint8_t addr = i2cAddr[addrIndex];
	// logger() << "\nTryDevice ", addr);
	auto & device = getDevice(addr);
	do { // try with hard reset
		i2c_recover.strategy().stackTrace(++st_index, "Try-Device - getStatus");
		//checkEEPROM(" EP Failed before tryDevice");
		if (!device.isEnabled()) device.reset();
		//if (i2C_h->getThisI2CFrequency(addr) == 0) {
		//	hasFailed = 1;
		//	dataBuffa = 0;
		//	continue;
		//}
		//if (i2c_recover.i2C_is_frozen()) {
		//	 logger() << "\nHung before try :", addr);
		//	//resetI2C_(*i2C_h, addr);
		//}
		mainLCD->setCursor(10, 1);
		if (attempts < NO_OF_ATTEMPTS) {
			mainLCD->print("Try:      ");
			mainLCD->setCursor(15, 1);
			mainLCD->print(NO_OF_ATTEMPTS - attempts + 1);
			printSpeed();
		} else mainLCD->print("                 ");
		dataBuffa = 1;
		if (addr == 0) {
		}
		else if (addr == 0x10) { // mix arduino
			//checkEEPROM(" EP Failed before mixValve.getPos(dataBuffa)");
			hasFailed = mixValve.getPos(dataBuffa);
		}
		else if (addr == 0x20) { // relay box
			//checkEEPROM(" EP Failed before rPort.initialiseDevice()");
			hasFailed = rPort.initialiseDevice();
		}
		else if (addr <= 0x26) {
			//checkEEPROM(" EP Failed before rem_lcd[addr - 0x24].initialiseDevice()");
			rem_lcd[addr - 0x24].initialiseDevice();
			hasFailed = rem_lcd[addr - 0x24].sendMessage("Hello World");
		}
		else {
			//checkEEPROM(" EP Failed before ts.readTemperature()");
			auto & ts = static_cast<I2C_Temp_Sensor &>(device);
			hasFailed = ts.readTemperature();
			dataBuffa = ts.get_temp();
		}
		//checkEEPROM(" EP Failed after tryDevice attemped");
		i2c_recover.strategy().stackTrace(++st_index, "Try-Device - attempted");

	} while (!keyIsWaiting() && hasFailed && --attempts > 0); // reset if hung

	if (hasFailed) {
		dataBuffa = 0;
		 logger() << "\n Failed after", NO_OF_ATTEMPTS, "attempts for", (int)addr);
		//checkEEPROM(" EP Failed after 5 tryDevice failures");
		i2c_recover.strategy().stackTrace(++st_index, "Try-Device - failed");
		// logger() << "\n tryDeviceAt() calling SpeedTest");
	}
	else if (NO_OF_ATTEMPTS - attempts > 0) {
		 logger() << "\n Succeeded after ", NO_OF_ATTEMPTS - attempts + 1, " attempts for", (int)addr);
		logger() << L_endl;
		i2c_recover.strategy().stackTrace(++st_index, "Try-Device - suceeded");
	}
	if (hasFailed == _OK && dataBuffa == 0) dataBuffa = 1;
	return dataBuffa;
}

/////////////////////////////// Keyboard functions ///////////////////////////

unsigned int analogReadDelay(byte an_pin) {
	delayMicroseconds(10000); // delay() does not work during an interrupt
	return analogRead(an_pin);   // read the value from the keypad
};

signed char readLocalKey() {
	unsigned int adc_key_in = analogReadDelay(KEY_ANALOGUE);   // read the value from the keypad
	while (adc_key_in != analogReadDelay(KEY_ANALOGUE)) {
		adc_key_in = analogReadDelay(KEY_ANALOGUE);
	}
	// Convert ADC value to key number
	//mainLCD->setCursor(0,1);
	//mainLCD->print(adc_key_in,DEC);  mainLCD->print("  ");

	signed char key;
	for (key = 0; key < NUM_LKEYS; key++) {
		if (adc_key_in > adc_LKey_val[key]) break;
	}
	if (key >= NUM_LKEYS) key = -1;     // No valid key pressed
	return key;
}

void readLocalKeyboard() { // interrupt handler
  // We have a new key-down so read the key
	signed char myKey = readLocalKey();
	while (myKey == 1 && digitalRead(LOCAL_INT_PIN) == LOW) {
		signed char newKey = readLocalKey();
		// see if a second key is pressed
		if (newKey >= 0) myKey = newKey;
	}
	// Store key info in array
	if (keyQuePos < 9 && myKey >= 0) {
		++keyQuePos;
		keyQue[keyQuePos][0] = 0;
		keyQue[keyQuePos][1] = myKey;
		// logger() << "\n readLocalKeyboard",myKey);
	}
}

void shiftKeyQue() {
	if (keyQuePos > 0) {
		for (byte i = 0; i < keyQuePos; ++i) {
			keyQue[i][0] = keyQue[i + 1][0];
			keyQue[i][1] = keyQue[i + 1][1];
		}
	}
	else {
		keyQue[0][1] = -1;
	}
	if (keyQuePos >= 0) --keyQuePos;
}

void readRemoteKeyboard() {
	//Serial.print("RC-ReadKey... ");
	for (int i = 0; i < sizeof(rem_lcd) / sizeof(rem_lcd[0]); ++i) {
		int8_t myKey = getKeyCode(rem_lcd[i].displ().readI2C_keypad());
		if (myKey >= 0 && keyQuePos < 9) {
			 logger() << "\nRC-ReadKey: ", myKey);
			++keyQuePos;
			keyQue[keyQuePos][0] = i + 1;
			keyQue[keyQuePos][1] = myKey;
		}
	}
}

int8_t getKeyCode(uint16_t gpio) {
	int8_t myKey;
	switch (gpio) {
	case 0x01: myKey = 2; break; // right
	case 0x20: myKey = 4; break; // down
	case 0x40: myKey = 1; break; // up
	case 0x80: myKey = 3; break; // left
	case 0:    myKey = -1; break;
	default: myKey = -2; // unrecognised
	}
	return myKey;
}

byte fromBCD(uint8_t value) {
	uint8_t decoded = value & 127;
	decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
	return decoded;
}

int I2C_Temp_Sensor::_error;

int8_t I2C_Temp_Sensor::readTemperature() {
	uint8_t temp[2] = { 0 };
	_error = read(DS75LX_Temp, 2, temp);
	// logger() << "\n readTemp()", getAddress(), getStatusMsg(_error) );

	_lastGood = (temp[0] << 8) | temp[1];
	return _error;
}


uint8_t I2C_Temp_Sensor::setHighRes() {
	write(DS75LX_Config, 0x60);
	return _error;
}

int8_t I2C_Temp_Sensor::get_temp() const {
	return (get_fractional_temp() + 128) / 256;
}

int16_t I2C_Temp_Sensor::get_fractional_temp() const {
	return _lastGood;
}

uint8_t I2C_Temp_Sensor::testDevice() {
	uint8_t temp[2] = { 75,0 };
	_error = write_verify(DS75LX_HYST_REG, 2, temp);
	// logger() << "\nTS at", getAddress(), getStatusMsg(_error));
	return _error;
}

// ************** Relay Port *****************
RelaysPort::RelaysPort(I2C_Recover & recovery, int addr, int8_t zeroCrossPin, int8_t resetPin)
	: I_I2Cdevice_Recovery(recovery, addr)
	, _zeroCrossPin(zeroCrossPin)
	, _resetPin(resetPin)
{
	pinMode(abs(_zeroCrossPin), INPUT);
	// logger() << "\nRelaysPort::setup()");
	digitalWrite(abs(_zeroCrossPin), HIGH); // turn on pullup resistors
	pinMode(abs(_resetPin), OUTPUT);
	digitalWrite(abs(_resetPin), (_resetPin < 0) ? HIGH : LOW);
}

uint8_t RelaysPort::initialiseDevice() {
	uint8_t pullUp_out[] = { 0 };
	uint8_t hasFailed = write_verify(REG_8PORT_PullUp, 1, pullUp_out); // clear all pull-up resistors
	if (hasFailed) {
		 logger() << "\nDevice 32. Initialise RelaysPort() write-verify failed at Freq:", i2C().getI2CFrequency());
	}
	else {
		hasFailed = write_verify(REG_8PORT_OLAT, 1, &relayRegister); // set latches
		writeInSync();
		hasFailed |= write_verify(REG_8PORT_IODIR, 1, pullUp_out); // set all as outputs
		if (hasFailed)  logger() << "\nInitialise RelaysPort() lat-write failed at Freq:", i2C().getI2CFrequency());
		//else  logger() << "\nInitialise RelaysPort() succeeded at Freq:", i2C().getI2CFrequency());
	}
	return hasFailed;
}

uint8_t RelaysPort::testDevice() {
	return write_verify(REG_8PORT_OLAT, 1, &relayRegister); // set latches
}

RemoteDisplay::RemoteDisplay(I2C_Recover & recovery, int addr)
	: I_I2Cdevice_Recovery(recovery, addr),
	_lcd(7, 6, 5,
		4, 3, 2, 1,
		0, 4, 3, 2,
		this, addr,
		0xFF, 0x1F) {
	//Serial.print("RemoteDisplay : "); Serial.print(addr, HEX); Serial.println(" Constructed");
}

RemoteDisplay::RemoteDisplay(int addr)
	: I_I2Cdevice_Recovery(addr),
	_lcd(7, 6, 5,
		4, 3, 2, 1,
		0, 4, 3, 2,
		this, addr,
		0xFF, 0x1F) {
	//Serial.print("RemoteDisplay : "); Serial.print(addr, HEX); Serial.println(" Constructed");
}

uint8_t RemoteDisplay::testDevice() {
	//Serial.println("RemoteDisplay.testDevice");
	uint8_t dataBuffa[2] = { 0x5C, 0x36 };
	return write_verify(0x04, 2, dataBuffa); // Write to GPINTEN
}

uint8_t RemoteDisplay::sendMessage(const char * msg) {
	uint8_t status = _OK;
	if (displ().print(msg) != strlen(msg)) status = _Insufficient_data_returned;
	return status;
}


uint8_t RemoteDisplay::initialiseDevice() {
	uint8_t rem_error = _lcd.begin(16, 2);
	// logger() << "\n Remote.begin() for display :", getAddress(), getStatusMsg(rem_error));
	if (rem_error) {
		// logger() << "\n Remote.begin() for display :", getAddress(), getStatusMsg(rem_error));
	}
	else {
		displ().print("Start ");
	}
	return rem_error;
}

uint8_t MixValveController::testDevice() {
	unsigned long waitTime = 3000UL + timeOfReset_mS_;
	unsigned long tryAgainTime = millis() + 2;
	if (waitTime < tryAgainTime) waitTime = tryAgainTime;
	uint8_t status;
	uint8_t dataBuffa[1] = { 55 };
	do {
		//i2c_recover.Wait_For_I2C_Data_Line_OK();
		status = I_I2Cdevice::write_verify(7, 1, dataBuffa); // write request temp
	} while (status && millis() < waitTime);
	tryAgainTime = millis() - tryAgainTime + 2;
	
	if (status)  logger() << "\n MixValveController::testDevice failed. Addr: 0x" << L_hex << getAddress() << getStatusMsg(status) << L_endl;
	else  logger() << "\n MixValveController::testDevice OK after mS" <<  tryAgainTime << L_endl;
	return status;
}

uint8_t MixValveController::getPos(uint8_t & pos) {
	unsigned long waitTime = 3000UL + timeOfReset_mS_;
	//unsigned long tryAgainTime = millis() + 3;
	//if (waitTime < tryAgainTime) waitTime = tryAgainTime;
	uint8_t status;
	do {
		//i2c_recover.Wait_For_I2C_Data_Line_OK();
		status = I_I2Cdevice::read(valve_pos, 1, &pos); // non-recovery read request temp
	} while (status && millis() < waitTime );

	if (status) status = read(valve_pos, 1, &pos); // read request temp
	//tryAgainTime = millis() - tryAgainTime + 3;
	//if (status)  logger() << "\n MixValveController::getPos failed. Addr:", getAddress());
	//else  logger() << "\n MixValveController::getPos OK after mS", tryAgainTime);
	return status;
}