#include "ota_update.h"
#include "ftp_stack.h"
#include "globals.h"

#include <mbedtls/sha1.h>
#include <esp_ota_ops.h>
#include <SPIFFS.h>

#define TAG "ota_update"

static const char* wombat_sha1 = "wombat.sha1";
static const char* wombat_bin = "wombat.bin";

/**
 * Check if the server has a newer version of the firmware than is running on the Wombat.
 *
 * @param ota_ctx a reference to an OTA firmware info structure that carries info about the firmware on the server.
 * @return -1 if retrieving wombat.sha1 fails, 0 if the running version is at least as new as the version on the
 * server, 1 if the server as a new version.
 */
int ota_check_for_update(ota_firmware_info_t& ota_ctx) {
    if (!ftp_get(wombat_sha1)) {
        ESP_LOGW(TAG, "FTP get wombat.sha1 failed");
        return -1;
    }

    int i_file_len = 0;
    if (r5.getFileSize(wombat_sha1, &i_file_len) || r5.getFileContents(wombat_sha1, g_buffer)) {
        ESP_LOGW(TAG, "Could not read wombat.sha1");
        return -1;
    }

    g_buffer[i_file_len] = 0;

    memset(ota_ctx.file_hash, 0, sizeof(ota_ctx.file_hash));
    memset(ota_ctx.git_commit_id, 0, sizeof(ota_ctx.git_commit_id));

    int s_rc = sscanf(g_buffer, "%u.%u.%u %lu %40s %40s", &ota_ctx.new_major, &ota_ctx.new_minor, &ota_ctx.new_update, &ota_ctx.file_len, &ota_ctx.file_hash, &ota_ctx.git_commit_id);
    if (s_rc != 6) {
        ESP_LOGE(TAG, "Failed to parse version information");
        return -1;
    }

    ESP_LOGI(TAG, "Version on server: %u.%u.%u, size = %lu, commit: %s (s_rc = %d)", ota_ctx.new_major, ota_ctx.new_minor, ota_ctx.new_update, ota_ctx.file_len, ota_ctx.git_commit_id, s_rc);

    if (ota_ctx.new_major < ver_major) {
        return 0;
    }

    if (ota_ctx.new_major > ver_major) {
        return 1;
    }

    // Major version is the same.
    if (ota_ctx.new_minor < ver_minor) {
        return 0;
    }

    if (ota_ctx.new_minor > ver_minor) {
        return 1;
    }

    // Minor version is the same.
    if (ota_ctx.new_update > ver_update) {
        return 1;
    }

    return 0;
}

bool ota_download_update(const ota_firmware_info_t& ota_ctx) {
    if (ota_ctx.file_len < 1) {
        ESP_LOGI(TAG, "Invalid arguments");
        return false;
    }

    r5.deleteFile(wombat_bin);
    if (!ftp_get(wombat_bin)) {
        ESP_LOGW(TAG, "FTP get wombat.bin failed");
        return false;
    }

    int bin_size;
    r5.getFileSize(wombat_bin, &bin_size);
    if (bin_size != ota_ctx.file_len) {
        ESP_LOGW(TAG, "wombat.bin wrong size");
        return false;
    }

    ESP_LOGI(TAG, "Next OTA update partition");

    const esp_partition_t* p_type = esp_ota_get_next_update_partition(nullptr);
    ESP_LOGI(TAG, "%d/%d %lx %lx %s", p_type->type, p_type->subtype, p_type->address, p_type->size, p_type->label);

    esp_ota_handle_t ota_handle;
    esp_err_t esp_err = esp_ota_begin(p_type,ota_ctx.file_len, &ota_handle);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %d", esp_err);
        return false;
    }

    mbedtls_sha1_context sha1_ctx;

    mbedtls_sha1_init(&sha1_ctx);
    mbedtls_sha1_starts_ret(&sha1_ctx);

    size_t bytes_read = 0;
    size_t offset = 0;
    while (offset < ota_ctx.file_len) {
        SARA_R5_error_t err = r5.getFileBlock("wombat.bin", g_buffer, offset, 64000, bytes_read);
        if (err != SARA_R5_ERROR_SUCCESS) {
            break;
        }

        ESP_LOGI(TAG, "Read %zu bytes: %02X %02X ... %02X %02X", bytes_read, g_buffer[0], g_buffer[1],
                 g_buffer[bytes_read - 2], g_buffer[bytes_read - 1]);
        offset += bytes_read;

        mbedtls_sha1_update_ret(&sha1_ctx, reinterpret_cast<const unsigned char *>(g_buffer), bytes_read);

        ESP_LOGI(TAG, "Writing firmware chunk to OTA partition");
        esp_err = esp_ota_write(ota_handle, static_cast<const void*>(g_buffer), bytes_read);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %d", esp_err);
            break;
        }
    }

    unsigned char sha1_buf[20];
    mbedtls_sha1_finish_ret(&sha1_ctx, sha1_buf);
    mbedtls_sha1_free(&sha1_ctx);

    if (esp_err == ESP_OK) {
        esp_err = esp_ota_end(ota_handle);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed writing firmware to OTA partition");
            return false;
        }
    }

    if (offset != ota_ctx.file_len) {
        ESP_LOGE(TAG, "Download failed wrong file size");
        return false;
    }

    char sha1_text[41];
    size_t i = 0;
    for (unsigned char c : sha1_buf) {
        sprintf(&sha1_text[i], "%02x", c);
        i += 2;
    }
    sha1_text[i] = 0;

    ESP_LOGI(TAG, "Hash check: reference vs downloaded");
    ESP_LOGI(TAG, "%s : %s", ota_ctx.file_hash, sha1_text);

    if (strncmp(ota_ctx.file_hash, sha1_text, sizeof(sha1_text)) != 0) {
        ESP_LOGE(TAG, "Download failed hashes do not match");
        return false;
    }

    ESP_LOGI(TAG, "Switching boot partition to %s", p_type->label);
    esp_err = esp_ota_set_boot_partition(p_type);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %d", esp_err);
        return false;
    }

    if (spiffs_ok) {
        File f = SPIFFS.open(send_fw_version_name, FILE_WRITE);
        f.write('T');
        f.close();
    }

    return true;
}

//***********************************************************************************************
//                                B A C K T O F A C T O R Y                                     *
//***********************************************************************************************
// Return to factory version.                                                                   *
// This will set the otadata to boot from the factory image, ignoring previous OTA updates.     *
//                                                                                              *
// From:https://www.esp32.com/viewtopic.php?t=4210                                              *
//***********************************************************************************************
void back_to_factory(void) {
    esp_partition_iterator_t  pi ;                                  // Iterator for find
    const esp_partition_t*    factory ;                             // Factory partition
    esp_err_t                 err ;

    pi = esp_partition_find ( ESP_PARTITION_TYPE_APP,               // Get partition iterator for
                              ESP_PARTITION_SUBTYPE_APP_FACTORY,    // factory partition
                              "factory" ) ;
    if (pi == nullptr) {                                            // Check result
        ESP_LOGE (TAG, "Failed to find factory partition") ;
    } else {
        factory = esp_partition_get (pi) ;                          // Get partition struct
        esp_partition_iterator_release (pi) ;                       // Release the iterator
        err = esp_ota_set_boot_partition (factory) ;                // Set partition for boot
        if (err != ESP_OK ) {                                       // Check error
            ESP_LOGE (TAG, "Failed to set boot partition") ;
        }
    }
}
