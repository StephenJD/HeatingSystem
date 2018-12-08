#include "ThrmStore_Stream.h"
#include "D_DataStream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "ThrmStore_Run.h"
#include "TempSens_Run.h"

using namespace Utils;

template<>
const char DataStreamT<EpDataT<THRMST_DEF>, ThrmSt_Run, ThrmStore_EpMan, TMS_COUNT>::setupPrompts[TMS_COUNT][13] = {
	"DHW Flo l/s:",
	"DHW Flo sec:",
	"Heat Relay:",
	"Gas TS:",
	"OvrHeatTemp:",
	"Ovr Heat TS:",
	"Mid TS:",
	"Lower TS:",
	"Mains TS:",
	"DHW PreX TS:",
	"DHW Flow TS:",
	"Outside TS:",
	"Conductivty:",
	"Cylindr Dia:",
	"Cylindr Hgt:",
	"TopSensHght:",
	"MidSensHght:",
	"BotSensHght:",
	"Boiler KW:"
};

template<>
ValRange DataStreamT<EpDataT<THRMST_DEF>, ThrmSt_Run, ThrmStore_EpMan, TMS_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	switch (item) { 
// DHWtestInterval,GasRelay,OvrHeatTemp,OvrHeatTS,LowerDhwTS,GroundTS,DHWFlowTS,OutsideTS,
// Conductivity,CylDia,CylHeight,TopSensHeight,MidSensHeight,LowerSensHeight,BoilerPower,TMS_COUNT};
	case GasRelay:
		strcat(fieldText,f->relays[myVal].dStream().getName());
		return ValRange(0,0,f->relays.last());
	case OvrHeatTS: // TempSensors
	case MidDhwTS:
	case LowerDhwTS:
	case GroundTS:
	case DHWFlowTS:
	case DHWpreMixTS:
	case OutsideTS:
	case GasTS:
		strcat(fieldText,f->tempSensors[myVal].dStream().getName());
		return ValRange(0,0,f->tempSensors.last());
	case OvrHeatTemp:
	case DHWflowRate:
	case DHWflowTime:
	case Conductivity:
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,0,99);
	default:
		strcat(fieldText,cIntToStr(myVal,3,'0'));
		return ValRange(3,0,255);
	}
	f->thermStoreR().calcCapacities();
}

ThrmStore_Stream::ThrmStore_Stream(ThrmSt_Run & run)
	: DataStreamT<EpDataT<THRMST_DEF>, ThrmSt_Run, ThrmStore_EpMan, TMS_COUNT>(run)
{}

const U1_byte ThrmStore_Stream::getNoOfStreamItems() const {return i_NoOfInfo;} // returns no of members of the Collection -1

char * ThrmStore_Stream::objectFldStr(S1_ind activeInd, bool inEdit) const {
	switch (activeInd) {
	case i_MainsT:
		strcpy(fieldText, "Mains:");
		strcat (fieldText,cIntToStr(runI().getGroundTemp(),2,'0'));
		break;
	case i_TopT:
		strcpy (fieldText,"Top:");
		strcat (fieldText,cIntToStr(runI().getTopTemp(),2,'0'));
		break;
	case i_MidT:
		strcpy (fieldText,"Mid:");
		strcat (fieldText,cIntToStr(f->tempSensorR(getVal(MidDhwTS)).getSensTemp(),2,'0'));
		break;
	case i_LwrT:
		strcpy (fieldText,"Lwr:");
		strcat (fieldText,cIntToStr(f->tempSensorR(getVal(LowerDhwTS)).getSensTemp(),2,'0'));
		break;
	case i_PreT:
		strcpy (fieldText,"PrH:");
		strcat (fieldText,cIntToStr(f->tempSensorR(getVal(DHWpreMixTS)).getSensTemp(),2,'0'));
		break;
	case i_dhwT:
		strcpy (fieldText,"DHW:");
		strcat (fieldText,cIntToStr(f->tempSensorR(getVal(DHWFlowTS)).getSensTemp(),2,'0'));
		break;
	case i_Cond:
		strcpy (fieldText,"Cnd:");
		strcat (fieldText,cIntToStr(getVal(Conductivity),2,'0'));
		break;
	case i_StoveT:
		{strcpy (fieldText,"MF:");
		strcat (fieldText,cIntToStr(f->tempSensorR(f->occasionalHeaters[0].getVal(FlowTS)).getSensTemp(),2,'0'));
		bool pumpOn = f->occasionalHeaters[0].run().check();
		strcat (fieldText, pumpOn ? "!" : " ");}
		break;
	case i_CoilT:
		strcpy (fieldText,"Cyl:");
		strcat (fieldText,cIntToStr(f->tempSensorR(f->occasionalHeaters[0].getVal(ThrmStrTS)).getSensTemp(),2,'0'));
		break;
	case i_GasT:
		strcpy (fieldText,"Gas:");
		strcat (fieldText,cIntToStr(f->tempSensorR(getVal(GasTS)).getSensTemp(),2,'0'));
		break;
	}
	fieldCText[0] = strlen(fieldText); // cursor offset
	fieldCText[1] = 0; // newLine
	return fieldCText;
}