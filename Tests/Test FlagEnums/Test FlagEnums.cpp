// Test FlagEnums.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Flag_Enum.h>
#include <Logging.h>
#include <Arduino.h>

constexpr uint32_t SERIAL_RATE = 115200;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_allwaysFlush);
		return _log;
	}
}
using namespace arduino_logger;
using namespace flag_enum;

enum TestFlags { F0, F1, F2, F3, F4, F5, F6, F7, NoOfFlags };
using FlagValType = uint8_t;

void test(int is, int shouldBe, const char* msg) {
	if (is != shouldBe) {
		logger() << msg << L_tabs << F("Required") << shouldBe << F("But got") << is << L_endl;
	}
}

int main()
{
	logger() << "Start" << L_endl;
	FlagValType flagVal = 0;
	FlagValType topBit = 1 << sizeof(FlagValType) * 8 - 1;
	auto feRef = FE_Ref<TestFlags, NoOfFlags>(flagVal);
	test(FlagValType(feRef), 0, "IniZero");
	FlagValType expectedWhole = 0;
	for (int i = 0; i < NoOfFlags; ++i) {
		auto flag = TestFlags(i);
		feRef.set(flag);
		test(feRef.is(flag), 1, "set1");
		expectedWhole |= topBit >> i;
		test(flagVal, expectedWhole, "Val");
	}

	for (int i = 0; i < NoOfFlags; ++i) {
		auto flag = TestFlags(i);
		feRef.clear(flag);
		test(feRef.is(flag), 0, "set1");
		expectedWhole &= ~(topBit >> i);
		test(flagVal, expectedWhole, "Val");
	}

	for (int i = 0; i < NoOfFlags; ++i) {
		auto flag = TestFlags(i);
		FE_Ref<TestFlags, NoOfFlags>(flagVal).set(flag);
		auto feObj = FE_Obj<TestFlags, NoOfFlags>(flagVal);

		test(feObj.is(flag), 1, "obj1");
		expectedWhole |= topBit >> i;
		test(flagVal, expectedWhole, "ObjVal");
	}

	for (int i = 0; i < NoOfFlags; ++i) {
		auto flag = TestFlags(i);
		FE_Ref<TestFlags, NoOfFlags>(flagVal).clear(flag);
		auto feObj = FE_Obj<TestFlags, NoOfFlags>(flagVal);
		
		test(feObj.is(flag), 0, "set1");
		expectedWhole &= ~(topBit >> i);
		test(flagVal, expectedWhole, "Val");
	}

	for (int i = 0; i < NoOfFlags; ++i) {
		auto flag = TestFlags(i);
		FE_Ref<TestFlags, NoOfFlags>(flagVal).set(flag);
		auto feObj = FE_Obj<TestFlags, NoOfFlags>{};
		feObj = flagVal;

		test(feObj.is(flag), 1, "obj1");
		expectedWhole |= topBit >> i;
		test(flagVal, expectedWhole, "ObjVal");
	}

	for (int i = 0; i < NoOfFlags; ++i) {
		auto flag = TestFlags(i);
		FE_Ref<TestFlags, NoOfFlags>(flagVal).clear(flag);
		auto feObj = FE_Obj<TestFlags, NoOfFlags>{};
		feObj = flagVal;
		test(feObj.is(flag), 0, "set1");
		expectedWhole &= ~(topBit >> i);
		test(flagVal, expectedWhole, "Val");
	}
	logger() << "Tests done" << L_endl;
}