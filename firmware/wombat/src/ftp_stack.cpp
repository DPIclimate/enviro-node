#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"
#include "sd-card/interface.h"
#include "Utils.h"

#define TAG "ftp_stack"

static CommandURCVector<SARA_R5_ftp_command_opcode_t> urcs;

static void ftp_cmd_callback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    log_to_sdcardf("ftp cb cmd: %d, result: %d", cmd, result);

    urcs.addPair(static_cast<SARA_R5_ftp_command_opcode_t>(cmd), result);

    if (result == 0) {
        int e1, e2;
        r5.getFTPprotocolError(&e1, &e2);
        urcs.back().err1 = e1;
        urcs.back().err2 = e2;

        ESP_LOGE(TAG, "FTP op failed: %d, %d", e1, e2);
    }
}

[[nodiscard]]
bool ftp_login(void) {
    log_to_sdcard("ftp login");
    ESP_LOGI(TAG, "Starting modem and login");

    if ( ! connect_to_internet()) {
      ESP_LOGE(TAG, "[E] Could not connect to internet");
      return false;
    }

    DeviceConfig &config = DeviceConfig::get();
    std::string &host = config.getFtpHost();
    std::string &user = config.getFtpUser();
    std::string &password = config.getFtpPassword();

    SARA_R5_error_t err = r5.setFTPserver(host.c_str());
    if (err) {
        ESP_LOGE(TAG, "Failed to set FTP server hostname");
        log_to_sdcard("[E] Failed to set FTP server hostname");
        return false;
    }
    delay(20);

    err = r5.setFTPcredentials(user.c_str(), password.c_str());
    if (err) {
        ESP_LOGE(TAG, "Failed to set FTP credentials");
        log_to_sdcard("Failed to set FTP credentials");
        return false;
    }
    delay(20);

    if (r5.setFTPtimeouts(180, 120, 120)) {
        ESP_LOGW(TAG, "Setting FTP timeouts failed.");
        log_to_sdcard("Setting FTP timeouts failed.");
    }
    delay(20);

    r5.setFTPCommandCallback(ftp_cmd_callback);

    if (r5.connectFTP() != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Connection or ftp_login to FTP server failed");
        log_to_sdcard("Connection or ftp_login to FTP server failed");
        return false;
    }
    delay(20);

    log_to_sdcard("Waiting for mqtt login URC");
    int result = -1;
    bool found_urc = urcs.waitForURC(SARA_R5_FTP_COMMAND_LOGIN, &result, 30, 500);
    if (!found_urc || result != 1) {
        ESP_LOGE(TAG, "Connection or ftp_login to FTP server failed");
        log_to_sdcard("Connection or ftp_login to FTP server failed");
        return false;
    }

    ESP_LOGI(TAG, "Connected to FTP server");
    log_to_sdcard("Connected to FTP server");
    return true;
}

bool ftp_logout(void) {
    log_to_sdcard("ftp logout");

    r5.disconnectFTP();
    delay(20);

    int result = -1;
    urcs.waitForURC(SARA_R5_FTP_COMMAND_LOGOUT, &result, 30, 200);

    return result == 1;
}

[[nodiscard]]
bool ftp_get(const char * filename) {
    log_to_sdcard("ftp get");

    SARA_R5_error_t err = r5.ftpGetFile(filename);
    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "ftp get error returned from R5 stack: %d", err);
        log_to_sdcardf("ftp get error returned from R5 stack: %d", err);
        return false;
    }

    ESP_LOGI(TAG, "Waiting for FTP download URC");
    log_to_sdcard("Waiting for FTP download URC");

    // Wait for up to 10 minutes - 300 intervals of 2 seconds.
    int result = -1;
    urcs.waitForURC(SARA_R5_FTP_COMMAND_GET_FILE, &result, 300, 2000);

    return result == 1;
}

[[nodiscard]]
bool ftp_create_remote_dir() {
    log_to_sdcard("ftp mkdir");

    DeviceConfig &config = DeviceConfig::get();

    snprintf(g_buffer, MAX_G_BUFFER, "%s/node_%s",ftp_file_upload_dir, config.node_id);
    ESP_LOGI(TAG, "path to create %s", g_buffer);

    SARA_R5_error_t err = r5.ftpCreateDirectory(g_buffer);
    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "ftp create dir error returned from R5 stack: %d", err);
        log_to_sdcardf("ftp create dir error returned from R5 stack: %d", err);
        return false;
    }

    int result = -1;
    urcs.waitForURC(SARA_R5_FTP_COMMAND_MKDIR, &result, 30, 1000);

    if (result != 1) {
        ESP_LOGE(TAG, "Failed to create directory for file upload");
    }

    return result == 1;
}

bool ftp_change_dir() {
    DeviceConfig &config = DeviceConfig::get();
    snprintf(g_buffer, MAX_G_BUFFER, "%s/node_%s",ftp_file_upload_dir, config.node_id);

    log_to_sdcardf("ftp chdir %s", g_buffer);

    //Change into directory for upload
    SARA_R5_error_t err = r5.ftpChangeWorkingDirectory(g_buffer);

    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Error returned from R5 stack in change_dir: %d", err);
        log_to_sdcardf("ftp chdir failed: %d", err);
        return false;
    }

    int result = -1;
    urcs.waitForURC(SARA_R5_FTP_COMMAND_CHANGE_DIR, &result, 30, 1000);

    if (result != 1) {
        ESP_LOGE(TAG, "Change directory failed");
    }

    return result == 1;
}

[[nodiscard]]
bool ftp_upload_file(const String& filename) {
    DeviceConfig &config = DeviceConfig::get();

    log_to_sdcard("ftp upload");

    const String path_name = "/" + filename;
    size_t file_size = SDCardInterface::get_file_size(path_name.c_str());
    if (file_size == 0) {
        ESP_LOGE(TAG, "Could not find file on SD card, or file does not contain any contents");
        return false;
    }

    //Size of each file to be uploaded to ftp server
    //Calculate the largest possible chunk based of the remaining space on the modem
    size_t size;
    r5.getAvailableSize(&size);
    //log_to_sdcardf("ftp upload space on r5 fs: %lu", size);
    delay(20);

    if (size < MAX_G_BUFFER) {
        ESP_LOGE(TAG, "Not enough space on modem filesystem");
        log_to_sdcard("[E] ftp upload not enough space on modem filesystem");
        return false;
    }

    size = size / 3;
    size_t CHUNK_SIZE = size; // Allow for space remaining on the modem
    if (CHUNK_SIZE < MAX_G_BUFFER) {
        ESP_LOGE(TAG, "Not enough space on modem filesystem");
        log_to_sdcard("[E] ftp upload not enough space on modem filesystem");
        return false;
    }

    ESP_LOGI(TAG, "Each block with be of size %zu", CHUNK_SIZE);

    size_t num_chunks = std::ceil(file_size / CHUNK_SIZE);
    ESP_LOGI(TAG, "Size of file to upload is %zu, will be split into %zu chunks", file_size, num_chunks + 1);

    //Create unique directory on ftp server for sd card files
    bool dir_created = ftp_create_remote_dir();
    if (!dir_created) {
        // Don't return as dir may not have been created because it already exists
        ESP_LOGW(TAG, "Could not create remote dir, it may already exist");
    }

    bool dir_change = ftp_change_dir();
    if (!dir_change) {
        ESP_LOGE(TAG, "Could not change to dir");
        log_to_sdcard("[E] ftp upload could not change to dir");
        return false;
    }

    static const size_t filename_size = 50;
    char chunk_filename[filename_size + 1]; // Filename for chunk

    size_t file_position = 0;
    for (int i = 0; i <= num_chunks; i++) {
        size_t bytes_read_chnk = 0; // Number of bytes read into the current chunk

        snprintf(chunk_filename, filename_size, "%s_%05d", filename.c_str(), i);

        while ((bytes_read_chnk < CHUNK_SIZE) && (file_position < file_size)) {
            size_t bytes_to_read = MAX_G_BUFFER;
            if (CHUNK_SIZE - bytes_read_chnk < MAX_G_BUFFER) {
                bytes_to_read = CHUNK_SIZE - bytes_read_chnk;
            }

            size_t bytes_read = SDCardInterface::read_file(path_name.c_str(), g_buffer, bytes_to_read, file_position);
            if (bytes_read == 0) {
                ESP_LOGE(TAG, "File bytes_read failed");
                log_to_sdcard("[E] ftp upload failing a");
                r5.deleteFile(chunk_filename);
                return false;
            }

            int bytes_to_write = static_cast<int>(bytes_read);
            SARA_R5_error_t err = r5.appendFileContents(chunk_filename, g_buffer, bytes_to_write);
            delay(20);
            if (err != SARA_R5_ERROR_SUCCESS) {
                ESP_LOGE(TAG, "Append to chunk file failed, error: %d", err);
                log_to_sdcard("[E] ftp upload failing b");
                r5.deleteFile(chunk_filename);
                return false;
            }

            file_position += bytes_read;
            bytes_read_chnk += bytes_read;

            ESP_LOGI(TAG, "Bytes written %d, bytes written in chunk %d", file_position, bytes_read_chnk);
        }

        ESP_LOGI(TAG, "Written chunk %s, now uploading", chunk_filename);

        //Upload chunk to ftp server
        uint8_t retry = 0;
        while (true) {
            if (retry > 3) {
                ESP_LOGE(TAG, "Upload failed, giving up");
                log_to_sdcard("[E] ftp upload failing c");
                r5.deleteFile(chunk_filename);
                delay(20);
                return false;
            }

            SARA_R5_error_t err = r5.ftpPutFile(chunk_filename, chunk_filename);
            delay(20);
            if (err == SARA_R5_ERROR_SUCCESS) {
                break;
            }

            retry++;
            ESP_LOGE(TAG, "Upload command failed, retrying");
            log_to_sdcard("[W] Upload command failed, retrying");
        }

        int result = -1;
        urcs.waitForURC(SARA_R5_FTP_COMMAND_PUT_FILE, &result, 300, 2000);
        if (result == 1) {
            ESP_LOGI(TAG, "Uploaded chunk to ftp");
            log_to_sdcard("Uploaded chunk to ftp");
        } else {
            ESP_LOGE(TAG, "Upload failed");
            log_to_sdcard("[E] ftp upload failing d");
        }

        SARA_R5_error_t err = r5.deleteFile(chunk_filename);
        delay(20);
        if (err != SARA_R5_ERROR_SUCCESS) {
            ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
            log_to_sdcardf("[E] Error returned from R5 stack: %d", err);
            return false;
        }
    }

    log_to_sdcard("ftp upload ok");
    return true;
}
