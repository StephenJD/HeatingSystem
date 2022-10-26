// This is the Multi-Master Arduino Mini Controller

//#include <MemoryFree.h>
#include <Relays.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_Device.h>
//#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h>
#include <PinObject.h>
#include <Logging.h>
#include <EEPROM_RE.h>
#include <Timer_mS_uS.h>
#include <Watchdog_Timer.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>

// ******** NOTE select Arduino Pro Mini 328P 3.3v 8MHz **********
// Arduino Pro Mini must be upgraded with  boot-loader to enable watchdog timer.
// File->Preferences->Additional Boards Manager URLs: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
// Tools->Board->Boards Manager->Install MiniCore
// 1. In Arduino: Load File-> Examples->ArduinoISP->ArduinoISP
// 2. Upload this to any board, including another Mini Pro.
// 3. THEN connect SPI pins from the ISP board to the Mini Pro: MOSI(11/51)->11, MISO(12/50)->12, SCK(13/52)->13, 10->Reset on MiniPro, Vcc and 0c pins.
// 4. Tools->Board->MiniCore Atmega 328
// 5. External 8Mhz clock, BOD 2.7v, LTO enabled, Variant 328P, UART0.
// 6. Tools->Programmer->Arduino as ISP
// 7. Burn Bootloader
// 8. Then upload your desired sketch to ordinary ProMini board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

// ********** NOTE: ***************
// If watchdog is being used, set_watchdog_timeout_mS() must be set at the begining of startup, before any calls to delay()
// Otherwise, depending on how cleanly the Arduino shuts down, it may time-out before it is reset.
// ********************************

using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;

constexpr uint32_t SERIAL_RATE = 115200;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_allwaysFlush);
		return _log;
	}
}
using namespace arduino_logger;

EEPROMClassRE& eeprom() {
  static EEPROMClassRE _eeprom;
  return _eeprom;
}

I2C_Talk& i2C() {
  static I2C_Talk _i2C{};
  return _i2C;
}

I2C_Recover_Retest i2c_recover(i2C());
//I2C_Recover i2c_recover(i2C());
I_I2Cdevice_Recovery programmer = {i2c_recover, PROGRAMMER_I2C_ADDR };

auto led_Status = Pin_Wag(LED_BUILTIN, HIGH);

// All I2C transfers are initiated by Master
auto rl_register_set = i2c_registers::Registers<RelaysSlave::RL_ALL_REG_SIZE>{i2C()};
i2c_registers::I_Registers& rl_registers = rl_register_set;

RelaysSlave  relays = {i2c_recover, eeprom()};

auto nextSecond = Timer_mS(100);

void setup() {
	relays.begin();
	set_watchdog_timeout_mS(8000);
	logger().begin(SERIAL_RATE);
	logger() << F("Setup Started") << L_flush;
	i2C().setAsSlave(RL_PORT_I2C_ADDR);
	i2C().setTimeouts(WORKING_SLAVE_BYTE_PROCESS_TIMOUT_uS, I2C_Talk::WORKING_STOP_TIMEOUT, 10000);
	i2C().setMax_i2cFreq(100000);
	i2C().begin();

	for (int i = 0; i < 5; ++i) { // signal a reset
		led_Status.set();
		delay(50);
		led_Status.clear();
		delay(50);
	}

	delay(500);
	i2C().onReceive(rl_register_set.receiveI2C);
	i2C().onRequest(rl_register_set.requestI2C);
	logger() << F("Setup complete.") << L_flush;
}

void loop() {
	if (nextSecond) { // once per second
		nextSecond.repeat();
		if (relays.verifyConnection()) reset_watchdog();
		//relays.test();
		relays.setPorts();
	}
}