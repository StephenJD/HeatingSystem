#if !defined (_F_Factory_h_)
#define _F_Factory_h_

///////////////////////////// Factory //////////////
// Single Responsibility is to create/initialise  //
// Groups for each object type	                  //	
////////////////////////////////////////////////////
#include "A__Constants.h"
#include "D_Group.h"
#include "DateTime_Stream.h"
#include "DateTime_Run.h"

#if defined (ZPSIM)
	extern DTtype CURR_TIME;
#endif
//#include "D_EpData.h"
class DailyProg_Stream;
class TempSens_Run;
class Relay_Run;
class Zone_Run;
class TimeTemp_Stream;
class MixValve_Run;
class ThrmSt_Run;
class Events_Stream;
class Display_Stream;
class FactoryObjects;
void setFactory(FactoryObjects * fact);

void setFactory(FactoryObjects * fact);

FactoryObjects * getFactory(FactoryObjects * fact = 0);

class Factory {
public:
	enum {EEPROM_OK = -1, RELOAD_EEPROM = 0};
	Factory();
	static bool initialiseProgrammer(); // Dummy return val to allow call by Displays() base-class constructor 
	static U1_byte noOf(noItemsVals myObject); 
	FactoryObjects * createFactObject();

	Group * numberOf;
	Group * iniSetup;
	Group * thermStore;
	Group * displays;
	Group * relays;
	Group * mixingValves;
	Group * tempSensors;
	Group * occasionalHeaters;
	Group * zones;
	Group * towelRads;
	Group * dateTimes;
	Group * dailyPrograms;
	Group * timeTemps;
	Group * infoMenu;
	Group * events;

	~Factory();
private:
	static void setVersion(const char *version);
	static char *getVersion();
	void createObjects(S1_byte loadDefaults);
	Factory(const Factory &); // disable copying
};

// The FactoryObjects class is a simple mechanism for producing references to the
// dynamically created Group objects for ease of use by the rest of the system
// The horrible dereferencing of the Factory class pointers is done in the 
// FactoryObjects constructor argument list, called by the Factory class after creating
// and initiallising the Group objects
class FactoryObjects {
public:
	enum {EEPROM_OK = -1, RELOAD_EEPROM = 0};
	FactoryObjects(
		Group & numberOf, 
		Group & iniSetup, 
		Group & thermStore, 
		Group & displays, 
		Group & relays, 
		Group & mixingValves, 
		Group & tempSensors,
		Group & occasionalHeaters,
		Group & zones,
		Group & towelRads,
		Group & dateTimes,
		Group & dailyPrograms,
		Group & timeTemps,
		Group & infoMenu,
		Group & events
		);

	static Group & refreshTTs();
	static Group & refreshDPs();
	static Group & refreshDTs();

	Group & numberOf;
	Group & iniSetup;
	Group & thermStore;
	Group & displays;
	Group & relays;
	Group & mixingValves;
	Group & tempSensors;
	Group & occasionalHeaters;
	Group & zones;
	Group & towelRads;
	Group & dateTimes;
	Group & dailyPrograms;
	Group & timeTemps;
	Group & infoMenu;
	Group & events;

	TempSens_Run & tempSensorR(U1_byte index);
	Relay_Run & relayR(U1_byte index);
	MixValve_Run & mixValveR(U1_byte index);
	Zone_Run & zoneR(U1_byte index);
	ThrmSt_Run & thermStoreR();

	Zone_Stream & zoneS(U1_byte index);
	DateTime_Stream & dateTimesS(U1_byte index);
	DailyProg_Stream & dailyProgS(U1_byte index);
	TimeTemp_Stream & timeTempS(U1_byte index);
	Events_Stream & eventS();
	Display_Stream & displayS(U1_byte index);
};

//extern FactoryObjects * f; // Factory object created by initialiseProgrammer().

#endif