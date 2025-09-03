#pragma once
/*
 * modbus_crc.h
 *
 *  Created on: Jul 28, 2025
 *      Author: W
 */

#include <stdint.h>


uint16_t crc16(uint8_t *buffer, uint16_t buffer_length);
