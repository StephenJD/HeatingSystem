
#include <HeatingSystem.h>
#include <HardwareInterfaces\A__Constants.h>
#include <Relay_Bitwise.h>
#include <Clock_I2C.h>
#include <Logging_SD.h>
#include <I2C_Talk.h>
#include <I2C_RecoverRetest.h>
#include <EEPROM_RE.h>
#include <Wire.h>
#include <MemoryFree.h>
#include <Watchdog_Timer.h>
#include <Arduino.h>
#include <Timer_mS_uS.h>

using namespace HardwareInterfaces;

constexpr uint32_t SERIAL_RATE = 115200;
constexpr int WATCHDOG_TIMOUT = 8000; // max for Mega.

constexpr uint8_t RTC_RESET_PIN = 4;
constexpr uint8_t RTC_ADDRESS = 0x68;
constexpr uint8_t  dsBacklight = 14;
constexpr uint8_t  dsContrast = 60;
constexpr uint8_t  dsBLoffset = 14;
// Following are extern'd so must not be constexpr
uint8_t PHOTO_ANALOGUE = A0;
uint8_t  BRIGHNESS_PWM = 5; // pins 5 & 6 are not best for PWM control.
uint8_t  CONTRAST_PWM = 6;

#if defined(__SAM3X8E__)
	#define DIM_LCD 200

	I2C_Talk rtc{ Wire1 };
	I2C_Recovery::I2C_Recover_Retest r2c_recover(rtc);

	EEPROMClassRE & eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ r2c_recover, ( rtc.ini(Wire1,100000),rtc.extendTimeouts(5000, 5, 1000),EEPROM_I2C_ADDR) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}
	EEPROMClassRE & EEPROM = eeprom();

	Clock & clock_() {
		static Clock_I2C<rtc> _clock((rtc.ini(Wire1, 100000), rtc.extendTimeouts(5000, 5, 1000), RTC_ADDRESS));
		return _clock;
	}

	const float megaFactor = 1;
	const char * LOG_FILE = "D.txt";
	const int ramFileSize = 10000;
	auto rtc_reset_wag = Pin_Wag{ RTC_RESET_PIN, HIGH };

	uint8_t resetRTC(I2C_Talk & i2c, Pin_Wag & reset) {
		reset.set();
		delay(200);
		reset.clear();
		delay(200);
		i2c.begin();
		return 0;
	}
#else
	//Code in here will only be compiled if an Arduino Mega is used.
	#define DIM_LCD 135
	#define NO_RTC

	EEPROMClassRE & eeprom() {
		return EEPROM;
	}

	Clock & clock_() {
		static Clock_EEPROM _clock(Assembly::EEPROM_CLOCK_ADDR);
		return _clock;
	}

	const float megaFactor = 3.3 / 5;
	const int ramFileSize = 1000;
#endif

namespace arduino_logger {
	Logger& logger() {
		//static Serial_Logger _log(SERIAL_RATE);
		//static RAM_Logger _log("R", ramFileSize, true, clock_());
		//static EEPROM_Logger _log("E", EEPROM_LOG_START, EEPROM_LOG_END, false, clock_());
		static SD_Logger _log("E", SERIAL_RATE, clock_());
		return _log;
	}

	Logger& zTempLogger() {
		static SD_Logger _log("Z", SERIAL_RATE, clock_()/*, L_null*/);
		return _log;
	}

	Logger& profileLogger() {
		static SD_Logger _log("P", SERIAL_RATE, clock_()/*, L_null*/);
		return _log;
	}
}
using namespace arduino_logger;

HeatingSystem * heating_system = 0;

LocalLCD_Pinset localLCDpins() {
	return{ 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26 }; // new board
}

namespace HeatingSystemSupport {
	bool dataHasChanged;
}

namespace LCD_UI {
	void notifyDataIsEdited() { // global function for notifying anyone who needs to know
		HeatingSystemSupport::dataHasChanged = true;
		// Checks Zone Temps, then sets each zone.nextEvent to now.
		//if (heating_system) heating_system->updateChangedData();
	}
}

 namespace HardwareInterfaces {
 	Bitwise_RelayController & relayController() {
 		static RelaysPort _relaysPort(0x7F, heating_system->recoverObject(), IO8_PORT_OptCoupl);
 		return _relaysPort;
 	}
 }

  void ui_yield() {
 	static bool inYield = false;
 	static Timer_mS yieldTime{ 50 };
 	if (yieldTime) {
 		yieldTime.repeat();
 		reset_watchdog();
 		if (inYield) {
 			//logger() << F("\n*******  Recursive ui_yield call *******\n\n");
 			return;
 		}
 		inYield = true;
 		//if (millis()%1000 == 0) logger() << F("ui_yield call") << L_endl;
 		//Serial.println("Start ui_yield");
 		if (heating_system) heating_system->serviceConsoles();
 		//Serial.println("End ui_yield");
 		inYield = false;
 	}
 }

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	Serial.println("Start...");
	set_watchdog_timeout_mS(60000);
	resetRTC(rtc, rtc_reset_wag); // Before logging to avoid no-date SD logs.
	//logger().begin(SERIAL_RATE);
	logger() << L_flush;
	logger() << L_time << F(" ****** Arduino Restarted ") << millis() << F("mS ago. Timeout: ") << WATCHDOG_TIMOUT/1000 << F("S\n\n") << L_flush;
	zTempLogger() << L_time << F(" ****** Arduino Restarted ******\n\n") << L_flush;
	profileLogger() << L_time << F(" ****** Arduino Restarted ******\n\n") << L_flush;
	logger() << F("RTC Speed: ") << rtc.getI2CFrequency() << L_endl;
	pinMode(RESET_LEDP_PIN, OUTPUT);
	digitalWrite(RESET_LEDP_PIN, HIGH);
	auto supplyVcorrection = 2.5 * 1024. / 3.3 / analogRead(RESET_5vREF_PIN);
	logger() << "supplyVcorrection: " << supplyVcorrection << " dsContrast: " << dsContrast << " corrected:" << dsContrast * supplyVcorrection << L_endl;
	analogWrite(BRIGHNESS_PWM, 255);  // Brightness analogRead values go from 0 to 1023, analogWrite values from 0 to 255
	analogWrite(CONTRAST_PWM, dsContrast * supplyVcorrection);  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255
	HeatingSystemSupport::initialise_virtualROM();
	logger() << F("Construct HeatingSystem") << L_endl;
	HeatingSystem hs{};
	heating_system = &hs;
	logger() << L_time << F("set_watchdog_timeout_mS: ") << WATCHDOG_TIMOUT << L_endl;
	set_watchdog_timeout_mS(WATCHDOG_TIMOUT);
	while (true) {
		//Serial.println("Loop");
		hs.updateChangedData();
		hs.serviceTemperatureController();
		hs.serviceConsoles();
		//logger().flush();
	}
}

void loop(){
}
