#pragma once

#include "debgSwitch.h"
#include "A__Constants.h"
#include "D_DataStream.h"
#include "D_EpData.h"
#include "DateTime_A.h"

class Zone_Run;
class Zone_EpMan;
struct ValRange;

class Zone_Stream : public DataStreamT<Zone_EpD, Zone_Run, Zone_EpMan, Z_SET_VALS>
{
public:	
	Zone_Stream(Zone_Run & zRun);
	// Specialised methods
	char * objectFldStr(S1_ind activeInd, bool inEdit) const; 
	S1_newPage onSelect(D_Data & d_data); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage startEdit(D_Data & d_data);
	S1_newPage onItemSelect(S1_byte myListPos, D_Data & d_data);
	void save();
	void saveItem(S1_ind item);
	// Delegated methods
	char * getAbbrev() const;

	struct zdtData { // data for currently displayed zone and DT
		//zdtData() : firstDT(255){}
		U1_ind firstDT;
		U1_ind lastDT;
		U1_ind firstDP;

		U1_ind viewedDTposition; // set by Field from::up_down_key
		DTtype fromDate;
		DTtype expiryDate;
		U1_ind currDT;
		U1_ind currDP;
		U1_ind nextDT;
		E_dpTypes currDPtype;
		U1_ind prevDP; // for returning if cancelling a newProg
	} static currZdtData;

	// Extra Query methods
	U1_byte noOfUsrSets() const {return Z_NUM_USR_VALS - Z_SET_VALS + 1;}
	char * abbrevFldStr(bool inEdit, char newLine = 0) const;
	char * offsetFldStr(bool inEdit, char newLine = 0) const;
	char * usrFldStr(S1_ind item, bool inEdit) const;
	char * dtFromFldStr(S1_ind item, bool inEdit, char newLine = 0) const; 
	char * dtToFldStr(S1_ind item, bool inEdit, char newLine = 0) const; 
	char * progFldStr(S1_ind item, bool inEdit, char newLine = 0) const;
	char * remoteFldStr(S1_ind activeInd, bool inEdit) const;
	U1_byte lastDT() const;
	U1_byte lastDP() const;
	bool isFirstNamedDP(S1_ind item) const;
	U1_ind nextDP(U1_ind currDP, S1_byte move) const; // returns actual index of next uniquly named DP
	U1_ind sameDP(U1_ind currDP, S1_byte move) const; // returns actual index of next DP of same name


	// Extra Modifier methods
	S1_newPage onUsrSelect(S1_byte myListPos, D_Data & d_data);
	void saveUsr(S1_ind item);
	S1_newPage abbrevSelect();
	S1_newPage offsetSelect();
	S1_newPage fromSelect();
	S1_newPage toSelect();
	S1_newPage progEdit();
	U1_ind insertNewDT(S1_inc move); // returns new index if new DT created
	void deleteDT();
	void commit();
	void load();
	void setZdata(S1_ind dtPos = -1, S1_inc move = 0) const;
	void userRequestTempChange(S1_byte increment);

	void abbrevSave();
	void offsetSave();
	void fromSave();
	void toSave();
	S1_newPage progSave();
	S1_newPage nameEdit();
	static void refreshDTsequences();

protected:
	// Extra methods
	S1_newPage onNameSelect();
	const char * getUsrPrompts(S1_ind item) const;
	U2_byte getUserSetting(S1_ind item) const;
	ValRange addUsrValToFieldStr(S1_ind item, S2_byte myVal = 0) const; // return false for multi-position edit
	//S1_ind hasDPname(const std::string & name) const;
	S1_ind hasMatchingDT(DTtype originalFromDate, DTtype originalToDate, S1_ind currDP) const;
	void setUserSetting(S1_ind item, S2_byte myVal);
	void synchroniseZones(DTtype originalFromDate, DTtype originalToDate);

	// Delegated methods
	void setAutoMode(U1_byte mode);
	U1_byte getAutoMode() const;
	U1_byte getGroup() const;
	U1_byte getWeatherSetback() const;
	
private:
	bool dtIsCurrWeekly(U1_ind dtInd) const;
	S1_byte findDT(DTtype fromDate, S1_ind DPindex) const;
	//U1_ind getNthDP(S1_ind item) const; // returns actual index
	// Extra data
	static const char userPrompts[][40];
};

typedef Zone_Stream::zdtData ZdtData;

class EditTempReq : public Edit {
public:	
	void setRange(const ValRange & vRange, S2_byte currVal, Zone_Stream * zone);
	void nextVal(S2_byte move);
	void cancelEdit();
private:
	Zone_Stream * zone;
} extern editTempReq;


#if defined (ZPSIM)
#include <string>
std::string debugDPs(char* msg);
std::string debugCurrTime();
#endif