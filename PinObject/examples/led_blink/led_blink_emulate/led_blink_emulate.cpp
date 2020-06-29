// led_blink_emulate.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <PinObject.h>

#include <iostream>
using namespace std;

using namespace HardwareInterfaces;

int main()
{
    std::cout << "LED_Blink\n"; 
	auto builtinLED = Pin_Wag{ 13, 1 };
    std::cout << "\notherLED\n"; 
	auto otherLED = Pin_Wag{ 12, 0 };
	otherLED.set();
    std::cout << "\npinCopy\n"; 

	auto pinCopy{ builtinLED };
    std::cout << "\notherCopy\n"; 
	auto otherCopy{ otherLED };

	cout << "\nbuiltinLED Port: " << int(builtinLED.port()) << " State: " << int(builtinLED.logicalState()) << endl;
	cout << "pinCopy Port: " << int(pinCopy.port()) << " State: " << int(pinCopy.logicalState()) << endl;
	cout << "otherLED Port: " << int(otherLED.port()) << " State: " << int(otherLED.logicalState()) << endl;
	cout << "otherCopy Port: " << int(otherCopy.port()) << " State: " << int(otherCopy.logicalState()) << endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
