#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_Device.h>
#include <I2C_RecoverRetest.h>
#include <I2C_Scan.h>
#include <Logging.h>
#include <EEPROM.h>
#include <Timer_mS_uS.h>
#include <Watchdog_Timer.h>

#define SERIAL_RATE 115200

//////////////////////////////// Start execution here ///////////////////////////////
using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_allwaysFlush);
		return _log;
	}
}
using namespace arduino_logger;

#if defined(__SAM3X8E__)
	constexpr uint8_t EEPROM_I2C_ADDR = 0x50;

	I2C_Talk rtc{ Wire1 };

	EEPROMClassRE& eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1,100000),rtc.extendTimeouts(5000, 5, 1000),EEPROM_I2C_ADDR) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}
	EEPROMClassRE& EEPROM = eeprom();
#else
	EEPROMClassRE& eeprom() {
		return EEPROM;
	}
#endif

I2C_Talk i2C{};

I2C_Recover_Retest i2c_recover(i2C);
//I2C_Recover i2c_recover(i2C);

constexpr int MAX_NO_OF_MASTERS = 16;

auto register_set = i2c_registers::Registers<MAX_NO_OF_MASTERS>{ i2C };
int noOfMasters = 0;
int myMasterIndex = 0;
int prevRecipientIndex = 0;
int nextRecipientIndex = 0;

I_I2Cdevice_Recovery masters[MAX_NO_OF_MASTERS] = { {i2c_recover}};
uint8_t sentMessages[MAX_NO_OF_MASTERS] = {0};
uint8_t messagesToSend[MAX_NO_OF_MASTERS] = {0};
int badMessageCount = 0;
int mastersForwardingOK = 10;
constexpr uint8_t HAPPY_MESSAGE = 1;
constexpr uint8_t LOST_MASTER_MESSAGE = 2;

void setI2CAddress(I2C_Talk & i2C) {
	uint8_t address = 0;
	uint8_t mask = 1;
	for (int pin = 2; pin < 8; ++pin) { // pins 2-7 allow address setting in binary from 0-63
		pinMode(pin, INPUT_PULLUP);
		if (digitalRead(pin)) address |= mask;
		mask <<= 1;
	}
	i2C.setAsMaster(address);
	logger() << F("This Master at 0x") << L_hex << address << L_endl;
}

void setNext_PrevRecipients() {
	prevRecipientIndex = myMasterIndex - 1;
	if (prevRecipientIndex < 0) prevRecipientIndex = noOfMasters - 1;
	nextRecipientIndex = myMasterIndex + 1;
	if (nextRecipientIndex == noOfMasters) nextRecipientIndex = 0;
}

void findOtherMasters() {
	logger() << F("\nfindOtherMasters") << L_endl;
	auto scanner = I2C_Scan(i2C);
	scanner.scanFromZero();
	auto myMasterAddress = i2C.address();
	bool masterIndexPasssed = false;
	myMasterIndex = 0;
	noOfMasters = 0;
	while (scanner.show_next()) {
		reset_watchdog();
		badMessageCount = 1;
		flashLED();
		if (scanner.foundDeviceAddr < myMasterAddress) ++myMasterIndex;
		else if (!masterIndexPasssed && scanner.foundDeviceAddr > myMasterAddress) {
			++noOfMasters;
			masterIndexPasssed = true;
		}
		masters[noOfMasters].setAddress(scanner.foundDeviceAddr);
//logger() << F("\nMaster at: 0x") << L_hex << masters[noOfMasters].getAddress() << L_endl;
//delay(1000);
		++noOfMasters;
	}
	masters[myMasterIndex].setAddress(myMasterAddress);
	if (myMasterIndex == noOfMasters) ++noOfMasters;

	for (int i = 0; i < noOfMasters; ++i) {
		logger() << F("Master at: 0x") << L_hex << masters[i].getAddress() << L_endl;
	}
	logger() << F("This Master Index:") << myMasterIndex << L_endl << L_endl;
	setNext_PrevRecipients();
	mastersForwardingOK = 100;
}

void setup() {
	Serial.begin(SERIAL_RATE);
	set_watchdog_timeout_mS(16000); // max 16S
	//disable_watchdog();
	logger().begin(SERIAL_RATE);
	logger() << F("Setup Started") << L_endl;
delay(1000);
	setI2CAddress(i2C);
	i2C.setTimeouts(10000, I2C_Talk::WORKING_STOP_TIMEOUT, 20000);
	i2C.setMax_i2cFreq(100000);
	i2C.onReceive(register_set.receiveI2C);
	i2C.onRequest(register_set.requestI2C);
	i2C.begin();
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
	findOtherMasters();
	//set_watchdog_timeout_mS(16000); // max 16S
	logger() << F("\nSetup Done") << L_flush;
}

//uint8_t createMessageFor(int recipientIndex) {
//	return uint8_t(myMasterIndex * MAX_NO_OF_MASTERS + recipientIndex); // == 1!
//}

bool thisIsOneIsent(uint8_t & message) {
	if (myMasterIndex == 0) {
		if (message == HAPPY_MESSAGE || message == LOST_MASTER_MESSAGE) return true;
		else {
			logger() << myMasterIndex << F("\n*** Send LostMasterMsg ***\n") << L_endl;
			message = LOST_MASTER_MESSAGE;
		}
	}
	return false;
	//auto messageBase = myMasterIndex * MAX_NO_OF_MASTERS;
	//return message >= messageBase && message < messageBase + MAX_NO_OF_MASTERS;
}

uint8_t readMyRegisterOnRemote(int remoteIndex) {
	if (masters[remoteIndex].getStatus() == _disabledDevice) masters[remoteIndex].reset();
	uint8_t message = 0;
	masters[remoteIndex].read(myMasterIndex, 1, &message);
	return message;
}

bool lastMessageHasBeenProcessed(int remoteIndex) {
	uint8_t remoteMessage = readMyRegisterOnRemote(remoteIndex);
	if (remoteMessage) {
		if (remoteMessage == sentMessages[remoteIndex]) {
			logger() << myMasterIndex << F(": Wait for M") << remoteIndex << F(" has ") << remoteMessage<< L_endl;
			return false;
		} else {
			logger() << myMasterIndex << F(": Bad message at M") << remoteIndex << F(" has ") << remoteMessage<< L_endl;
			++badMessageCount;
		}
	}
	return true;
}

void sendMessageTo(int remoteIndex, uint8_t message) {
	if (remoteIndex == myMasterIndex) {
		logger() << myMasterIndex << F(": Error trying to send to M") << remoteIndex << L_endl;
		sendMessageTo(0, LOST_MASTER_MESSAGE + myMasterIndex);
		return;
	}
	auto timeout = Timer_mS(10);
	while (!timeout && !lastMessageHasBeenProcessed(remoteIndex));
	if (timeout) {
		logger() << myMasterIndex << F(": forced send to break deadly-embrace with M") << remoteIndex << L_endl;
	}
	logger() << myMasterIndex << F(": at: ") << millis() << F(" : sendMessage ") << message << F(" to M") << remoteIndex << F(" at 0x") << L_hex << masters[remoteIndex].getAddress() << L_endl;
	masters[remoteIndex].write(myMasterIndex, 1, &message);
	sentMessages[remoteIndex] = message;
}

bool retrieveMessages() {
	bool foundAMessage = false;
	for (int reg = 0; reg < MAX_NO_OF_MASTERS; ++reg) {
		reset_watchdog();
		auto message = register_set.getRegister(reg);
		if (message != 0) {
			register_set.setRegister(reg, 0);
			foundAMessage = true;
			logger() << myMasterIndex << F(": at: ") << millis() << F(" : Received ") << message << F(" from M") << reg << L_endl;
			if (thisIsOneIsent(message)) {
				logger() << myMasterIndex << F(": One I sent!") << L_endl;
				initiateMessage(1);
				mastersForwardingOK = 10;
			} else {
				sendMessageTo(nextRecipientIndex, message);
				if (message == HAPPY_MESSAGE) {
					mastersForwardingOK = 10;
				} else {
					findOtherMasters();
					break;
				}
			}
		}
	}
	return foundAMessage;
}

void initiateMessage(int recipient) {
	logger() << L_endl << myMasterIndex << F(": Initiate to M") << recipient << L_endl;
	sendMessageTo(recipient, HAPPY_MESSAGE);
}

void sendCheckMastersMessage() {
	logger() << L_endl << myMasterIndex << F("\nsendCheckMastersMessage\n") << L_endl;
	findOtherMasters();
	auto recipient = 0;
	if (myMasterIndex == 0) recipient = 1;
	sendMessageTo(recipient, LOST_MASTER_MESSAGE + myMasterIndex);
	mastersForwardingOK = 100;
}

int flashLength() {
	if (badMessageCount) {
		badMessageCount = 0;
		logger() << "Bad Msg" << L_endl;
		return 10;
	} else if (mastersForwardingOK > 10) {
		--mastersForwardingOK;
		logger() << "Try: " << mastersForwardingOK << L_endl;
		return 50;
	} else if (mastersForwardingOK > 0) {
		--mastersForwardingOK;
		return 200;
	} else {
		return 1000;
	}
}

void flashLED() {
	static auto startFlash = millis();
	static auto endFlash = startFlash + flashLength();
	if (millis() > startFlash) {
		auto flash_length = flashLength();
		digitalWrite(LED_BUILTIN, HIGH);
		endFlash = startFlash + flash_length;
		startFlash = endFlash + flash_length;
	} else if (millis() > endFlash) {
		digitalWrite(LED_BUILTIN, LOW);
	}
}

void loop() {
	/*
	* Up to 16 masters can be messaged.
	* Each master has an index, 0-noOfMasters determined by it's order in the discovered I2C device addresses.
	* Each master owns one register, R[m] which is the only register it ever writes to on other masters.
	* Each initiates and sends a message to each of the other masters, M[0..z]R[m]. The content is the m * 16 plus RecipientIndex.
	* The master then reads it's own registers. For each non-zero message:
	*	If it finds a message in R[m-1]:
	*		If the message is the one it originated, it increments the goodMessageCount.
	*		Else it sends it to the next master M[m+1]R[m].
	*	If it find a message in any other R[x]:
	*		If x+1 == m it sends it to M[x]R[m]
	*		Else it sends it to  M[x+1]R[m]
	*	After reading a register, it sets it to zero.
	*	Before sending a message, it reads M[x]R[m]:
	*		If the register is zero, it sends the message.
	*		If the register is still the value it last sent, it keeps reading.
	*		If the register is any other value it increments the badMessageCount, and sends the message.
	*		For each master, it remembers the last message sent.
	*	While badMessageCount > 0, fast-flash the LED and decrement the count.
	*	Else slow-flash the LED.
	*/
	reset_watchdog();
	if (!mastersForwardingOK) sendCheckMastersMessage();
	bool foundAMessage = retrieveMessages();
	//if (myMasterIndex == 0 && !foundAMessage) {
	if (myMasterIndex == 0 && mastersForwardingOK == 0) {
		initiateMessage(1);
	}
	flashLED();
	//delay(1000);
	//Serial.println(millis() / 1000);
}