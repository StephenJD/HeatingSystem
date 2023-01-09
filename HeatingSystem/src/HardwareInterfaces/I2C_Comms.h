#pragma once

#include <OLED_Thick_Display.h>
#include <Mix_Valve.h>

namespace HardwareInterfaces {

	enum Register_Constants {
		R_SLAVE_REQUESTING_INITIALISATION // Never written to by slaves. Used only during initialisation
		, R_PROG_WAITING_FOR_REMOTE_I2C_COMS // Set to 0xF0 (1111,0000) by remote arduinos after mastering I2C coms.
		, PROG_REG_MV0_OFFSET
		, PROG_REG_MV1_OFFSET = PROG_REG_MV0_OFFSET + Mix_Valve::MV_VOLATILE_REG_SIZE
		, PROG_REG_RC_US_OFFSET = PROG_REG_MV0_OFFSET + Mix_Valve::MV_VOLATILE_REG_SIZE * NO_OF_MIXERS
		, PROG_REG_RC_DS_OFFSET = PROG_REG_RC_US_OFFSET + OLED_Thick_Display::R_DISPL_REG_SIZE
		, PROG_REG_RC_FL_OFFSET = PROG_REG_RC_DS_OFFSET + OLED_Thick_Display::R_DISPL_REG_SIZE
		, SIZE_OF_ALL_REGISTERS = PROG_REG_RC_FL_OFFSET + OLED_Thick_Display::R_DISPL_REG_SIZE
		, MV0_REG_OFFSET = 0
		, MV1_REG_OFFSET = MV0_REG_OFFSET + Mix_Valve::MV_ALL_REG_SIZE
	};

	enum SlaveRequestIni : uint8_t {
		NO_INI_REQUESTS
		, MV_US_REQUESTING_INI = 1
		, MV_DS_REQUESTING_INI = 2
		, RC_US_REQUESTING_INI = 4
		, RC_DS_REQUESTING_INI = 8
		, RC_FL_REQUESTING_INI = 16
		, ALL_REQUESTING = (RC_FL_REQUESTING_INI * 2) - 1
	};
}