#include "OLED_Master_Display.h"
#include <MsTimer2.h>
#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#include "I2C_Talk.h"
#include "I2C_Recover.h"
#include "I2C_RecoverRetest.h"


using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;
using namespace HardwareInterfaces;

I2C_Talk i2C(DS_REMOTE_MASTER_I2C_ADDR, Wire, 100000);
I2C_Recover i2c_recover(i2C);
uint8_t reg = 0;

namespace OLED_Master_Display {
    constexpr auto SCREEN_WIDTH = 128;  // OLED display width, in pixels
    constexpr auto SCREEN_HEIGHT = 32; // OLED display height, in pixels
    constexpr auto FONT_WIDTH = 8;
    // Declaration for SSD1306 display connected using software SPI (default case):
    constexpr auto OLED_MOSI = 11;  // DIn
    constexpr auto OLED_CLK = 13;
    constexpr auto OLED_DC = 9;
    constexpr auto OLED_CS = 8;
    constexpr auto OLED_RESET = 10;


    int8_t _sleepRow = -1;
    int8_t _sleepCol = 0;
    RemoteKeypadMaster _remoteKeypad{ 50,10 };
    U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI _display(OLED_CS, OLED_DC, OLED_RESET);
    Display_Mode _display_mode = RoomTemp;
    Registers _registers;

    void begin() { 
        _display.begin();
        displayPage();
    }
    void startDisplaySleep();
    void stopDisplaySleep();
    static void moveDisplay();
    const uint8_t* getFont(bool bold = false);
    void changeMode(int keyCode);
    void changeValue(int keyCode);

    void startDisplaySleep() {
        _sleepRow = 0; _sleepCol = 0;
        MsTimer2::set(SLEEP_MOVE_PERIOD_mS, moveDisplay);
        MsTimer2::start();
    }

    void stopDisplaySleep() {
        MsTimer2::stop();
        _sleepRow = -1;
        Serial.println("Wake...");
    }

    void moveDisplay() {
        ++_sleepCol;
        if (_sleepCol > SCREEN_WIDTH / FONT_WIDTH - 2) {
            _sleepCol = 0;
            ++_sleepRow;
            if (_sleepRow > 3) _sleepRow = 0;
        }
        sleepPage();
    }

    const uint8_t* getFont(bool bold) {
        if (bold) return 	u8x8_font_pxplusibmcga_f; // 5,444 bold
        else return 	u8x8_font_pxplusibmcgathin_f; // 5,444 v nice
    }

    void changeMode(int keyCode) {
        auto newMode = NoOfDisplayModes + _display_mode + (keyCode == I_Keypad::KEY_LEFT ? -1 : 1);
        _display_mode = Display_Mode(newMode % NoOfDisplayModes);
        _registers.dataChanged = true;
    }

    void changeValue(int keyCode) {
        auto increment = keyCode == I_Keypad::KEY_UP ? 1 : -1;
        switch (_display_mode) {
        case RoomTemp: _registers.modifyRegister(_registers.roomTempRequest, increment); break;
        case TowelRail: _registers.setRegister(_registers.towelRailRequest, (_registers.getRegister(_registers.towelRailRequest) + increment) % 2); break;
        case HotWater: _registers.setRegister(_registers.hotWaterRequest, (_registers.getRegister(_registers.hotWaterRequest) + increment) % 2); break;
        }
        _registers.dataChanged = true;
    }

    void displayPage() {
        stopDisplaySleep();
        _display.clear();
        _display.setFont(getFont());
        _display.setCursor(0, 0);
        switch (_display_mode) {
        case RoomTemp:
            _display.print(F("Room Temperature"));
            _display.setCursor(0, 1);
            _display.print(F("Ask "));
            _display.setFont(getFont(true));
            _display.print(_registers.roomTempRequest);
            _display.setFont(getFont());
            _display.print(F(" Is "));
            _display.print(_registers.roomTemp);
            _display.setCursor(0, 2);
            _display.print(F("Warmup in "));
            _display.print(_registers.roomWarmupTime / 6.);
            _display.print(F("h"));
            break;
        case TowelRail:
            _display.print(F("Towel Rail"));
            _display.setCursor(0, 1);
            _display.setFont(getFont(true));
            _display.print(_registers.towelRailRequest ? F("Manual ") : F("Auto "));
            _display.setFont(getFont());
            _display.print(_registers.towelRailOnTime ? F("(Is On)") : F("(Is Off)"));
            break;
        case HotWater:
            _display.print(F("Hot Water"));
            _display.setCursor(0, 1);
            _display.setFont(getFont(true));
            _display.print(_registers.hotWaterRequest ? F("Manual ") : F("Auto "));
            _display.setFont(getFont());
            _display.setCursor(0, 2);
            _display.print(F("Warm in "));
            if (_registers.hotWaterWarmupTime <= 60) {
                _display.print(_registers.hotWaterWarmupTime);
                _display.print(F("m"));
            } else {
                _display.print(_registers.hotWaterWarmupTime / 60.);
                _display.print(F("h"));
            }
        }
        _registers.dataChanged = false;
    }

    void sleepPage() {
        _display.clear();
        _display.setFont(getFont());
        _display.setCursor(_sleepCol, _sleepRow);
        _display.print(_registers.roomTemp);
    }

    void processKeys() {
      _remoteKeypad.readKey();
      _remoteKeypad.oneSecondElapsed();
      auto key = _remoteKeypad.popKey();
      if (key > I_Keypad::NO_KEY) Serial.println(key);
      if (key > I_Keypad::NO_KEY) {
        if (key == I_Keypad::KEY_WAKEUP ) {
          displayPage();
        } else {
          if (key/2 == I_Keypad::MODE_LEFT_RIGHT) {
            changeMode(key);
          }
          else {
            changeValue(key); 
          }
        }
      }
      if (_registers.dataChanged) displayPage();
      else if(!_remoteKeypad.displayIsAwake() && _sleepRow == -1) {
        Serial.println("Sleep...");
        startDisplaySleep();
      }
    }

    // Called when data is sent by Master, telling slave how many bytes have been sent.
    void receiveI2C(int howMany) {
        // must not do a Serial.print when it receives a register address prior to a read.
        //	delayMicroseconds(1000); // for testing only
        uint8_t msgFromMaster[3]; // = [register, data-0, data-1]
        if (howMany > 3) howMany = 3;
        int noOfBytesReceived = i2C.receiveFromMaster(howMany, msgFromMaster);
        if (noOfBytesReceived) {
            reg = msgFromMaster[0];
            if (reg >= Registers::reg_size) reg = 0;
            // If howMany > 1, we are receiving data, otherwise Master has selected a register ready to read from.
            // All writable registers are single-byte, so just read first byte sent.
            if (howMany > 1) _registers.setRegister(reg, msgFromMaster[1]);
        }
    }

    // Called when data is requested.
    // The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
    // To keep things simple, we will send a max of two-bytes.
    // Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
    // But I2C devices use big-endianness: MSB at the smallest address: So a uint16_t is [MSB, LSB].
    // A single read returns the MSB of a 2-byte value (as in a Temp Sensor), 2-byte read is required to get the LSB.
    // To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most. 
    // getRegister() returns uint8_t as uint16_t which puts LSB at the lower address, which is what we want!
    // We can send the address of the returned value to i2C().write().
    // 
    void requestEventI2C() {
        //	delayMicroseconds(1000); // for testing only
        uint16_t regVal = _registers.getRegister(reg);
        //uint8_t * valPtr = (uint8_t *)&regVal;
        // Mega OK with logging, but DUE master is not!
        //logger() << F("requestEventI2C: mixV[") << int(regSet) << F("] Reg: ") << int(reg) << F(" Sent:") << regVal << F(" [") << *(valPtr++) << F(",") << *(valPtr) << F("]") << L_endl;
        i2C.write((uint8_t*)&regVal, 2);
    }
}