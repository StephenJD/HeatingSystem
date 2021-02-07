#pragma once
/*

 ******** NOTE select Arduino Pro Mini 328P 3.3v 8MHz **********
 Arduino Pro Mini must be upgraded with  boot-loader to enable watchdog timer.
 File->Preferences->Additional Boards Manager URLs: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
 Tools->Board->Boards Manager->Install MiniCore
 1. In Arduino: Load File-> Examples->ArduinoISP->ArduinoISP
 2. Upload this to any board, including another Mini Pro.
 3. THEN connect SPI pins from the ISP board to the Mini Pro: MOSI(11/51)->11, MISO(12/50)->12, SCK(13/52)->13, 10->Reset on MiniPro, Vcc and 0c pins.
 4. Tools->Board->MiniCore Atmega 328
 5. External 8Mhz clock, BOD 1.7v, LTO enabled, Variant 328P, UART0.
 6. Tools->Programmer->Arduino as ISP
 7. Burn Bootloader
 8. Then upload your desired sketch to ordinary ProMini board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

 Arduino Mega's must be upgraded with boot-loader to enable watchdog timer.
 The MegaCore package doesn't work well. Use https://github.com/nickgammon/arduino_sketches
 1. Download the zipfile and unpack into your sketch folder.
 2. Upload the Atmega_Board_Programmer sketch to your programmer board (e.g. Arduino Pro Mini).
 3. THEN connect SPI pins to the Mega: MOSI(11)->51, MISO(12)->50, SCK(13)->52, 10->Reset on Mega, Vcc and 0c pins.
 4. Run the sketch via the Serial Monitor (Just open the monitor at BAUD_RATE = 115200)
 5. When you see the prompts, type "G" to upload the bootloader.
 6. Edit boards.txt by appending:
 (Note the active version of this file is probably in C:\Users\<Name>\AppData\Local\arduino15\packages\arduino\hardware\avr\1.8.2\boards.txt)
 ##############################################################

megao.name=Arduino Mega1280 Optiboot
megao.upload.protocol=arduino
megao.upload.maximum_size=130048
megao.upload.speed=115200
megao.bootloader.low_fuses=0xff
megao.bootloader.high_fuses=0xdc
megao.bootloader.extended_fuses=0xf5
megao.bootloader.path=optiboot
#megao.bootloader.file=optiboot_atmega1280.hex
megao.bootloader.unlock_bits=0x3F
megao.bootloader.lock_bits=0x0F
megao.build.mcu=atmega1280
megao.build.f_cpu=16000000L
megao.build.core=arduino
megao.build.variant=mega
megao.build.variant=mega
megao.build.board=AVR_MEGA1280
megao.upload.tool=avrdude

7. When wanting to upload sketch to the Mega, you must now select Tools->Board->Arduino Mega1280 Optiboot

*/

// functions must not be inlined.

// ********** NOTE: ***************
// If watchdog is being used, set_watchdog_timeout_mS() must be set at the begining of startup, before any calls to delay()
// Otherwise, depending on how cleanly the Arduino sht down, it may time-out before it is reset.
// ********************************

void watchdogSetup();

void reset_watchdog();

/// <summary>
/// Max priod is 16000mS
/// </summary>
void set_watchdog_timeout_mS(int period_mS);

void disable_watchdog();

#if defined(__SAM3X8E__)

#else
	// Function Pototype must be in header
	//void wdt_init(void) __attribute__((naked, used, section(".init3")));
#endif