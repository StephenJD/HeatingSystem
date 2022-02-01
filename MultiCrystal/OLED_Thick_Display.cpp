#include "OLED_Thick_Display.h"
#include <MsTimer2.h>

#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h>
#include <I2C_Registers.h>
#include <Conversions.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

// Declaration for SSD1306 display connected using software SPI (default case):
constexpr uint8_t OLED_MOSI = 11;  // DIn
constexpr uint8_t OLED_CLK = 13;
constexpr uint8_t OLED_DC = 9;
constexpr uint8_t OLED_CS = 8;
constexpr uint8_t OLED_RESET = 10;

constexpr auto SLEEP_MOVE_PERIOD_mS = 60000UL;
constexpr int SCREEN_WIDTH = 128;  // OLED display width, in pixels
constexpr int SCREEN_HEIGHT = 32; // OLED display height, in pixels
constexpr int FONT_WIDTH = 8;

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace GP_LIB;

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

OLED_Thick_Display::OLED_Thick_Display(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& my_registers, int address, int regOffset, unsigned long* timeOfReset_mS)
    :I2C_To_MicroController(recover, my_registers, address, regOffset, timeOfReset_mS)
    , _tempSensor{ recover }
    , _display(OLED_CS, OLED_DC, OLED_RESET)
    {}

void OLED_Thick_Display::begin() { // all registers start as zero.
    i2C().begin();
    setMyI2CAddress();
    auto speedTest = I2C_SpeedTest{ *this };
    speedTest.fastest();
    auto reg = registers();
    _tempSensor.initialise(reg.get(R_ROOM_TS_ADDR));
    //auto lock = getLock();
    reg.set(R_DISPL_REG_OFFSET, NO_REG_OFFSET_SET);
    auto mode = ModeFlags(reg.get(R_MODE));
    mode.set(e_ENABLE_KEYBOARD).setValue(5);
    reg.set(R_MODE, uint8_t(mode));
    _remoteKeypad.set_console_mode(*reg.ptr(R_MODE));
    _display.begin();
    clearDisplay();
}
    
void OLED_Thick_Display::setMyI2CAddress() {
    pinMode(5, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);
    auto reg = registers();
    if (!digitalRead(5)) {
        i2C().setAsMaster(DS_CONSOLE_I2C_ADDR);
        REQUESTING_INI = RC_DS_REQUESTING_INI;
        reg.set(R_ROOM_TS_ADDR, DS_ROOM_TEMPSENS_ADDR);
        Serial.print(F("DS_CONSOLE 0x"));
    } else if (!digitalRead(6)) {
        i2C().setAsMaster(US_CONSOLE_I2C_ADDR);
        REQUESTING_INI = RC_US_REQUESTING_INI;
        reg.set(R_ROOM_TS_ADDR, US_ROOM_TEMPSENS_ADDR);
        Serial.print(F("US_CONSOLE 0x"));
    } else if (!digitalRead(7)) {
        i2C().setAsMaster(FL_CONSOLE_I2C_ADDR);
        REQUESTING_INI = RC_F_REQUESTING_INI;
        reg.set(R_ROOM_TS_ADDR, FL_ROOM_TEMPSENS_ADDR);
        Serial.print(F("FL_CONSOLE 0x"));
    } else Serial.println(F("Err: None of Pins 5-7 selected."));
    Serial.println(i2C().address(), HEX);
}

void OLED_Thick_Display::sendDataToProgrammer(int regNo) {
    // New request temps initiated by the programmer are sent by the programmer.
    // In Multi-Master Mode: 
    //		Room temp and requests are sent by the console to the Programmer.
    //		Warmup-times are read by the console from the programmer.
    // In Slave-Mode: 
    //		Requests are read from the console by the Programmer.
    //		Room Temp and Warmup-times are sent to the console by the programmer.

#ifdef ZPSIM
    uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(TwoWire::i2CArr[US_CONSOLE_I2C_ADDR][0]);
#endif
    auto reg = registers();
    if (ModeFlags(reg.get(R_MODE)).is(e_MASTER) && reg.get(R_DISPL_REG_OFFSET) != NO_REG_OFFSET_SET) {
        //auto lock = getLock();
        //Serial.print(F("Send to Offs ")); Serial.println(reg.get(R_DISPL_REG_OFFSET));
        write(reg.get(R_DISPL_REG_OFFSET) + regNo, 1, reg.ptr(regNo));
    }
}

void OLED_Thick_Display::refreshRegisters() {
    //auto lock = getLock();
    auto reg = registers();
    if (ModeFlags(reg.get(R_MODE)).is(e_MASTER) && reg.get(R_DISPL_REG_OFFSET) != NO_REG_OFFSET_SET) {
        _tempSensor.readTemperature();
        auto fractional_temp = _tempSensor.get_fractional_temp();
        auto roomTempDeg = fractional_temp >> 8;
        auto roomTempFract = static_cast<uint8_t>(fractional_temp);
        if (reg.update(R_ROOM_TEMP, roomTempDeg)) {
            sendDataToProgrammer(R_ROOM_TEMP);
            _dataChanged = true;
        }        
        if (reg.update(R_ROOM_TEMP_FRACTION, roomTempFract)) {
            sendDataToProgrammer(R_ROOM_TEMP_FRACTION);
            _dataChanged = true;
        }
        auto startReg = reg.get(R_DISPL_REG_OFFSET) + R_WARM_UP_ROOM_M10;
        for (int i = 0; i < R_DISPL_REG_SIZE - R_WARM_UP_ROOM_M10; ++i) {
            uint8_t regVal;
            read(startReg + i, 1 , &regVal);
            _dataChanged |= reg.update(R_WARM_UP_ROOM_M10 + i, regVal);
        }
    }
    else _dataChanged |= ModeFlags(reg.get(R_MODE)).is(e_DATA_CHANGED);
    ModeFlags(*reg.ptr(R_MODE)).clear(e_DATA_CHANGED);
    auto reqTemp = reg.get(R_REQUESTING_ROOM_TEMP);
    if (reqTemp && _tempRequest != reqTemp) { // new request sent by programmer
        _tempRequest = reqTemp;
        reg.set(R_REQUESTING_ROOM_TEMP, 0);
        _dataChanged |= true;
    }
    bool towelRailNowOff = reg.get(R_ON_TIME_T_RAIL) == 0;
    if (towelRailNowOff) {
        _dataChanged |= reg.update(R_REQUESTING_T_RAIL, e_Auto);
    }   
    bool dhwOK = reg.get(R_WARM_UP_DHW_M10) == 0;
    if (dhwOK) {
        _dataChanged |= reg.update(R_REQUESTING_DHW, e_Auto);
    }
}

uint8_t OLED_Thick_Display::requestRegisterOffsetFromProgrammer() {
    //auto lock = getLock();
    auto status = _OK;
    auto reg = registers();
    if (ModeFlags(reg.get(R_MODE)).is(e_MASTER)) {
        uint8_t requestsRegisteredWithMaster = 0;
        status = read(R_SLAVE_REQUESTING_INITIALISATION, 1, &requestsRegisteredWithMaster);
        Serial.print(F("Ini Requests registered 0x")); Serial.println(requestsRegisteredWithMaster, BIN);
        requestsRegisteredWithMaster |= REQUESTING_INI;

        if (status != _OK) status = _I2C_ReadDataWrong;
        else {
            Serial.print(F("Ini Requests sent 0x")); Serial.println(requestsRegisteredWithMaster, BIN);
            status = write(R_SLAVE_REQUESTING_INITIALISATION, requestsRegisteredWithMaster);
            if (status != _OK) status = _NACK_during_data_send;
        }
        _tempSensor.setHighRes();
        delay(1000);
    }
    return status;
}

void OLED_Thick_Display::startDisplaySleep() {
    _sleepRow = 0; _sleepCol = 0;
    MsTimer2::set(SLEEP_MOVE_PERIOD_mS, ::moveDisplay);
    MsTimer2::start();
}

void OLED_Thick_Display::wakeDisplay() {
    MsTimer2::stop();
    _sleepRow = -1;
    Serial.println("Wake...");
}

void OLED_Thick_Display::moveDisplay() {
    ++_sleepCol;
    if (_sleepCol > SCREEN_WIDTH / FONT_WIDTH - 4) {
        _sleepCol = 0;
        ++_sleepRow;
        if (_sleepRow > 3) _sleepRow = 0;
    }
    sleepPage();
}

const uint8_t* OLED_Thick_Display::getFont(bool bold) {
    if (bold) return 	u8x8_font_pxplusibmcga_f; // 5,444 bold
    else return 	u8x8_font_pxplusibmcgathin_f; // 5,444 v nice
}

void OLED_Thick_Display::changeMode(int keyCode) {
    auto newMode = NoOfDisplayModes + _display_mode + (keyCode == I_Keypad::KEY_LEFT ? -1 : 1);
    _display_mode = Display_Mode(newMode % NoOfDisplayModes);
    _dataChanged = true;
}

void OLED_Thick_Display::changeValue(int keyCode) {
    auto increment = keyCode == I_Keypad::KEY_UP ? 1 : -1;
    auto reg = registers();
    switch (_display_mode) {
    case RoomTemp:
        if (ModeFlags(reg.get(R_MODE)).is(e_ENABLE_KEYBOARD)) {
            _tempRequest += increment;
            reg.set(R_REQUESTING_ROOM_TEMP, _tempRequest);
            sendDataToProgrammer(R_REQUESTING_ROOM_TEMP);
            Serial.print(F("Rr ")); Serial.println(_tempRequest);
        }
        break;
    case TowelRail: 
    {
        auto mode = nextIndex(0, reg.get(R_REQUESTING_T_RAIL), e_On, increment);
        reg.set(R_REQUESTING_T_RAIL, mode);
        if (mode == e_On) reg.set(R_ON_TIME_T_RAIL, 1); // to stop it resetting itself
        sendDataToProgrammer(R_REQUESTING_T_RAIL);
    }
        break;
    case HotWater: 
    {
        auto mode = nextIndex(0, reg.get(R_REQUESTING_DHW), e_On, increment);
        reg.set(R_REQUESTING_DHW, mode);
        if (mode == e_On) reg.set(R_WARM_UP_DHW_M10, -1); // to stop it resetting itself
        sendDataToProgrammer(R_REQUESTING_DHW);
    }
        break;
    }
    _dataChanged = true;
}

void OLED_Thick_Display::displayPage() {
    if (_sleepRow != -1) return;
    _display.clear();
    _display.setFont(getFont());
    _display.setCursor(0, 0);
    auto reg = registers();
    switch (_display_mode) {
    case RoomTemp:
        _display.print(F("Room Temp"));
        _display.setCursor(0, 1);
        _display.print(F("Ask "));
        _display.setFont(getFont(true));
        _display.print(_tempRequest);
        _display.setFont(getFont());
        _display.print(F(" Is "));
        {
            float roomTemp = reg.get(R_ROOM_TEMP) + float(reg.get(R_ROOM_TEMP_FRACTION)) / 256;
            _display.print(roomTemp,1);
        }
        _display.setCursor(0, 2);
        _display.print(F("Warmup in "));
        _display.print(reg.get(R_WARM_UP_ROOM_M10) / 6.,1);
        _display.print(F("h"));
        break;
    case TowelRail:
        {
            _display.print(F("Towel Rail"));
            _display.setCursor(0, 1);
            _display.setFont(getFont(true));
            auto request = reg.get(R_REQUESTING_T_RAIL);
            _display.print(request == e_Auto ? F("Auto ") : (request == e_Off ? F("Manual Off ") : F("Manual On ")));
            _display.setFont(getFont());
            auto onTime = reg.get(R_ON_TIME_T_RAIL);           
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
            auto request = reg.get(R_REQUESTING_DHW);
            _display.print(request == e_Auto ? F("Auto ") : (request == e_Off ? F("Manual Off ") : F("Manual On ")));
        }
        _display.setFont(getFont());
        _display.setCursor(0, 2);
        _display.print(F("Warm in "));
        int8_t dhw_warmupTime = reg.get(R_WARM_UP_DHW_M10);
        if (dhw_warmupTime  < 0 && dhw_warmupTime > -60) {
            _display.print(-dhw_warmupTime);
            _display.print(F("m"));
        } else {
            _display.print((uint8_t)dhw_warmupTime / 6.,1);
            _display.print(F("h"));
        }
    }
    _dataChanged = false;
}


void OLED_Thick_Display::clearDisplay() {
    _display.clear();
    _display.setFont(getFont());
    _display.setCursor(_sleepCol, _sleepRow);
}
void OLED_Thick_Display::sleepPage() {
    clearDisplay();
    auto reg = registers();
    float roomTemp = reg.get(R_ROOM_TEMP) + float(reg.get(R_ROOM_TEMP_FRACTION)) / 256;
    _display.print(roomTemp, 1);
}

void OLED_Thick_Display::processKeys() { // called by loop()
#ifdef ZPSIM
    uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(TwoWire::i2CArr[US_CONSOLE_I2C_ADDR][0]);
#endif
    if (registers().get(R_DISPL_REG_OFFSET) == NO_REG_OFFSET_SET) {
        auto status = requestRegisterOffsetFromProgrammer();
        _display.setCursor(0, 0);
        _display.print(F("Wait for Master"));
        if (status == _I2C_ReadDataWrong) {
            _display.setCursor(0, 1);
            _display.print(F("...unresponsive"));
        } else if (status == _NACK_during_data_send) {
            _display.setCursor(0, 1);
            _display.print(F("Sending request"));
        }
        _dataChanged = true;
        _remoteKeypad.popKey();
    } else {
        _remoteKeypad.readKey();
        if (_remoteKeypad.oneSecondElapsed()) {
            refreshRegisters();
        }
        auto key = _remoteKeypad.popKey();
        if (key > I_Keypad::NO_KEY) Serial.println(key);
        if (key > I_Keypad::NO_KEY) {
            if (key == I_Keypad::KEY_WAKEUP) {
                wakeDisplay();
                displayPage();
            } else {
                if (key / 2 == I_Keypad::MODE_LEFT_RIGHT) {
                    changeMode(key);
                } else {
                    changeValue(key);
                }
            }
        }
        if (_dataChanged) displayPage();
        else if (!_remoteKeypad.displayIsAwake() && _sleepRow == -1) {
            Serial.println("Sleep...");
            startDisplaySleep();
        }
    }
}

#ifdef ZPSIM
    void OLED_Thick_Display::setKey(I_Keypad::KeyOperation key) {
        _remoteKeypad.putKey(key);
    }

    char* OLED_Thick_Display::oledDisplay() {
        return _display.displayBuffer;
    }
#endif
