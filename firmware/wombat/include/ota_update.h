#ifndef WOMBAT_OTA_UPDATE_H
#define WOMBAT_OTA_UPDATE_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint16_t new_major;
    uint16_t new_minor;
    uint16_t new_update;
    size_t file_len;
    char file_hash[41];
    char git_commit_id[41];
} ota_firmware_info_t;

int ota_check_for_update(ota_firmware_info_t& ota_ctx);
bool ota_download_update(const ota_firmware_info_t& ota_ctx);
void back_to_factory(void);
#endif //WOMBAT_OTA_UPDATE_H
