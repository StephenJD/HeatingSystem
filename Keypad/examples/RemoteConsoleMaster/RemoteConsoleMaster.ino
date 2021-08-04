#include <Arduino.h>
#include <I2C_Talk.h>
//#include <Logging.h>
#include <avr/interrupt.h>
#include "RemoteKeypadMaster.h"
#include <Clock.h>
#include <Timer_mS_uS.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#define SERIAL_RATE 115200

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
constexpr auto OLED_MOSI = 11;  // DIn
constexpr auto OLED_CLK = 13;
constexpr auto OLED_DC = 9;
constexpr auto OLED_CS = 8;
constexpr auto OLED_RESET = 10;

U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI display(OLED_CS, OLED_DC, OLED_RESET);  // 2464/215 bytes

//////////////////////////////// Start execution here ///////////////////////////////
namespace HardwareInterfaces {
RemoteKeypadMaster remoteKeypad;
}

using namespace HardwareInterfaces;
//using namespace I2C_Talk_ErrorCodes;
//int portb;
//int keyPin;

const uint8_t PROGRAMMER_I2C_ADDR = 0x11;

Clock& clock_() {
  static Clock _clock{};
  return _clock;
}

//namespace arduino_logger {
//	Logger& logger() {
//		static Serial_Logger _log(SERIAL_RATE, L_flush);
//		return _log;
//	}
//}
//using namespace arduino_logger;

I2C_Talk i2C(DS_REMOTE_MASTER_I2C_ADDR, Wire, 100000); // default 400kHz

void applyKey(int keyCode);

void setup() {
  //logger() << F("Start") << L_endl;
  for (int pin = 0; pin < 18; ++pin) pinMode(pin, INPUT_PULLUP);
  i2C.begin();
  display.begin();  // 142/0 bytes
  applyKey(0);
  //display.setFont(u8x8_font_px437wyse700b_2x2_r);
  //display.drawString(0, 0, "px437_2x");

  // display.setFont(u8x8_font_5x7_r);
  //display.drawString(0, 2, "AbCdEfGgHiJ123");
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

int fontNo = 0;
const uint8_t* getFont(int fontNo) {
  while (true) {
    switch (fontNo) {  // no fonts: 18012 bytes (55%), no logging: 13886 bytes (43%), No I2C: 11724 bytes (36%), no display: 8668/525 bytes (26%). Display-only: 11132/740 bytes.
     // case  0:  return u8x8_font_amstrad_cpc_extended_f;  // 1574 bytes
      //case  1:  return u8x8_font_5x7_f; // 2,062 bytes
    //case  3:  return u8x8_font_8x13_1x2_f;  // 3,600 bytes
    //case 	4:  return 	u8x8_font_8x13B_1x2_f;
    //case 	5:  return 	u8x8_font_7x14_1x2_f; // 7,236
    //case 	6:  return 	u8x8_font_7x14B_1x2_f;
    //case 	7:  return 	u8x8_font_profont29_2x3_f; // 14,404
    //case 	8:  return 	u8x8_font_courB18_2x3_f;
    //case 	9:  return 	u8x8_font_courR18_2x3_f; // 7,168 c.f. 3
    //case 	10:  return 	u8x8_font_courB24_3x4_f;
    //case 	11:  return 	u8x8_font_courR24_3x4_f;
    //case 	12:  return 	u8x8_font_lucasarts_scumm_subtitle_o_2x2_f;
    //case 	13:  return 	u8x8_font_lucasarts_scumm_subtitle_r_2x2_f; // 10,724
    //case 	14:  return 	u8x8_font_inr21_2x4_f;
    //case 	15:  return 	u8x8_font_inr33_3x6_f;
    //case 	16:  return 	u8x8_font_inr46_4x8_f;
    //case 	17:  return 	u8x8_font_inb21_2x4_f;
    //case 	18:  return 	u8x8_font_inb33_3x6_f;
    //case 	19:  return 	u8x8_font_inb46_4x8_f;
    //case 	20:  return 	u8x8_font_pressstart2p_f; // 5,444
    //case 	21:  return 	u8x8_font_pcsenior_f; // 5,444
    case 	22:  return 	u8x8_font_pxplusibmcgathin_f; // 5,444 v nice
    case 	23:  return 	u8x8_font_pxplusibmcga_f; // 5,444 bold
    //case 	24:  return 	u8x8_font_pxplustandynewtv_f; // 5,444
    //case 	25:  return 	u8x8_font_px437wyse700a_2x2_f; // 10,820
    //case 	26:  return 	u8x8_font_px437wyse700b_2x2_f; // 10,820
      default:
        ++fontNo;
        ::fontNo = fontNo;
        if (fontNo > 26) {
          fontNo = 0;
          //return u8x8_font_amstrad_cpc_extended_f;
        }
    }
  }
}

enum Display_Mode { Sleep
            , Active
            , Increase
            , Decrease
            , TowelRail
            , HotWater 
          } mode(Wake);

enum Key_Mode {
  Wake
  ,UpDown
  ,LeftRight
}

Key_Mode keyMode(int keyCode) {
  if (keyCode == )
}

char msg[] = { "M1Aa2Bb3Cc4Dd5Ee6Ff7" };
char subMsg[] = { "M1Aa2Bb3Cc4Dd5Ee6Ff7" };
int noOfChars = 14;
int fontsize = 1;
int brightness = 255;

void changeMode(int keyCode) {
  
}

  void updateDisplay() {
  strncpy(subMsg, msg, noOfChars);
  subMsg[noOfChars] = 0;
  display.clear();                   // 80/0 bytes
  display.setFont(getFont(fontNo));  // 1580/0 bytes
  display.setCursor(0, 0);           // 0/0 bytes
  display.print(fontNo);             // 260/0 bytes
  display.print(subMsg);             // 44/0 bytes
  //display.setCursor(0, 1);
  //display.print(subMsg);
  display.setCursor(0, 2);
  display.print(subMsg);
  //display.setCursor(0, 3);
  //display.print(subMsg); 
}

//Timer_mS timeToReadKey{ 10 };
/*
void readKeypad() {
  if (timeToReadKey) {
    timeToReadKey.repeat();
    for (auto& pin : RemoteKeypadMaster::KeyPins) {
      if (pin.pinChanged() == 1) {
        auto myKey = remoteKeypad.getKeyCode(pin.port());
        remoteKeypad.putKey(myKey);
      }
    }
  }  
}
*/

void loop() {
  remoteKeypad.readKey();
  auto key = remoteKeypad.popKey();
  if (wakeDisplay(key)) {
    if (key/2 == MODE_LEFT_RIGHT) changeMode(key);
    else changeValue(key);   
    updateDisplay();
    sleepDisplay();  
  }
}