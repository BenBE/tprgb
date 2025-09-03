#pragma once
/*
 * modbus.h
 *
 *  Created on: Aug 31, 2025
 *      Author: W
 */

#include <stdbool.h>
#include <stdint.h>


/* Modbus configuration */
#define MB_MAX_PACKET_LEN 256u
#define MB_MIN_PACKET_LEN 5u
#define MB_SLAVE_ADDRESS 0x01u
#define MB_MASTER_ADDRESS 0x00u

/* Modbus commands */
#define MB_FUNC_READ_COILS 1u

#define MB_FUNC_WRITE_SINGLE_COIL 5u
#define MB_FUNC_WRITE_SINGLE_REG 6u
#define MB_FUNC_WRITE_COILS 15u
#define MB_FUNC_WRITE_HOLDING_REG 16u
#define MB_FUNC_READ_HOLDING_REG 23u

#define MB_ADDR_HOLDING_REG_R 40001U
#define MB_ADDR_HOLDING_REG_G 40002U
#define MB_ADDR_HOLDING_REG_B 40003U
#define MB_ADDR_HOLDING_REG_W 40004U

#define MB_ADDR_HOLDING_REG_FLASH_START 40005U
#define MB_ADDR_HOLDING_REG_ERASE_FLASH 40006U
#define MB_ADDR_HOLDING_REG_FINISH_FLASH 40007U
#define MB_ADDR_HOLDING_REG_ABORT_FLASH 40008U

#define MB_EX_ILLEGAL_FUNCTION 0x01
#define MB_EX_ILLEGAL_DATA_ADDRESS 0x02
#define MB_EX_ILLEGAL_DATA_VALUE 0x03
#define MB_EX_DEVICE_FAILURE 0x04
#define MB_EX_ACKNOWLEDGE 0x05
#define MB_EX_DEVICE_BUSY 0x06
#define MB_EX_MEMORY_PARITY_ERROR 0x08
#define MB_EX_GATEWAY_PATH_UNAVAILABLE 0x0A
#define MB_EX_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND 0x0B


/* Modbus packet */
typedef struct {
	uint8_t address;
	uint8_t function;
	uint8_t data[252];  // 256 - 4 = 252
	uint8_t data_len;
} mb_packet;

extern volatile bool mb_can_evalueate_packet;

void mb_init();
bool mb_crc_is_ok();
mb_packet* mb_packet_eval();
void mb_packet_transmit(mb_packet *packet);

void mb_handle_irq(void);

void mb_packet_handle_default(mb_packet *packet);
