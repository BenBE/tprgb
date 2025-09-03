#include "modbus.h"

/*
 * modbus.c
 *
 *  Created on: Aug 31, 2025
 *      Author: W
 */

#include <string.h>

#include "stm32f0xx_ll_usart.h"

#include "modbus_crc.h"


volatile bool mb_can_evalueate_packet = false;

static mb_packet request_packet;

static uint8_t rx_buffer_index;
static uint8_t rx_buffer[MB_MAX_PACKET_LEN];

void mb_init() {
	mb_can_evalueate_packet = false;

	rx_buffer_index = 0;
	memset(rx_buffer, 0, sizeof(rx_buffer));

	request_packet.address = 0x00;
	request_packet.function = 0x00;
	request_packet.data_len = 0x00;
	memset(request_packet.data, 0, sizeof(request_packet.data));

	LL_USART_EnableIT_RXNE(USART1);
}

bool mb_crc_is_ok() {
	if (rx_buffer_index < MB_MIN_PACKET_LEN) {
		return false;
	}

	// get crc from the transmitted packet
	uint8_t crc_rx_low = rx_buffer[rx_buffer_index-2];
	uint8_t crc_rx_high = rx_buffer[rx_buffer_index-1];
	uint16_t crc_16_rx = ((uint16_t)crc_rx_high << 8) | ((uint16_t)crc_rx_low);

	uint16_t crc_16_calc = crc16(rx_buffer, rx_buffer_index-2);

	return crc_16_rx == crc_16_calc;
}

mb_packet* mb_packet_eval() {
	if (rx_buffer[0] == MB_SLAVE_ADDRESS) { // received packet for my address
		request_packet.address = rx_buffer[0];
		request_packet.function = rx_buffer[1];
		request_packet.data_len = rx_buffer_index - 4;
		memcpy(request_packet.data, &rx_buffer[2], request_packet.data_len);

		// clean
		mb_can_evalueate_packet = false;
		rx_buffer_index = 0;
		memset(rx_buffer, 0, sizeof(rx_buffer));

		return &request_packet;
	} else { // clean
		mb_can_evalueate_packet = false;
		rx_buffer_index = 0;
		memset(rx_buffer, 0, sizeof(rx_buffer));

		return NULL;
	}
}

void mb_packet_transmit(mb_packet *packet) {
	for (uint8_t i = 0; i < packet->data_len; i++) {
		while(!LL_USART_IsActiveFlag_TXE(USART1));
		LL_USART_TransmitData8(USART1, packet->data[i]);
	}
}

void mb_handle_irq(void)
{
	static bool is_first_byte = true; // workaround for always receiving an empty byte for now reason after reset
	if (is_first_byte) {
		is_first_byte = false;
		(void)LL_USART_ReceiveData8(USART1);
		return;
	}

	if (
		LL_USART_IsActiveFlag_RXNE(USART1) &&
		LL_USART_IsEnabledIT_RXNE(USART1)
	) {
		if (rx_buffer_index < sizeof(rx_buffer)) {
			rx_buffer[rx_buffer_index] = LL_USART_ReceiveData8(USART1);
			rx_buffer_index++;
		}

		if (rx_buffer_index >= MB_MIN_PACKET_LEN) {
			mb_can_evalueate_packet = true;
		}
	}
}
