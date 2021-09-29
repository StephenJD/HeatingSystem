#include "OLED_Master_Display.h"
#include <MsTimer2.h>

#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h>
#include <I2C_Registers.h>
#include <TempSensor.h>
#include <Conversions.h>
#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace GP_LIB;

I2C_Talk i2C;
I2C_Recover i2c_recover(i2C);
auto tempSensor = TempSensor{ i2c_recover };
constexpr auto R_SLAVE_REQUESTING_INITIALISATION = 0;
enum SlaveRequestIni {
    NO_INI_REQUESTS
    , MV_US_REQUESTING_INI = 1
    , MV_DS_REQUESTING_INI = 2
    , RC_US_REQUESTING_INI = 4
    , RC_DS_REQUESTING_INI = 8
    , RC_F_REQUESTING_INI = 16
    , ALL_REQUESTING = (RC_F_REQUESTING_INI * 2) - 1
}; 
auto REQUESTING_INI = RC_US_REQUESTING_INI;

namespace OLED_Master_Display {

    // Free functions
    int8_t _sleepRow = -1;
    int8_t _sleepCol = 0;
    bool dataChanged;

    RemoteKeypadMaster _remoteKeypad{ 50,10 };
    U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI _display(OLED_CS, OLED_DC, OLED_RESET);
    Display_Mode _display_mode = RoomTemp;
#ifdef ZPSIM
    auto rem_registers = i2c_registers::Registers<R_DISPL_REG_SIZE, US_CONSOLE_I2C_ADDR>{i2C};
#else
    auto rem_registers = i2c_registers::Registers<R_DISPL_REG_SIZE>{i2C};
#endif
    void setRemoteI2CAddress() {
        pinMode(5, INPUT_PULLUP);
        pinMode(6, INPUT_PULLUP);
        pinMode(7, INPUT_PULLUP);
        if (!digitalRead(5)) {
            i2C.setAsMaster(DS_CONSOLE_I2C_ADDR);
            REQUESTING_INI = RC_DS_REQUESTING_INI;
        } else if (!digitalRead(6)) {
            i2C.setAsMaster(US_CONSOLE_I2C_ADDR);
            REQUESTING_INI = RC_US_REQUESTING_INI;
        } else if (!digitalRead(7)) {
            i2C.setAsMaster(FL_CONSOLE_I2C_ADDR);
            REQUESTING_INI = RC_F_REQUESTING_INI;
        } else Serial.println(F("Err: None of Pins 5-7 selected."));
    }

    void sendDataToProgrammer(int reg) {
        // Room temp and requests are sent by the console to the Programmer.
        // The programmer does not read them from the console.
        // New request temps initiated by the programmer are sent by the programmer and not read by the console.
        // Warmup-times are read by the console from the programmer.

#ifdef ZPSIM
        uint8_t(&debug)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(*rem_registers.reg_ptr(0));
        uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(Wire.i2CArr[US_CONSOLE_I2C_ADDR][0]);
#endif
        if (rem_registers.getRegister(R_DISPL_REG_OFFSET) != 0) {
            //Serial.print(F("Send to Offs ")); Serial.println(rem_registers.getRegister(R_DISPL_REG_OFFSET));
            i2C.write(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(R_DISPL_REG_OFFSET) + reg, 1, rem_registers.reg_ptr(reg));
        }
    }

    void readDataFromProgrammer() {
        if (rem_registers.getRegister(R_DISPL_REG_OFFSET) != 0) {
            i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(R_DISPL_REG_OFFSET) + R_WARM_UP_ROOM_M10, R_DISPL_REG_SIZE - R_WARM_UP_ROOM_M10, rem_registers.reg_ptr(R_WARM_UP_ROOM_M10));
        }
        bool towelRailNowOff = rem_registers.getRegister(R_ON_TIME_T_RAIL) == 0;
        if (towelRailNowOff) {
            if (rem_registers.getRegister(R_REQUESTING_T_RAIL) == e_On) rem_registers.setRegister(R_REQUESTING_T_RAIL, e_Auto);
        } else if (rem_registers.getRegister(R_REQUESTING_T_RAIL) == e_Off) sendDataToProgrammer(R_REQUESTING_T_RAIL);
    }

    void requestRegisterOffsetFromProgrammer() {
        uint8_t registerOffsetReqStillLodged = 0;
        i2C.read(PROGRAMMER_I2C_ADDR, R_SLAVE_REQUESTING_INITIALISATION, 1, &registerOffsetReqStillLodged);
        registerOffsetReqStillLodged |= REQUESTING_INI;
//#ifndef ZPSIM
//        while (i2C.write(PROGRAMMER_I2C_ADDR, R_SLAVE_REQUESTING_INITIALISATION, registerOffsetReqStillLodged) != _OK);
//        do {
//            Serial.flush();
//            Serial.println(F("wait for offset..."));
//            delay(100);
//            i2C.read(PROGRAMMER_I2C_ADDR, R_SLAVE_REQUESTING_INITIALISATION, 1, &registerOffsetReqStillLodged);
//        } while (registerOffsetReqStillLodged | REQUESTING_INI);
//#endif
        i2C.write(PROGRAMMER_I2C_ADDR, R_SLAVE_REQUESTING_INITIALISATION, registerOffsetReqStillLodged);
    }

    void begin() {
        i2C.begin();
        setRemoteI2CAddress();
        i2C.onReceive(rem_registers.receiveI2C);
        i2C.onRequest(rem_registers.requestI2C);
        rem_registers.setRegister(R_DISPL_REG_OFFSET, 0);
        requestRegisterOffsetFromProgrammer();
        tempSensor.initialise(rem_registers.getRegister(R_ROOM_TS_ADDR));
        tempSensor.setHighRes();
        readDataFromProgrammer();
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
            rem_registers.addToRegister(R_REQUESTED_ROOM_TEMP, increment);
            sendDataToProgrammer(R_REQUESTED_ROOM_TEMP);
            Serial.print(F("Rr ")); Serial.println(rem_registers.getRegister(R_REQUESTED_ROOM_TEMP));
            break;
        case TowelRail: 
        {
            auto mode = nextIndex(0, rem_registers.getRegister(R_REQUESTING_T_RAIL), e_ModeIsSet-1, increment);
            rem_registers.setRegister(R_REQUESTING_T_RAIL, mode);
            if (mode == e_On) {
                rem_registers.setRegister(R_ON_TIME_T_RAIL, 60);
                sendDataToProgrammer(R_ON_TIME_T_RAIL);
            }
            sendDataToProgrammer(R_REQUESTING_T_RAIL);
        }
            break;
        case HotWater: 
        {
            auto mode = nextIndex(0, rem_registers.getRegister(R_REQUESTING_DHW), e_ModeIsSet - 1, increment);
            rem_registers.setRegister(R_REQUESTING_DHW, mode);
            sendDataToProgrammer(R_REQUESTING_DHW);
        }
            break;
        }
        dataChanged = true;
        //readDataFromProgrammer();
    }

    void displayPage() {
        stopDisplaySleep();
        _display.clear();
        _display.setFont(getFont());
        _display.setCursor(0, 0);
        switch (_display_mode) {
        case RoomTemp:
            _display.print(F("Room Temp"));
            _display.setCursor(0, 1);
            _display.print(F("Ask "));
            _display.setFont(getFont(true));
            _display.print(rem_registers.getRegister(R_REQUESTED_ROOM_TEMP));
            _display.setFont(getFont());
            _display.print(F(" Is "));
            _display.print(rem_registers.getRegister(R_ROOM_TEMP));
            _display.setCursor(0, 2);
            _display.print(F("Warmup in "));
            _display.print(rem_registers.getRegister(R_WARM_UP_ROOM_M10) / 6.);
            _display.print(F("h"));
            break;
        case TowelRail:
            {
                _display.print(F("Towel Rail"));
                _display.setCursor(0, 1);
                _display.setFont(getFont(true));
                auto request = rem_registers.getRegister(R_REQUESTING_T_RAIL);
                _display.print(request == e_Auto ? F("Auto ") : (request == e_Off ? F("Manual Off ") : F("Manual On ")));
                _display.setFont(getFont());
                auto onTime = rem_registers.getRegister(R_ON_TIME_T_RAIL);           
                if (onTime) {
                    _display.setCursor(0, 2);
                    _display.print(F("Off in "));
                    _display.print(onTime);
                    _display.print(F("m"));
                } else if (request == e_Auto) _display.print(F("(Is Off)"));
            }
            break;
        case HotWater:
            _display.print(F("Hot Water"));
            _display.setCursor(0, 1);
            _display.setFont(getFont(true));
            {
                auto request = rem_registers.getRegister(R_REQUESTING_DHW);
                _display.print(request == e_Auto ? F("Auto ") : (request == e_Off ? F("Manual Off ") : F("Manual On ")));
            }
            _display.setFont(getFont());
            _display.setCursor(0, 2);
            _display.print(F("Warm in "));
            if (rem_registers.getRegister(R_WARM_UP_DHW_M10) <= 0) {
                _display.print(-rem_registers.getRegister(R_WARM_UP_DHW_M10));
                _display.print(F("m"));
            } else {
                _display.print(rem_registers.getRegister(R_WARM_UP_DHW_M10) / 6.);
                _display.print(F("h"));
            }
        }
        dataChanged = false;
    }

    void sleepPage() {
        _display.clear();
        _display.setFont(getFont());
        _display.setCursor(_sleepCol, _sleepRow);
        _display.print(rem_registers.getRegister(R_ROOM_TEMP));
    }

    void processKeys() {
#ifdef ZPSIM
        uint8_t(&debug)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(*rem_registers.reg_ptr(0));
        uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(Wire.i2CArr[US_CONSOLE_I2C_ADDR][0]);
#endif
        if (rem_registers.getRegister(R_DISPL_REG_OFFSET) == 0) {
            requestRegisterOffsetFromProgrammer();
            _display.setCursor(0, 0);
            _display.print(F("Wait for Master"));
        } else {
            _remoteKeypad.readKey();
            if (_remoteKeypad.oneSecondElapsed()) {
                readTempSensor();
                readDataFromProgrammer();
            }
            auto key = _remoteKeypad.popKey();
            if (key > I_Keypad::NO_KEY) Serial.println(key);
            if (key > I_Keypad::NO_KEY) {
                if (key == I_Keypad::KEY_WAKEUP) {
                    displayPage();
                } else {
                    if (key / 2 == I_Keypad::MODE_LEFT_RIGHT) {
                        changeMode(key);
                    } else {
                        changeValue(key);
                    }
                }
            }
            dataChanged = true;
            if (dataChanged) displayPage();
            else if (!_remoteKeypad.displayIsAwake() && _sleepRow == -1) {
                Serial.println("Sleep...");
                startDisplaySleep();
            }
        }
    }

    void readTempSensor() {
        auto fractional_temp = tempSensor.get_fractional_temp();
        rem_registers.setRegister(R_ROOM_TEMP, fractional_temp >> 8);
        uint8_t fraction = uint8_t(fractional_temp);
        //if (fraction != rem_registers.getRegister(R_ROOM_TEMP_FRACTION)) {
            rem_registers.setRegister(R_ROOM_TEMP_FRACTION, fraction);
            sendDataToProgrammer(R_ROOM_TEMP);
            sendDataToProgrammer(R_ROOM_TEMP_FRACTION);
        //}
    }

#ifdef ZPSIM
    void setKey(I_Keypad::KeyOperation key) {
        _remoteKeypad.putKey(key);
    }

    char* oledDisplay() {
        return _display.displayBuffer;
    }
#endif
}