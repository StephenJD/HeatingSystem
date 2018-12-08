// MixValve_Run.h: interface for the MixValve_Run class.
//
//////////////////////////////////////////////////////////////////////

#if !defined (_MV_RUN_H_)
#define _MV_RUN_H_

#include "debgSwitch.h"
#include "A__Constants.h"
#include "D_Operations.h"
#include "D_EpData.h"
#include "Mix_Valve.h"
#if defined (ZPSIM)
	#include <fstream>
#endif

class MixValve_Run : public RunT<EpDataT<MIXV_DEF> >
{
public:
	MixValve_Run();
	bool needHeat(bool isHeating) const; // used by ThermStore.needHeat	
	U1_byte sendSetup() const;
	bool amControlZone(U1_byte callTemp, U1_byte maxTemp, U1_byte relayID);
	bool check();

	#if defined (ZPSIM)
		S2_byte getValvePos() const; // public for simulator
		void reportMixStatus() const;
		S1_byte controlRelay() const {return _controlZoneRelay;}
		U1_byte mixCallT() const {return _mixCallTemp;}
		S1_byte getState() const; // 1 = heat, 0 = off, -1 = cool
		
		volatile S1_byte motorState; // required by simulator
		static std::ofstream lf;
		~MixValve_Run(){lf.close();}
	#endif
	U1_byte readFromValve(Mix_Valve::Registers reg) const;
private:
	friend class MixValve_Stream;

	bool controlZoneRelayIsOn() const;
	U1_byte writeToValve(Mix_Valve::Registers reg, U1_byte value) const; // returns I2C Error 
	U1_byte getStoreTemp() const;

	void setLimitZone(U1_ind mixValveIndex);

	volatile U1_byte _mixCallTemp; 
	volatile U1_byte _controlZoneRelay;
	U1_byte _limitTemp;
};

#endif
