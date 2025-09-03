#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>


typedef struct fw_info {
    const char magic[4];    // Magic identifier ("TPFW")
    const char *vendor;
    const char *product;
    const char *version;
    const char *build_date;
    const uint32_t flags;
    const void *data;
} fw_info_t;

extern const fw_info_t FW_Info;

extern const char FW_Info_BuildDate[];
extern const char FW_Info_Version[];
extern const char FW_Info_Vendor[];
extern const char FW_Info_Product[];
extern const char FW_Info_Homepage[];

#ifdef __cplusplus
}
#endif
