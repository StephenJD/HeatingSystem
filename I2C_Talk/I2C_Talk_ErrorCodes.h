#pragma once

namespace I2C_Talk_ErrorCodes {
	enum error_codes { _OK, _Insufficient_data_returned, _NACK_during_address_send, _NACK_during_data_send, _NACK_during_complete, _NACK_receiving_data, _Timeout, _disabledDevice, _slave_shouldnt_write, _I2C_not_created, _I2C_Device_Not_Found, _I2C_ReadDataWrong, _I2C_AddressOutOfRange };
}