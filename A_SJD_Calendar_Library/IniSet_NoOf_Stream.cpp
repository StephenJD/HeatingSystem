#include "IniSet_NoOf_Stream.h"
#include "A_Displays_Data.h"
////////////////////////////// IniSet_Stream /////////////////////////////////////

IniSet_Stream::IniSet_Stream(RunT<EpDataT<INI_DEF> > & run) : DataStreamT<EpDataT<INI_DEF>, RunT<EpDataT<INI_DEF> >, IniSet_EpMan, INI_COUNT>(run) {}

S1_newPage IniSet_Stream::onItemSelect(S1_byte myListPos, D_Data & d_data){
	return myListPos + mp_set_displ;
}

////////////////////////////// NoOf_Stream /////////////////////////////////////

NoOf_Stream::NoOf_Stream(RunT<NoOf_EpD> & run) : DataStreamT<NoOf_EpD, RunT<NoOf_EpD>, NoOf_EpMan, noAllDPs>(run) {}

////////////////////////////// InfoMenu_Stream /////////////////////////////////////

InfoMenu_Stream::InfoMenu_Stream(RunT<EpDataT<INFO_DEF> > & run) : DataStreamT<EpDataT<INFO_DEF>, RunT<EpDataT<INFO_DEF> >, IniSet_EpMan, INFO_COUNT>(run) {}

S1_newPage InfoMenu_Stream::onItemSelect(S1_byte myListPos, D_Data & d_data){
	if (myListPos == 0) return mp_sel_setup; else return myListPos + mp_infoHome;
}