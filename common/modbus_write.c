#include "modbus.h"

#include <string.h>

#include "stm32f0xx_ll_usart.h"

#include "modbus_crc.h"
#include "version.h"


typedef struct devinfo_record {
    uint8_t object_id;
    size_t object_length;
    uint8_t *object_data;
} devinfo_record_t;

static const devinfo_record_t device_info[] = {
    {
        .object_id = 0x00, // VendorName
        .object_length = 0,
        .object_data = (uint8_t *)FW_Info_Vendor,
    },
    {
        .object_id = 0x01, // ProductCode
        .object_length = 0,
        .object_data = (uint8_t *)FW_Info_Product,
    },
    {
        .object_id = 0x02, // MajorMinorRevision
        .object_length = 0,
        .object_data = (uint8_t *)FW_Info_Version,
    },
    {
        .object_id = 0x03, // VendorUrl
        .object_length = 0,
        .object_data = (uint8_t *)FW_Info_Homepage,
    },

    // End of List
    {
        .object_id = 0,
        .object_length = 0,
        .object_data = NULL,
    },
};

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

static void mb_packet_handle_mei(mb_packet *packet) {
    if (packet->address != MB_SLAVE_ADDRESS) {
        return;
    }

    // Handle MEI (More Extended Information) function
    if (packet->function != MB_FUNC_MEI) {
        // Invalid function code for MEI
        mb_send_error(packet->address, packet->function, MB_EX_ILLEGAL_FUNCTION);
        return;
    }

    if (packet->data_len < 3) {
        // Invalid data length
        mb_send_error(packet->address, packet->function, MB_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    uint8_t mei_type = packet->data[0];
    if (mei_type != 0x0E) {
        // Unsupported MEI type
        mb_send_error(packet->address, packet->function, MB_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    // Read parameters we need to respond
    uint16_t read_device_id_code = packet->data[1];
    if (read_device_id_code < 1 || read_device_id_code > 4) {
        // Invalid Read Device ID code
        mb_send_error(packet->address, packet->function, MB_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    uint8_t max_id = 0xFF;
    if (read_device_id_code == 1) {
        max_id = 0x02; // Basic
    } else if (read_device_id_code == 2) {
        max_id = 0x2A; // Regular
    } else if (read_device_id_code == 3) {
        max_id = 0xFF; // Extended
    } // read_device_id_code == 4 -> All

    uint8_t read_object_id = packet->data[2];
    size_t device_info_index = 0;
    while (!!device_info[device_info_index].object_data) {
        if (device_info[device_info_index].object_id > max_id) {
            // Jump to the end of the list
            while(!!device_info[device_info_index].object_data) {
                device_info_index++;
            }
            break;
        }

        if (device_info[device_info_index].object_id == read_object_id) {
            break;
        }
        device_info_index++;
    }
    if (!device_info[device_info_index].object_data) {
        if (read_device_id_code == 4) {
            // No more objects
            mb_send_error(packet->address, packet->function, MB_EX_ILLEGAL_DATA_ADDRESS);
            return;
        }
        device_info_index = 0;
    }

    bool has_more = !!device_info[device_info_index+1].object_data;
    if (read_device_id_code == 4) {
        has_more = false;
    }

    size_t object_length = device_info[device_info_index].object_length;
    if (!object_length) {
        object_length = strlen((char*)device_info[device_info_index].object_data);
    }

    // Prepare response packet
    packet->data[0] = 0x0E; // MEI type
    packet->data[1] = read_device_id_code; // Read Device ID code
    packet->data[2] = 0x83; // Conformity level (Extended + Individual)
    packet->data[3] = has_more ? 0xFF : 0x00;
    packet->data[4] = has_more ? device_info[device_info_index+1].object_id : 0x00; // Next Object ID or 0
    packet->data[5] = 0x01; // Number of objects
    packet->data[6] = device_info[device_info_index].object_id; // Object ID
    packet->data[7] = object_length; // Object Length
    for (uint8_t i = 0; i < object_length; i++) {
        packet->data[8 + i] = device_info[device_info_index].object_data[i];
    }
    packet->data_len = 8 + object_length;
    mb_send_packet(packet);
}

void mb_packet_handle_default(mb_packet *packet) {
    if (packet->address != MB_SLAVE_ADDRESS) {
        return;
    }

    switch (packet->function) {
        case MB_FUNC_MEI:
            // Handle MEI function
            mb_packet_handle_mei(packet);
            break;
        default:
            // Unsupported function code
            packet->function |= 0x80; // Set exception bit
            packet->data[0] = MB_EX_ILLEGAL_FUNCTION;
            packet->data_len = 1;
            mb_send_packet(packet);
            break;
    }
}
