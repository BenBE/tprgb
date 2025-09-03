#include "boot_main.h"

#include "modbus.h"


volatile mb_packet *mbp;

int boot_main(void) {
    mb_init();

    while(1) {
        if (mb_can_evalueate_packet) {
            if (mb_crc_is_ok()) {
                mbp = mb_packet_eval();
            }
        }
    }

    return 0;
}
