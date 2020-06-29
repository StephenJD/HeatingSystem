#include <PinObject.h>

using namespace HardwareInterfaces;

constexpr uint32_t SERIAL_SPEED = 115200;

auto builtinLED = Pin_Wag{ LED_BUILTIN, HIGH };
auto otherLED = Pin_Wag{ 12, LOW };

void setup() {
	// put your setup code here, to run once:
	Serial.begin(SERIAL_SPEED);
	Serial.println("Pin Object");
	auto pinCopy{ builtinLED };
	auto otherCopy{ otherLED };
	Serial.print("builtinLED Port: "); Serial.print(builtinLED.port()); Serial.print(" State: "); Serial.println(builtinLED.logicalState());
	Serial.print("pinCopy Port: "); Serial.print(pinCopy.port()); Serial.print(" State: "); Serial.println(pinCopy.logicalState());
	Serial.print("otherLED Port: "); Serial.print(otherLED.port()); Serial.print(" State: "); Serial.println(otherLED.logicalState());
	Serial.print("otherCopy Port: "); Serial.print(otherCopy.port()); Serial.print(" State: "); Serial.println(otherCopy.logicalState());
}

void loop() {
	// put your main code here, to run repeatedly:
	builtinLED.set();
	delay(200);
	builtinLED.clear();
	delay(200);
}
