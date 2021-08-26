#include "OLED_Master_Display.h"
#include <MsTimer2.h>

#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h>
#include <I2C_Registers.h>
#include <TempSensor.h>
#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;
using namespace HardwareInterfaces;

I2C_Talk i2C;
I2C_Recover i2c_recover(i2C);
auto tempSensor = TempSensor{ i2c_recover };
constexpr auto slave_requesting_resend_data = 0;

namespace OLED_Master_Display {

    // Free functions
    int8_t _sleepRow = -1;
    int8_t _sleepCol = 0;
    bool dataChanged;

    RemoteKeypadMaster _remoteKeypad{ 50,10 };
    U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI _display(OLED_CS, OLED_DC, OLED_RESET);
    Display_Mode _display_mode = RoomTemp;
    auto rem_registers = i2c_registers::Registers<remoteRegister_size>{i2C};

    void setRemoteI2CAddress() {
        pinMode(5, INPUT_PULLUP);
        pinMode(6, INPUT_PULLUP);
        pinMode(7, INPUT_PULLUP);
        if (!digitalRead(5)) {
            i2C.setAsMaster(DS_REMOTE_I2C_ADDR);
        } else if (!digitalRead(6)) {
            i2C.setAsMaster(US_REMOTE_I2C_ADDR);
        } else if (!digitalRead(7)) {
            i2C.setAsMaster(FL_REMOTE_I2C_ADDR);
        } else Serial.println(F("Err: None of Pins 5-7 selected."));
    }

    void sendDataToProgrammer() {
        //Serial.print(F("Send to Offs ")); Serial.println(rem_registers.getRegister(remote_register_offset));
        i2C.write(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + roomTemp, 5, rem_registers.reg_ptr(roomTemp));
    }

    void requestDataFromProgrammer() {
        i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + roomTempRequest, 5, rem_registers.reg_ptr(roomTempRequest));
    }

    void requestRegisterOffsetFromProgrammer() {
        uint8_t registerOffsetReqStillLodged = 1;
        while (i2C.write(PROGRAMMER_I2C_ADDR, slave_requesting_resend_data, registerOffsetReqStillLodged) != _OK);
        do {
            Serial.flush();
            Serial.println(F("wait for offset..."));
            delay(100);
            i2C.read(PROGRAMMER_I2C_ADDR, slave_requesting_resend_data, 1, &registerOffsetReqStillLodged);
        } while (registerOffsetReqStillLodged);
    }

    void begin() {
        i2C.begin();
        setRemoteI2CAddress();
        i2C.onReceive(rem_registers.receiveI2C);
        i2C.onRequest(rem_registers.requestI2C);
        requestRegisterOffsetFromProgrammer();
        tempSensor.initialise(rem_registers.getRegister(roomTempSensorAddr));
        tempSensor.setHighRes();
        requestDataFromProgrammer();
        sendDataToProgrammer();
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
        dataChanged = true;
    }

    void changeValue(int keyCode) {
        auto increment = keyCode == I_Keypad::KEY_UP ? 1 : -1;
        switch (_display_mode) {
        case RoomTemp:
            rem_registers.addToRegister(roomTempRequest, increment);
            Serial.print(F("Rr ")); Serial.println(rem_registers.getRegister(roomTempRequest));
            break;
        case TowelRail: rem_registers.setRegister(towelRailRequest, (rem_registers.getRegister(towelRailRequest) + increment) % 2); break;
        case HotWater: rem_registers.setRegister(hotWaterRequest, (rem_registers.getRegister(hotWaterRequest) + increment) % 2); break;
        }
        dataChanged = true;
        sendDataToProgrammer();
        requestDataFromProgrammer();
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
            _display.print(rem_registers.getRegister(roomTempRequest));
            _display.setFont(getFont());
            _display.print(F(" Is "));
            _display.print(rem_registers.getRegister(roomTemp));
            _display.setCursor(0, 2);
            _display.print(F("Warmup in "));
            _display.print(rem_registers.getRegister(roomWarmupTime_m10) / 6.);
            _display.print(F("h"));
            break;
        case TowelRail:
            _display.print(F("Towel Rail"));
            _display.setCursor(0, 1);
            _display.setFont(getFont(true));
            _display.print(rem_registers.getRegister(towelRailRequest) ? F("Manual ") : F("Auto "));
            _display.setFont(getFont());
            _display.print(rem_registers.getRegister(towelRailOnTime_m) ? F("(Is On)") : F("(Is Off)"));
            break;
        case HotWater:
            _display.print(F("Hot Water"));
            _display.setCursor(0, 1);
            _display.setFont(getFont(true));
            _display.print(rem_registers.getRegister(hotWaterRequest) ? F("Manual ") : F("Auto "));
            _display.setFont(getFont());
            _display.setCursor(0, 2);
            _display.print(F("Warm in "));
            if (rem_registers.getRegister(hotWaterWarmupTime_m10) <= 0) {
                _display.print(-rem_registers.getRegister(hotWaterWarmupTime_m10));
                _display.print(F("m"));
            } else {
                _display.print(rem_registers.getRegister(hotWaterWarmupTime_m10) / 6.);
                _display.print(F("h"));
            }
        }
        dataChanged = false;
    }

    void sleepPage() {
        _display.clear();
        _display.setFont(getFont());
        _display.setCursor(_sleepCol, _sleepRow);
        _display.print(rem_registers.getRegister(roomTemp));
    }

    void processKeys() {
      _remoteKeypad.readKey();
      if (_remoteKeypad.oneSecondElapsed()) {
          readTempSensor();
          requestDataFromProgrammer();
      }
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
      if (dataChanged) displayPage();
      else if(!_remoteKeypad.displayIsAwake() && _sleepRow == -1) {
        Serial.println("Sleep...");
        startDisplaySleep();
      }
    }

    void readTempSensor() {
        auto fractional_temp = tempSensor.get_fractional_temp();
        rem_registers.setRegister(roomTemp, fractional_temp >> 8);
        uint8_t fraction = uint8_t(fractional_temp);
        if (fraction != rem_registers.getRegister(roomTemp_fraction)) {
            rem_registers.setRegister(roomTemp_fraction, fraction);
            sendDataToProgrammer();
        }
    }

}