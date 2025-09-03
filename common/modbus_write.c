#include "modbus.h"

#include "stm32f0xx_ll_usart.h"

#include "modbus_crc.h"


static void mb_send_buffer(uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        while(!LL_USART_IsActiveFlag_TXE(USART1));
        LL_USART_TransmitData8(USART1, buf[i]);
    }
}

static void mb_send_error(uint8_t address, uint8_t function, uint8_t exception_code) {
    uint8_t buf[5] = {
        address,
        function | 0x80,
        exception_code,
    };

    uint16_t crc = crc16(buf, 3);
    buf[3] = (crc >> 0) & 0xFF;
    buf[4] = (crc >> 8) & 0xFF;

    mb_send_buffer(buf, sizeof(buf));
}

static void mb_send_packet(mb_packet *packet) {
    uint8_t* buf = (uint8_t*)packet;
    uint16_t len = packet->data_len + (packet->data - buf);

    if (sizeof(mb_packet) < len + 2) {
        // Packet too large to send
        return;
    }

    uint16_t crc = crc16(buf, len);
    buf[len++] = (crc >> 0) & 0xFF;
    buf[len++] = (crc >> 8) & 0xFF;

    mb_send_buffer(buf, len);
}

void mb_packet_handle_default(mb_packet *packet) {
    if (packet->address != MB_SLAVE_ADDRESS) {
        return;
    }

    switch (packet->function) {
        default:
            // Unsupported function code
            packet->function |= 0x80; // Set exception bit
            packet->data[0] = MB_EX_ILLEGAL_FUNCTION;
            packet->data_len = 1;
            mb_send_packet(packet);
            break;
    }
}
