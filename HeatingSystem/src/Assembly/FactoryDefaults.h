#pragma once
#include "..\HeatingSystem.h"
#include <RDB.h>

namespace Assembly {
	using namespace RelationalDatabase;
	void setFactoryDefaults(RDB<TB_NoOfTables> & db, size_t password);
}