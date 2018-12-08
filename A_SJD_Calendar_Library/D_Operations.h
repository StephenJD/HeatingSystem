#pragma once

#include "A__Constants.h"
class EpData;
class DataStream;
class EpManager;
class FactoryObjects;
///////////////////////////// Run /////////////////////////////////////

class Run
{
public:
	Run(EpData & epD);
	const EpData & epD() const {return epDataR;}
	EpData & epD() {
		return epDataR;
	}
	virtual bool check() {return false;}
	// delegated methods
	void setVal(U1_ind vIndex, U1_byte val);
	U1_byte getVal(U1_ind vIndex) const;
protected:
	static FactoryObjects * f;
	friend void setFactory(FactoryObjects *);
private:
	EpData & epDataR;
};

///////////////////////////// RunT Template /////////////////////////////////////
template<class T_EpD>
class RunT : public Run
{
public:
	RunT();
	T_EpD & epDI() {return epData;}
	const T_EpD & epDI() const {return epData;}

private:
	T_EpD epData;
};

template<class T_EpD>
RunT<T_EpD>::RunT() : Run(epData) {}

///////////////////////////// Operations Root /////////////////////////////////////
//   Templated class contains a Run, DataStream and EEpromManager object         //
///////////////////////////////////////////////////////////////////////////////////

class Operations
{
public:
	Operations() {}
	// delegated methods
	void setVal(U1_ind vIndex, U1_byte val);
	U1_byte getVal(U1_ind vIndex) const;

	virtual EpData & epD() = 0;
	virtual const EpData & epD() const = 0;
	virtual DataStream & dStream() = 0;
	virtual Run & run() = 0;
	virtual void loadDefaults() = 0;
	virtual EpManager & epM() = 0;
};

///////////////////////////// OperationsT Template /////////////////////////////////////

template <class T_Run, class T_DataStream>
class OperationsT : public Operations
{
public:
	OperationsT();
	DataStream & dStream() {return serObj;}
	Run & run() {return runObj;}
	const EpData & epD() const {return runObj.epDI();}
	EpData & epD() {return runObj.epDI();}
	EpManager & epM() {return serObj.epMi();}
	void loadDefaults();

private:
	T_Run runObj; // contains EpData	
	T_DataStream serObj; // contains EpManager	
};

template <class T_1, class T_2>
OperationsT<T_1,T_2>::OperationsT()
	: runObj(),
	serObj(runObj)	
	{}