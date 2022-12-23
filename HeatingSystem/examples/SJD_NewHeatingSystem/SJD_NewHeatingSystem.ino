#include <HeatingSystem.h>
#include <HardwareInterfaces\A__Constants.h>
#include <Relay_Bitwise.h>
#include <Clock_I2C.h>
#include <Logging_SD.h>
#include <Logging_Loop.h>
#include <Logging_Ram.h>
#include <I2C_Talk.h>
#include <I2C_RecoverRetest.h>
#include <Mega_Due.h>
#include <EEPROM_RE.h>
#include <Wire.h>
#include <MemoryFree.h>
#include <Watchdog_Timer.h>
#include <Arduino.h>
#include <Timer_mS_uS.h>

using namespace HardwareInterfaces;

constexpr int WATCHDOG_TIMOUT = 16000; // 8000 max for Mega.

constexpr uint8_t RTC_RESET_PIN = 4;
constexpr uint8_t RTC_ADDRESS = 0x68;

#if defined(__SAM3X8E__)
	// Following are extern'd so must not be constexpr
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

	EEPROMClassRE & eeprom() {
		return EEPROM;
	}

	Clock & clock_() {
		static Clock_EEPROM _clock(Assembly::EEPROM_CLOCK_ADDR);
		return _clock;
	}

	const int ramFileSize = 1000;
#endif

namespace arduino_logger {
	Logger& logger() {
		//static Serial_Logger _log(SERIAL_RATE);
		//static RAM_Logger _log("R", ramFileSize, true, clock_());
		//static EEPROM_Logger _log("E", EEPROM_LOG_START, EEPROM_LOG_END, false, clock_());
		static SD_Logger _log("E", SERIAL_RATE, clock_()/*, L_allwaysFlush*/);
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
	}
}

 namespace HardwareInterfaces {
 	Bitwise_RelayController & relayController() {
 		static RelaysPort _relaysPort(0x7F, heating_system->recoverObject(), RL_PORT_I2C_ADDR);
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
 		//Serial.println("Start ui_yield");
 		if (heating_system) heating_system->serviceConsoles_OK();
 		//Serial.println("End ui_yield");
 		inYield = false;
 	}
 }
 
void WDT_Handler(void) {
	reset_watchdog();
	// disable interrupt.
	//NVIC_DisableIRQ(WDT_IRQn);
	// save mem addresses in gen backup registers.
	pinMode(RESET_5vREF_PIN, OUTPUT);
	digitalWrite(RESET_5vREF_PIN, LOW);

	//HardReset::arduinoReset("Watchdog Timer");
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	{
		set_watchdog_timeout_mS(WATCHDOG_TIMOUT);
		Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
		Serial.println("Start...");
		Serial.flush();
		resetRTC(rtc, rtc_reset_wag); // Before logging to avoid no-date SD logs.
		logger() << L_flush;
		logger() << L_time << F(" ****** Arduino Restarted ") << micros()/1000 << F("mS ago. Timeout: ") << WATCHDOG_TIMOUT / 1000 << F("S\n\n") << L_flush;
		logger() << F("set_watchdog_timeout_mS: ") << WATCHDOG_TIMOUT << L_endl;
		LocalDisplay tempDisplay{};
		tempDisplay.setCursor(0, 0);
		tempDisplay.print("Start ");
		tempDisplay.print(clock_().c_str());
		tempDisplay.clearFromEnd();
		//set_watchdog_interrupt_mS(WATCHDOG_TIMOUT);
		//logger().begin(SERIAL_RATE);
		zTempLogger() << L_time << F(" ****** Arduino Restarted ******\n\n") << L_flush;
		profileLogger() << L_time << F(" ****** Arduino Restarted ******\n\n") << L_flush;
		reset_watchdog();
		logger() << F("RTC Speed: ") << rtc.getI2CFrequency() << L_endl;
		pinMode(RESET_LEDP_PIN, OUTPUT);
		digitalWrite(RESET_LEDP_PIN, HIGH);
		analogWrite(BRIGHNESS_PWM, BRIGHTNESS);  // Brightness analogRead values go from 0 to 1023, analogWrite values from 0 to 255
		analogWrite(CONTRAST_PWM, contrast());  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255
		HeatingSystemSupport::initialise_virtualROM();
		logger() << F("Construct HeatingSystem") << L_endl;
		reset_watchdog();
	}
	HeatingSystem hs{};
	heating_system = &hs;
	while (true) {
		reset_watchdog();	
		hs.run_stateMachine();
	}
}

void loop(){
}
