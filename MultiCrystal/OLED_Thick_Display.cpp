#include "OLED_Thick_Display.h"
#include <MsTimer2.h>
#include <Watchdog_Timer.h>

#include <I2C_Talk.h>
//#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h> // for Speed-test
#include <I2C_Registers.h>

using namespace arduino_logger;

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
    , RC_FL_REQUESTING_INI = 16
    , ALL_REQUESTING = (RC_FL_REQUESTING_INI * 2) - 1
}; 
auto REQUESTING_INI = ALL_REQUESTING;

OLED_Thick_Display::OLED_Thick_Display(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& my_registers, int other_microcontroller_address, int localRegOffset, int remoteRegOffset)
    :I2C_To_MicroController(recover, my_registers, other_microcontroller_address, localRegOffset, remoteRegOffset)
    , _tempSensor{ recover }
    , _display(OLED_CS, OLED_DC, OLED_RESET)
    {}

void OLED_Thick_Display::begin() { // all registers start as zero.
    i2C().begin();
    setMyI2CAddress();
    set_watchdog_timeout_mS(8000);
    auto reg = registers();
    _tempSensor.initialise(reg.get(R_ROOM_TS_ADDR));
    reg.set(R_REMOTE_REG_OFFSET, NO_REG_OFFSET_SET);
    I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_ENABLE_KEYBOARD).setValue(5)/*.set(R_VALIDATE_READ)*/; // wake-time
    _remoteKeypad.set_console_mode(*reg.ptr(R_DEVICE_STATE));
    _display.begin();
    _display.clear();
    _display.setFont(getFont());
    _sleepRow = -1;
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
    } else if (!digitalRead(6)) {
        i2C().setAsMaster(US_CONSOLE_I2C_ADDR);
        REQUESTING_INI = RC_US_REQUESTING_INI;
        reg.set(R_ROOM_TS_ADDR, US_ROOM_TEMPSENS_ADDR);
    } else if (!digitalRead(7)) {
        i2C().setAsMaster(FL_CONSOLE_I2C_ADDR);
        REQUESTING_INI = RC_FL_REQUESTING_INI;
        reg.set(R_ROOM_TS_ADDR, FL_ROOM_TEMPSENS_ADDR);
    }
    // logger() << F("S 0x") << L_hex << i2C().address() << L_endl;
}

bool OLED_Thick_Display::doneI2C_Coms(bool newSecond) {
    // Only reads TS's and clears I2C-Comms flag on programmer.
    auto reg = registers();
    auto device_State = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
    if (device_State.is(F_I2C_NOW)) {
        _tempSensor.setHighRes();
        _tempSensor.readTemperature();
        auto fractional_temp = _tempSensor.get_fractional_temp();
        auto roomTempDeg = fractional_temp >> 8;
        auto roomTempFract = static_cast<uint8_t>(fractional_temp);
        if (reg.update(R_ROOM_TEMP, roomTempDeg) && _state == NO_CHANGE) {
            _state = REFRESH_DISPLAY;
        }
        if (abs(roomTempFract - reg.get(R_ROOM_TEMP_FRACTION)) >= 32 && _state == NO_CHANGE) {
            _state = REFRESH_DISPLAY;
        } 
        reg.set(R_ROOM_TEMP_FRACTION, roomTempFract);
        uint8_t clearState = DEVICE_IS_FINISHED;
        auto timeout = Timer_mS(300);
        do {
           if (writeRegValue(R_PROG_WAITING_FOR_REMOTE_I2C_COMS, clearState)) break;
            i2C().begin();
        } while (!timeout);
        device_State.clear(F_I2C_NOW); // clears local register.
        logger() << F("I2CNow State: ") << reg.get(R_DEVICE_STATE) << F(" at mS ") << millis() << L_endl;
        return true;
    }
    return false;
}

void OLED_Thick_Display::refreshRegisters() {
    auto reg = registers();
    auto deviceFlags = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
    auto progDataChanged = deviceFlags.is(F_PROGRAMMER_CHANGED_DATA);
    deviceFlags.clear(F_PROGRAMMER_CHANGED_DATA);
    auto reqTemp = reg.get(R_REQUESTING_ROOM_TEMP); // might have been set by console. When Prog has read it, it gets set to zero, so might not yet be zero.
    if (_state == WAIT_PROG_ACK) {
        logger() << millis() % 10000 << F(" WPA") << L_endl;
        if (reqTemp == 0 || reqTemp != _tempRequest) {
            logger() << millis() % 10000 << F(" PA") << L_endl;
            _state = REFRESH_DISPLAY;
        }
    } else if (progDataChanged) {
        _state = REFRESH_DISPLAY;
        logger() << millis() % 10000 << F(" PD ") << reqTemp << L_endl;
    }
    
    if (_state != WAIT_PROG_ACK && reqTemp /*&& reqTemp != _tempRequest*/) { // request sent by programmer
        logger() << millis() % 10000 << F(" PT ") << reqTemp << L_endl;
        _tempRequest = reqTemp;
        reg.set(R_REQUESTING_ROOM_TEMP, 0);
        _state = REFRESH_DISPLAY;
    } else if (_tempRequest == 0) {
        logger() << millis() % 10000 << F(" RT") << L_endl;
        reg.set(R_REMOTE_REG_OFFSET, NO_REG_OFFSET_SET);
        _state = WAIT_PROG_ACK;
        wakeDisplay();
    }

    bool towelRailNowOff = reg.get(R_ON_TIME_T_RAIL) == 0;
    if (towelRailNowOff) {
        if (reg.update(R_REQUESTING_T_RAIL, e_Auto) && _state == NO_CHANGE) _state = REFRESH_DISPLAY;
    }   
    bool dhwOK = reg.get(R_WARM_UP_DHW_M10) == 0;
    if (dhwOK) {
        if (reg.update(R_REQUESTING_DHW, e_Auto) && _state == NO_CHANGE) _state = REFRESH_DISPLAY;
    }
    //if (_state == REFRESH_DISPLAY) logger() << millis() % 10000 << F(" RD") << L_endl;
}

void OLED_Thick_Display::sleepPage() {
    _display.clear();
    _display.setCursor(_sleepCol, _sleepRow);
    auto reg = registers();
    float roomTemp = reg.get(R_ROOM_TEMP) + float(reg.get(R_ROOM_TEMP_FRACTION)) / 256;
    _display.print(roomTemp, 1);
}

void OLED_Thick_Display::startDisplaySleep() {
    _sleepRow = 0; _sleepCol = 0;
    _display.setFont(u8x8_font_7x14_1x2_n);
    //_display.setFont(getFont());
    sleepPage();
    MsTimer2::set(SLEEP_MOVE_PERIOD_mS, ::moveDisplay);
    MsTimer2::start();
}

void OLED_Thick_Display::wakeDisplay() {
    logger() << F("Wake") << L_endl;
    MsTimer2::stop();
    _sleepRow = -1;
}

void OLED_Thick_Display::moveDisplay() {
    ++_sleepCol;
    if (_sleepCol > SCREEN_WIDTH / FONT_WIDTH - 4) {
        _sleepCol = 0;
        ++_sleepRow;
        if (_sleepRow > 2) _sleepRow = 0;
    }
    sleepPage();
}

const uint8_t* OLED_Thick_Display::getFont(bool bold) {
    if (bold) return 	u8x8_font_pxplusibmcga_r; // bold
    else return 	u8x8_font_pxplusibmcgathin_r; // v nice
}

void OLED_Thick_Display::changeMode(int keyCode) {
    auto newMode = NoOfDisplayModes + _display_mode + (keyCode == I_Keypad::KEY_LEFT ? -1 : 1);
    _display_mode = Display_Mode(newMode % NoOfDisplayModes);
    if (_state == NO_CHANGE) _state = REFRESH_DISPLAY;
}

void OLED_Thick_Display::changeValue(int keyCode) {
    auto increment = keyCode == I_Keypad::KEY_UP ? 1 : -1;
    auto reg = registers();
    //if (_state != NO_CHANGE) return;
    auto deviceFlags = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
    switch (_display_mode) {
    case RoomTemp:
        if (deviceFlags.is(F_ENABLE_KEYBOARD)) {
            _tempRequest += increment;
            reg.set(R_REQUESTING_ROOM_TEMP, _tempRequest);
            _state = NEW_T_REQUEST;
            logger() << millis() % 10000 << F(" CT ") << _tempRequest << L_endl;
        } 
        break;
    case TowelRail: 
        {
            auto mode = nextIndex(0, reg.get(R_REQUESTING_T_RAIL), e_On, increment);
            reg.set(R_REQUESTING_T_RAIL, mode);
            if (mode == e_On) reg.set(R_ON_TIME_T_RAIL, 1); // to stop it resetting itself
            _state = REFRESH_DISPLAY;
        }
        break;
    case HotWater: 
        {
            auto mode = nextIndex(0, reg.get(R_REQUESTING_DHW), e_On, increment);
            reg.set(R_REQUESTING_DHW, mode);
            if (mode == e_On) reg.set(R_WARM_UP_DHW_M10, -1); // to stop it resetting itself
            _state = REFRESH_DISPLAY;
        }
        break;
    }
}

void OLED_Thick_Display::displayPage() {
    if (_sleepRow != -1) return;
    _display.clear();
    _display.setFont(getFont());
    _display.setCursor(0, 0);
    auto reg = registers();
    switch (_display_mode) {
    case RoomTemp:
        if (_state == NEW_T_REQUEST) _state = WAIT_PROG_ACK;
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
        if (!I2C_Flags_Obj(reg.get(R_DEVICE_STATE)).is(F_ENABLE_KEYBOARD)) {
            _display.setCursor(0, 3);
            _display.print(F("(Keys Disabled)"));
        }
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
    if (_state == REFRESH_DISPLAY) _state = NO_CHANGE;
}

void OLED_Thick_Display::processKeys() { // called by loop()
#ifdef ZPSIM
    uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(TwoWire::i2CArr[US_CONSOLE_I2C_ADDR]);
#endif
    auto newSecond = _remoteKeypad.oneSecondElapsed();
    if (registers().get(R_REMOTE_REG_OFFSET) == NO_REG_OFFSET_SET) {
        if (newSecond) {
            logger() << F("WFM") << L_endl;
            _display.setCursor(0, 0);
            _display.print(F("Wait for Master"));
            _remoteKeypad.popKey();
        }
    } else {
        if (newSecond) {
            refreshRegisters();
        }
        _remoteKeypad.readKey();
        if (doneI2C_Coms(newSecond)) reset_watchdog();

        auto key = _remoteKeypad.popKey();
        if (key > I_Keypad::NO_KEY) {
            logger() << F("Key:") << key << L_endl;
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
        if (_state == NO_CHANGE) {
            if (!_remoteKeypad.displayIsAwake() && _sleepRow == -1) {
                startDisplaySleep();
            }
        } else if (_state != WAIT_PROG_ACK) displayPage();
    }
}

#ifdef ZPSIM
    void OLED_Thick_Display::setKey(I_Keypad::KeyOperation key) {
        _remoteKeypad.putKey(key);
    }

    char* OLED_Thick_Display::oledDisplay() {
        return _display.displayBuffer;
    }

    void OLED_Thick_Display::setTSaddr(int addr) {
        _tempSensor.setAddress(addr);
    }

#endif
