#include <Arduino.h>
#include <I2C_Talk.h>
#include <Logging.h>
#include <avr/interrupt.h>
#include "RemoteKeypadMaster.h"
#include <Clock.h>
#include <Timer_mS_uS.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#define SERIAL_RATE 115200

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
constexpr auto OLED_MOSI = 11; // DIn
constexpr auto OLED_CLK = 13;
constexpr auto OLED_DC = 8;
constexpr auto OLED_CS = 10;
constexpr auto OLED_RESET = 9;

U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI  display(OLED_CS, OLED_DC, OLED_RESET);

//////////////////////////////// Start execution here ///////////////////////////////
namespace HardwareInterfaces { RemoteKeypadMaster remoteKeypad; }

using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
//int portb;
//int keyPin;

const uint8_t PROGRAMMER_I2C_ADDR = 0x11;

Clock& clock_() {
	static Clock _clock{};
	return _clock;
}

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_flush);
		return _log;
	}
}
using namespace arduino_logger;

I2C_Talk i2C(DS_REMOTE_MASTER_I2C_ADDR, Wire, 100000); // default 400kHz

void setup()
{
  logger() << F("Start") << L_endl;
  for (int pin = 0; pin < 18; ++pin ) pinMode(pin, INPUT_PULLUP);
  i2C.begin();
  display.begin();
  display.setFont(u8x8_font_px437wyse700b_2x2_r);
  display.drawString(0, 0, "px437_2x");

  display.setFont(u8x8_font_5x7_r);
  display.drawString(0, 2, "AbCdEfGgHiJ123");
}
	
auto keyName(int keyCode) -> const __FlashStringHelper* {
		switch (keyCode) {
		case -1: return F("None");
		case -2: return F("Multiple");
		case 2: return F("Left");
		case 4: return F("Down");
		case 1: return F("Up");
		case 3: return F("Right");
		default: return F("Err");
		}
}

Timer_mS readKey{ 10 };

void loop()
{
	if (readKey) {
		readKey.repeat();
		for (auto& pin : RemoteKeypadMaster::KeyPins) {
			auto keyState = pin.pinChanged();
			if (keyState == 1) {
				auto myKey = remoteKeypad.getKeyCode(pin.port());
				remoteKeypad.putKey(myKey);
			} else if (keyState == -1) {
				//led.clear();
				logger() << "Release\n";
			}
		}

	}

	auto key = remoteKeypad.getKey();
	if (key >= 0) {
		while (key >= 0) {
			//led.set();
			i2C.write(PROGRAMMER_I2C_ADDR, 0, key);
			logger() << "Send " << keyName(key) << L_endl;
			key = remoteKeypad.getKey();
		}
	}


	static auto contrast = 255;
	static bool inverse = false;
	inverse = !inverse;
	display.clearLine(3);
	display.drawGlyph(0, 3, 'A');
	display.setCursor(1, 3);
	display.setInverseFont(inverse);
	display.print(contrast);
	//display.print((inverse ? " Inverse" : " Normal"));
	display.setInverseFont(0);
	display.setContrast(contrast);
	contrast -= 10;
	if (contrast < 0) contrast = 255;
	delay(500);
}

