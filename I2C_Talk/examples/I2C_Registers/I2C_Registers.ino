#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>

#define SERIAL_RATE 115200

constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;
constexpr uint8_t DS_REMOTE_I2C_ADDR = 0x12;
constexpr uint8_t US_REMOTE_I2C_ADDR = 0x13;
constexpr uint8_t FL_REMOTE_I2C_ADDR = 0x14;

I2C_Talk i2C;
//I2C_Talk i2C2(DS_REMOTE_I2C_ADDR, Wire1);

	enum RemoteRegisterName {
      remote_register_offset
		, roomTempSensorAddr
		, roomTempRequest
		, roomTemp
		, roomTemp_fraction
		, roomWarmupTime_m10
		, towelRailRequest
		, towelRailOnTime_m
		, hotWaterRequest
		, hotWaterTemp
		, hotWaterWarmupTime_m10 // If -ve, in 0-60 mins, if +ve in min_10
		, remoteRegister_size
	};

  enum MixValveRegisters {
		// All registers are single-byte.
    /*Read/Write Data*/		slave_requesting_resend_data,
		/*Read Only Data*/		status,
		/*Read Only Data*/		mode, count, valve_pos_MSB, valve_pos_LSB, state, // I2C devices use big-endianness: MSB at the smallest address: So a uint16_t is [MSB, LSB].
		/*Read/Write Data*/		flow_temp, request_temp, ratio, moveFromTemp, moveFromPos,
		/*Read/Write Config*/	temp_i2c_addr, traverse_time, wait_time, max_flow_temp, eeprom_OK1, eeprom_OK2,
		/*End-Stop*/			    mixValveRegister_size
	};

  enum rtc_registers {
    dd
    , mm
    , yy
    , rtcRegister_size
  };

  auto all_registers = i2c_registers::Registers<i2C, mixValveRegister_size + 3 * remoteRegister_size>{};
  auto rem_registers = i2c_registers::Registers<i2C, remoteRegister_size>{};
  //auto rtc_registers = i2c_registers::Registers<i2C2, rtcRegister_size>{};

void setMyI2CAddress() {
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  if (!digitalRead(5)) {
    i2C.setAsMaster(DS_REMOTE_I2C_ADDR);
    Serial.println(F("DS"));
  }
  else if (!digitalRead(6)) {
    i2C.setAsMaster(US_REMOTE_I2C_ADDR);  
    Serial.println(F("US"));
  } 
  else if (!digitalRead(7)) {
    i2C.setAsMaster(FL_REMOTE_I2C_ADDR);
    Serial.println(F("FL"));
  }
  else {
    i2C.setAsMaster(PROGRAMMER_I2C_ADDR);
    Serial.println(F("PR"));
  }
}

void createMyRegisters() {
  if (i2C.address() == PROGRAMMER_I2C_ADDR) {
    Serial.println(F("Create PR"));
 
    all_registers.setRegister(mixValveRegister_size + roomTempRequest,17);
    all_registers.setRegister(mixValveRegister_size + roomWarmupTime_m10,12);
    all_registers.setRegister(mixValveRegister_size + towelRailOnTime_m,55);
    all_registers.setRegister(mixValveRegister_size + hotWaterTemp,47);
    all_registers.setRegister(mixValveRegister_size + hotWaterWarmupTime_m10,-35);
    
    all_registers.setRegister(mixValveRegister_size + remoteRegister_size + roomTempRequest,18);
    all_registers.setRegister(mixValveRegister_size + remoteRegister_size + roomWarmupTime_m10,13);
    all_registers.setRegister(mixValveRegister_size + remoteRegister_size + towelRailOnTime_m,56);
    all_registers.setRegister(mixValveRegister_size + remoteRegister_size + hotWaterTemp,48);
    all_registers.setRegister(mixValveRegister_size + remoteRegister_size + hotWaterWarmupTime_m10,-36);

    all_registers.setRegister(mixValveRegister_size + 2*remoteRegister_size + roomTempRequest,19);
    all_registers.setRegister(mixValveRegister_size + 2*remoteRegister_size + roomWarmupTime_m10,14);
    all_registers.setRegister(mixValveRegister_size + 2*remoteRegister_size + towelRailOnTime_m,57);
    all_registers.setRegister(mixValveRegister_size + 2*remoteRegister_size + hotWaterTemp,49);
    all_registers.setRegister(mixValveRegister_size + 2*remoteRegister_size + hotWaterWarmupTime_m10,-37);
  } else {
    Serial.println(F("Create Rem"));
    rem_registers.setRegister(roomTemp,20);
    rem_registers.setRegister(towelRailRequest,0);
    rem_registers.setRegister(hotWaterRequest,1);
  }
}

void sendI2CregisterOffsets() {
  Serial.println(F("Sending Offs to Rems"));
  i2C.write(DS_REMOTE_I2C_ADDR, remote_register_offset, mixValveRegister_size);
  i2C.write(US_REMOTE_I2C_ADDR, remote_register_offset, mixValveRegister_size + remoteRegister_size);
  i2C.write(FL_REMOTE_I2C_ADDR, remote_register_offset, mixValveRegister_size + 2 * remoteRegister_size);
  all_registers.setRegister(slave_requesting_resend_data,0);
}

void sendDataToRemotes() {
  i2C.write(DS_REMOTE_I2C_ADDR, roomTempRequest, all_registers.getRegister(mixValveRegister_size + roomTempRequest));
  i2C.write(US_REMOTE_I2C_ADDR, roomTempRequest, all_registers.getRegister(mixValveRegister_size + remoteRegister_size + roomTempRequest));
  i2C.write(FL_REMOTE_I2C_ADDR, roomTempRequest, all_registers.getRegister(mixValveRegister_size + 2 * remoteRegister_size + roomTempRequest));

  i2C.write(DS_REMOTE_I2C_ADDR, roomWarmupTime_m10, all_registers.getRegister(mixValveRegister_size + roomWarmupTime_m10));
  i2C.write(US_REMOTE_I2C_ADDR, roomWarmupTime_m10, all_registers.getRegister(mixValveRegister_size + remoteRegister_size + roomWarmupTime_m10));
  i2C.write(FL_REMOTE_I2C_ADDR, roomWarmupTime_m10, all_registers.getRegister(mixValveRegister_size + 2 * remoteRegister_size + roomWarmupTime_m10));

  i2C.write(DS_REMOTE_I2C_ADDR, towelRailOnTime_m, all_registers.getRegister(mixValveRegister_size + towelRailOnTime_m));
  i2C.write(US_REMOTE_I2C_ADDR, towelRailOnTime_m, all_registers.getRegister(mixValveRegister_size + remoteRegister_size + towelRailOnTime_m));
  i2C.write(FL_REMOTE_I2C_ADDR, towelRailOnTime_m, all_registers.getRegister(mixValveRegister_size + 2 * remoteRegister_size + towelRailOnTime_m));
  
  i2C.write(DS_REMOTE_I2C_ADDR, hotWaterTemp, all_registers.getRegister(mixValveRegister_size + hotWaterTemp));
  i2C.write(US_REMOTE_I2C_ADDR, hotWaterTemp, all_registers.getRegister(mixValveRegister_size + remoteRegister_size + hotWaterTemp));
  i2C.write(FL_REMOTE_I2C_ADDR, hotWaterTemp, all_registers.getRegister(mixValveRegister_size + 2 * remoteRegister_size + hotWaterTemp));

  i2C.write(DS_REMOTE_I2C_ADDR, hotWaterWarmupTime_m10, all_registers.getRegister(mixValveRegister_size + hotWaterWarmupTime_m10));
  i2C.write(US_REMOTE_I2C_ADDR, hotWaterWarmupTime_m10, all_registers.getRegister(mixValveRegister_size + remoteRegister_size + hotWaterWarmupTime_m10));
  i2C.write(FL_REMOTE_I2C_ADDR, hotWaterWarmupTime_m10, all_registers.getRegister(mixValveRegister_size + 2 * remoteRegister_size + hotWaterWarmupTime_m10));
}

void sendDataToProgrammer() {
  Serial.print(F("Send to Offs ")); Serial.println(rem_registers.getRegister(remote_register_offset));
  i2C.write(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + roomTemp, rem_registers.getRegister(roomTemp));
  i2C.write(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + towelRailRequest, rem_registers.getRegister(towelRailRequest));
  i2C.write(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + hotWaterRequest, rem_registers.getRegister(hotWaterRequest));
}

void requestDataFromRemotes() {
  i2C.read(DS_REMOTE_I2C_ADDR, roomTemp, 1, all_registers.reg_ptr(mixValveRegister_size + roomTemp));
  i2C.read(US_REMOTE_I2C_ADDR, roomTemp, 1, all_registers.reg_ptr(mixValveRegister_size + remoteRegister_size + roomTemp));
  i2C.read(FL_REMOTE_I2C_ADDR, roomTemp, 1, all_registers.reg_ptr(mixValveRegister_size + 2 * remoteRegister_size + roomTemp));

  i2C.read(DS_REMOTE_I2C_ADDR, towelRailRequest, 1, all_registers.reg_ptr(mixValveRegister_size + towelRailRequest));
  i2C.read(US_REMOTE_I2C_ADDR, towelRailRequest, 1, all_registers.reg_ptr(mixValveRegister_size + remoteRegister_size + towelRailRequest));
  i2C.read(FL_REMOTE_I2C_ADDR, towelRailRequest, 1, all_registers.reg_ptr(mixValveRegister_size + 2 * remoteRegister_size + towelRailRequest));

  i2C.read(DS_REMOTE_I2C_ADDR, hotWaterRequest, 1, all_registers.reg_ptr(mixValveRegister_size + hotWaterRequest));
  i2C.read(US_REMOTE_I2C_ADDR, hotWaterRequest, 1, all_registers.reg_ptr(mixValveRegister_size + remoteRegister_size + hotWaterRequest));
  i2C.read(FL_REMOTE_I2C_ADDR, hotWaterRequest, 1, all_registers.reg_ptr(mixValveRegister_size + 2 * remoteRegister_size + hotWaterRequest));
}

void requestDataFromProgrammer() {
  i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + roomTempRequest, 1, rem_registers.reg_ptr(roomTempRequest));
  i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + roomWarmupTime_m10, 1, rem_registers.reg_ptr(roomWarmupTime_m10));
  i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + towelRailOnTime_m, 1, rem_registers.reg_ptr(towelRailOnTime_m));
  i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + hotWaterTemp, 1, rem_registers.reg_ptr(hotWaterTemp));
  i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(remote_register_offset) + hotWaterWarmupTime_m10, 1, rem_registers.reg_ptr(hotWaterWarmupTime_m10));
}

void printRegisters() {
  if (i2C.address() == PROGRAMMER_I2C_ADDR) {
    Serial.print(F("Of Req ")); Serial.println(all_registers.getRegister(slave_requesting_resend_data));

    Serial.print(F("Of 1 ")); Serial.println(mixValveRegister_size);
    Serial.print(F("Of 2 ")); Serial.println(mixValveRegister_size + remoteRegister_size);
    Serial.print(F("Of 3 ")); Serial.println(mixValveRegister_size + 2*remoteRegister_size);

    Serial.print(F("Rr 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + roomTempRequest));
    Serial.print(F("Rr 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + roomTempRequest));
    Serial.print(F("Rr 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + roomTempRequest));

    Serial.print(F("Rt 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + roomTemp));
    Serial.print(F("Rt 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + roomTemp));
    Serial.print(F("Rt 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + roomTemp));

    Serial.print(F("Rm 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + roomWarmupTime_m10));
    Serial.print(F("Rm 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + roomWarmupTime_m10));
    Serial.print(F("Rm 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + roomWarmupTime_m10));

    Serial.print(F("TRr 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + towelRailRequest));
    Serial.print(F("TRr 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + towelRailRequest));
    Serial.print(F("TRr 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + towelRailRequest));

    Serial.print(F("TRm 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + towelRailOnTime_m));
    Serial.print(F("TRm 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + towelRailOnTime_m));
    Serial.print(F("TRm 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + towelRailOnTime_m));

    Serial.print(F("HWr 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + hotWaterRequest));
    Serial.print(F("HWr 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + hotWaterRequest));
    Serial.print(F("HWr 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + hotWaterRequest));

    Serial.print(F("HWt 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + hotWaterTemp));
    Serial.print(F("HWt 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + hotWaterTemp));
    Serial.print(F("HWt 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + hotWaterTemp));

    Serial.print(F("HWm 1 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + hotWaterWarmupTime_m10));
    Serial.print(F("HWm 2 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + remoteRegister_size + hotWaterWarmupTime_m10));
    Serial.print(F("HWm 3 ")); Serial.println(all_registers.getRegister(mixValveRegister_size + 2*remoteRegister_size + hotWaterWarmupTime_m10));
  } else {
    Serial.print(F("Of ")); Serial.println(rem_registers.getRegister(remote_register_offset));
    Serial.print(F("Rr ")); Serial.println(rem_registers.getRegister(roomTempRequest));
    Serial.print(F("Rt ")); Serial.println(rem_registers.getRegister(roomTemp));
    Serial.print(F("Rm ")); Serial.println(rem_registers.getRegister(roomWarmupTime_m10));
    Serial.print(F("TRr ")); Serial.println(rem_registers.getRegister(towelRailRequest)); 
    Serial.print(F("TRm ")); Serial.println(rem_registers.getRegister(towelRailOnTime_m));
    Serial.print(F("HWr ")); Serial.println(rem_registers.getRegister(hotWaterRequest));
    Serial.print(F("HWt ")); Serial.println(rem_registers.getRegister(hotWaterTemp));
    Serial.print(F("HWm ")); Serial.println(rem_registers.getRegister(hotWaterWarmupTime_m10));
  }
  Serial.println();
}

void setup() {
  //flashLED(500);
  Serial.begin(SERIAL_RATE);
  Serial.flush();
  // Data is accessed with a register adress, then the number of bytes, usually just one byte.
  // We have four register sets starting at different addresses.
  // Each sending device has one set of registers starting at address 0.
  // Sending devices do not automatically identify themselves, 
  // so the sender must either first send it's identity, then send its register address and data,
  // or it must know which address to write to in the master.
  // The most generic approach is for the master to send to each slave the offset it should apply to the register address when sending data.
  // So it is assumed that each slave sends the correct offsetted address.
  setMyI2CAddress();
  createMyRegisters();
  if (i2C.address() == PROGRAMMER_I2C_ADDR) {
    sendI2CregisterOffsets();
    i2C.onReceive(all_registers.receiveI2C);
    i2C.onRequest(all_registers.requestI2C);
  } else {
    i2C.onReceive(rem_registers.receiveI2C);
    i2C.onRequest(rem_registers.requestI2C);
  }
  //i2C2.onReceive(rtc_registers.receiveI2C);
  //i2C2.onRequest(rtc_registers.requestI2C);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (all_registers.getRegister(slave_requesting_resend_data)) sendI2CregisterOffsets();
  Serial.println("\nSend Data");
  if (i2C.address() == PROGRAMMER_I2C_ADDR) sendDataToRemotes();
  else sendDataToProgrammer();
  printRegisters();
  delay(1000);

  Serial.println("\nRequest Data");
  if (i2C.address() == PROGRAMMER_I2C_ADDR) requestDataFromRemotes();
  else requestDataFromProgrammer();
  printRegisters();
  delay(1000);
}
