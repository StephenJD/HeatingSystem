//#define CATCH_CONFIG_MAIN // Make Catch provide main()
//// - only do this in one cpp file!
//#include <catch.hpp>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>


void GetConsolePos(int *x, int *y, int *width, int * height) {
	RECT rect = { NULL };
	if (GetWindowRect(GetConsoleWindow(), &rect)) {
		*x = rect.left;
		*y = rect.top;
		*width = rect.right - rect.left;
		*height = rect.bottom - rect.top;
	}
}

void SetConsolePos(int x, int y, int width, int height) {
	HWND consoleWindow = GetConsoleWindow();
	SetWindowPos(consoleWindow, 0, x, y, width, height, SWP_NOZORDER);
}

LSTATUS WriteRegistry(LPCSTR sPath, LPCSTR sKey, int value)
{
	HKEY hKey;
	LSTATUS nResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, sPath,0, KEY_ALL_ACCESS, &hKey);
	if (nResult == ERROR_SUCCESS)
	{
		nResult = RegSetValueEx(hKey, sKey, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
		RegCloseKey(hKey);
	}

	return (nResult);
}

LSTATUS CreateRegistryEntry(LPCSTR sPath) {
	HKEY hKey;
	DWORD dwType = REG_DWORD;
	LSTATUS nResult = RegCreateKeyEx(HKEY_CURRENT_USER, sPath, 0, NULL, REG_OPTION_CREATE_LINK, KEY_ALL_ACCESS, NULL, &hKey, &dwType);
	return nResult;
}

LSTATUS ReadRegistry(LPCSTR sPath, LPCSTR sKey, int * value)
{
	HKEY hKey;
	LSTATUS nResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, sPath,0, KEY_READ | KEY_WOW64_64KEY, &hKey);

	if (nResult == ERROR_SUCCESS)
	{
		DWORD dataLength = sizeof(*value);
		DWORD dwType = REG_DWORD;
		nResult = RegQueryValueEx(hKey, sKey, NULL, &dwType,	(LPBYTE)value, &dataLength);
		if (nResult != ERROR_SUCCESS) {
			nResult = WriteRegistry(sPath, sKey, *value);
		}
		RegCloseKey(hKey);
	}
	else {
		nResult = CreateRegistryEntry(sPath);
		nResult = WriteRegistry(sPath, sKey, *value);
	}
	return (nResult);
}

int main(int argc, char* argv[])
{
	// global setup...
	int consoleLeft = 0;
	int consoleTop = 0;
	int consoleWidth = 150;
	int consoleHeight = 40;

	auto failed = ReadRegistry("Software\\ZoneProgSim", "ConsoleLeft", &consoleLeft);
	failed = ReadRegistry("Software\\ZoneProgSim", "ConsoleTop", &consoleTop);
	failed = ReadRegistry("Software\\ZoneProgSim", "ConsoleWidth", &consoleWidth);
	failed = ReadRegistry("Software\\ZoneProgSim", "ConsoleHeight", &consoleHeight);
	if (!failed) {
		SetConsolePos(consoleLeft,consoleTop,consoleWidth, consoleHeight);
	}
	   
	int result = Catch::Session().run(argc, argv);

	// global clean-up...
 	GetConsolePos(&consoleLeft, &consoleTop, &consoleWidth, &consoleHeight);
	if (!failed) {
		WriteRegistry("Software\\ZoneProgSim", "ConsoleLeft", consoleLeft);
		WriteRegistry("Software\\ZoneProgSim", "ConsoleTop", consoleTop);
		WriteRegistry("Software\\ZoneProgSim", "ConsoleWidth", consoleWidth);
		WriteRegistry("Software\\ZoneProgSim", "ConsoleHeight", consoleHeight);
	}

	return 0;
}