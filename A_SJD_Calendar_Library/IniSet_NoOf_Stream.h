/* 
 * File:   IniSet_NoOf_Stream.h
 * Author: Stephen
 *
 * Created on 02 June 2014, 21:41
 */

#ifndef INISET_NOOF_STREAM_H
#define	INISET_NOOF_STREAM_H

#include "D_DataStream.h"
#include "D_EpDat_A.h"
#include "D_EpData.h"

class IniSet_EpMan;
class NoOf_EpMan;

////////////////////////////// IniSet_Stream /////////////////////////////////////

class IniSet_Stream : public DataStreamT<EpDataT<INI_DEF>, RunT<EpDataT<INI_DEF> >, IniSet_EpMan, INI_COUNT>
{
public:	
	IniSet_Stream(RunT<EpDataT<INI_DEF> > & run);
	S1_newPage onItemSelect(S1_byte myListPos, D_Data & d_data);
};

////////////////////////////// NoOf_Stream /////////////////////////////////////

class NoOf_Stream : public DataStreamT<NoOf_EpD, RunT<NoOf_EpD>, NoOf_EpMan, noAllDPs>
{
public:	
	NoOf_Stream(RunT<NoOf_EpD> & run);
};

////////////////////////////// InfoMenu_Stream /////////////////////////////////////

class InfoMenu_Stream : public DataStreamT<EpDataT<INFO_DEF>, RunT<EpDataT<INFO_DEF> >, IniSet_EpMan, INFO_COUNT>
{
public:	
	InfoMenu_Stream(RunT<EpDataT<INFO_DEF> > & run);
	S1_newPage onItemSelect(S1_byte myListPos, D_Data & d_data);
};

#endif	/* INISET_NOOF_STREAM_H */

