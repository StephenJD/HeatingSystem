#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>

#define SERIAL_RATE 115200

constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;
constexpr uint8_t US_CONSOLE_I2C_ADDR = 0x12;
constexpr uint8_t DS_CONSOLE_I2C_ADDR = 0x13;
constexpr uint8_t FL_CONSOLE_I2C_ADDR = 0x14;

I2C_Talk i2C;
//I2C_Talk i2C2(DS_CONSOLE_I2C_ADDR, Wire1);
  constexpr auto R_SLAVE_REQUESTING_INITIALISATION = 0;

	enum RemoteRegisterName {
		  R_DISPL_REG_OFFSET // ini
		, R_ROOM_TS_ADDR	// ini
		, R_ROOM_TEMP				    // send
		, R_ROOM_TEMP_FRACTION		// send
		, R_REQUESTING_T_RAIL		// send
		, R_REQUESTING_DHW		  // send
		, R_REQUESTED_ROOM_TEMP		  // send/receive
		, R_WARM_UP_ROOM_M10	// receive
		, R_ON_TIME_T_RAIL		// receive
		, R_DHW_TEMP			  // receive
		, R_WARM_UP_DHW_M10 // If -ve, in 0-60 mins, if +ve in min_10
		, R_DISPL_REG_SIZE
	};

 	enum MixValve_Volatile_Register_Names {
		// All registers are single-byte.
		// I2C devices use big-endianness: MSB at the smallest address: So a uint16_t is [MSB, LSB].
		status, mode, state, ratio
		, flow_temp, request_temp, moveFromTemp
		, count
		, valve_pos
		, moveFromPos
		, mixValve_volRegister_size
	};

	enum MixValve_EEPROM_Register_Names {
		  temp_i2c_addr = mixValve_volRegister_size
		, full_traverse_time
		, wait_time
		, max_flowTemp
		, mixValve_all_register_size
	};

  enum rtc_registers {
    dd
    , mm
    , yy
    , rtcRegister_size
  };

  constexpr auto remRegStart = 1 + mixValve_volRegister_size*2;
  auto all_registers = i2c_registers::Registers<remRegStart + 3*R_DISPL_REG_SIZE>{i2C};
  auto rem_registers = i2c_registers::Registers<R_DISPL_REG_SIZE>{i2C};
  //auto rtc_registers = i2c_registers::Registers<i2C2, rtcRegister_size>{};

void setMyI2CAddress() {
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  if (!digitalRead(5)) {
    i2C.setAsMaster(DS_CONSOLE_I2C_ADDR);
    Serial.println(F("DS"));
  }
  else if (!digitalRead(6)) {
    i2C.setAsMaster(US_CONSOLE_I2C_ADDR);  
    Serial.println(F("US"));
  } 
  else if (!digitalRead(7)) {
    i2C.setAsMaster(FL_CONSOLE_I2C_ADDR);
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
 
    all_registers.setRegister(remRegStart + R_REQUESTED_ROOM_TEMP,17);
    all_registers.setRegister(remRegStart + R_WARM_UP_ROOM_M10,12);
    all_registers.setRegister(remRegStart + R_ON_TIME_T_RAIL,55);
    all_registers.setRegister(remRegStart + R_DHW_TEMP,47);
    all_registers.setRegister(remRegStart + R_WARM_UP_DHW_M10,-35);
    
    all_registers.setRegister(remRegStart + R_DISPL_REG_SIZE + R_REQUESTED_ROOM_TEMP,18);
    all_registers.setRegister(remRegStart + R_DISPL_REG_SIZE + R_WARM_UP_ROOM_M10,13);
    all_registers.setRegister(remRegStart + R_DISPL_REG_SIZE + R_ON_TIME_T_RAIL,56);
    all_registers.setRegister(remRegStart + R_DISPL_REG_SIZE + R_DHW_TEMP,48);
    all_registers.setRegister(remRegStart + R_DISPL_REG_SIZE + R_WARM_UP_DHW_M10,-36);

    all_registers.setRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_REQUESTED_ROOM_TEMP,19);
    all_registers.setRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_WARM_UP_ROOM_M10,14);
    all_registers.setRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_ON_TIME_T_RAIL,57);
    all_registers.setRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_DHW_TEMP,49);
    all_registers.setRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_WARM_UP_DHW_M10,-37);
  } else {
    Serial.println(F("Create Rem"));
    rem_registers.setRegister(R_ROOM_TEMP,20);
    rem_registers.setRegister(R_REQUESTING_T_RAIL,0);
    rem_registers.setRegister(R_REQUESTING_DHW,1);
  }
}

void sendSlaveIniData() {
  Serial.println(F("Sending Offs to Rems"));
  i2C.write(DS_CONSOLE_I2C_ADDR, R_DISPL_REG_OFFSET, remRegStart);
  i2C.write(US_CONSOLE_I2C_ADDR, R_DISPL_REG_OFFSET, remRegStart + R_DISPL_REG_SIZE);
  i2C.write(FL_CONSOLE_I2C_ADDR, R_DISPL_REG_OFFSET, remRegStart + 2 * R_DISPL_REG_SIZE);
  all_registers.setRegister(R_SLAVE_REQUESTING_INITIALISATION,0);
}

void sendDataToRemotes() {
  i2C.write(DS_CONSOLE_I2C_ADDR, R_REQUESTED_ROOM_TEMP, 5, all_registers.reg_ptr(remRegStart + R_REQUESTED_ROOM_TEMP));
  i2C.write(US_CONSOLE_I2C_ADDR, R_REQUESTED_ROOM_TEMP, 5, all_registers.reg_ptr(remRegStart + R_DISPL_REG_SIZE + R_REQUESTED_ROOM_TEMP));
  i2C.write(FL_CONSOLE_I2C_ADDR, R_REQUESTED_ROOM_TEMP, 5, all_registers.reg_ptr(remRegStart + 2 * R_DISPL_REG_SIZE + R_REQUESTED_ROOM_TEMP));
}

void sendDataToProgrammer() {
  Serial.print(F("Send to Offs ")); Serial.println(rem_registers.getRegister(R_DISPL_REG_OFFSET));
  i2C.write(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(R_DISPL_REG_OFFSET) + R_ROOM_TEMP, 5, rem_registers.reg_ptr(R_ROOM_TEMP));
}

void requestDataFromRemotes() {
  i2C.read(DS_CONSOLE_I2C_ADDR, R_ROOM_TEMP, 5, all_registers.reg_ptr(remRegStart + R_ROOM_TEMP));
  i2C.read(US_CONSOLE_I2C_ADDR, R_ROOM_TEMP, 5, all_registers.reg_ptr(remRegStart + R_DISPL_REG_SIZE + R_ROOM_TEMP));
  i2C.read(FL_CONSOLE_I2C_ADDR, R_ROOM_TEMP, 5, all_registers.reg_ptr(remRegStart + 2 * R_DISPL_REG_SIZE + R_ROOM_TEMP));
}

void requestDataFromProgrammer() {
  i2C.read(PROGRAMMER_I2C_ADDR, rem_registers.getRegister(R_DISPL_REG_OFFSET) + R_REQUESTED_ROOM_TEMP, 5, rem_registers.reg_ptr(R_REQUESTED_ROOM_TEMP));
}

void printRegisters() {
  if (i2C.address() == PROGRAMMER_I2C_ADDR) {
    Serial.print(F("Of Req ")); Serial.println(all_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION));

    Serial.print(F("Of 1 ")); Serial.println(remRegStart);
    Serial.print(F("Of 2 ")); Serial.println(remRegStart + R_DISPL_REG_SIZE);
    Serial.print(F("Of 3 ")); Serial.println(remRegStart + 2*R_DISPL_REG_SIZE);

    Serial.print(F("Rr 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_REQUESTED_ROOM_TEMP));
    Serial.print(F("Rr 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_REQUESTED_ROOM_TEMP));
    Serial.print(F("Rr 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_REQUESTED_ROOM_TEMP));

    Serial.print(F("Rt 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_ROOM_TEMP));
    Serial.print(F("Rt 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_ROOM_TEMP));
    Serial.print(F("Rt 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_ROOM_TEMP));

    Serial.print(F("Rm 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_WARM_UP_ROOM_M10));
    Serial.print(F("Rm 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_WARM_UP_ROOM_M10));
    Serial.print(F("Rm 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_WARM_UP_ROOM_M10));

    Serial.print(F("TRr 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_REQUESTING_T_RAIL));
    Serial.print(F("TRr 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_REQUESTING_T_RAIL));
    Serial.print(F("TRr 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_REQUESTING_T_RAIL));

    Serial.print(F("TRm 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_ON_TIME_T_RAIL));
    Serial.print(F("TRm 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_ON_TIME_T_RAIL));
    Serial.print(F("TRm 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_ON_TIME_T_RAIL));

    Serial.print(F("HWr 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_REQUESTING_DHW));
    Serial.print(F("HWr 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_REQUESTING_DHW));
    Serial.print(F("HWr 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_REQUESTING_DHW));

    Serial.print(F("HWt 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_DHW_TEMP));
    Serial.print(F("HWt 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_DHW_TEMP));
    Serial.print(F("HWt 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_DHW_TEMP));

    Serial.print(F("HWm 1 ")); Serial.println(all_registers.getRegister(remRegStart + R_WARM_UP_DHW_M10));
    Serial.print(F("HWm 2 ")); Serial.println(all_registers.getRegister(remRegStart + R_DISPL_REG_SIZE + R_WARM_UP_DHW_M10));
    Serial.print(F("HWm 3 ")); Serial.println(all_registers.getRegister(remRegStart + 2*R_DISPL_REG_SIZE + R_WARM_UP_DHW_M10));
  } else {
    Serial.print(F("Of ")); Serial.println(rem_registers.getRegister(R_DISPL_REG_OFFSET));
    Serial.print(F("Rr ")); Serial.println(rem_registers.getRegister(R_REQUESTED_ROOM_TEMP));
    Serial.print(F("Rt ")); Serial.println(rem_registers.getRegister(R_ROOM_TEMP));
    Serial.print(F("Rm ")); Serial.println(rem_registers.getRegister(R_WARM_UP_ROOM_M10));
    Serial.print(F("TRr ")); Serial.println(rem_registers.getRegister(R_REQUESTING_T_RAIL)); 
    Serial.print(F("TRm ")); Serial.println(rem_registers.getRegister(R_ON_TIME_T_RAIL));
    Serial.print(F("HWr ")); Serial.println(rem_registers.getRegister(R_REQUESTING_DHW));
    Serial.print(F("HWt ")); Serial.println(rem_registers.getRegister(R_DHW_TEMP));
    Serial.print(F("HWm ")); Serial.println(rem_registers.getRegister(R_WARM_UP_DHW_M10));
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
    sendSlaveIniData();
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
  if (all_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION)) sendSlaveIniData();
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
