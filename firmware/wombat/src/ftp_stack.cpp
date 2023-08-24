#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"
#include "sd-card/interface.h"
#include "Utils.h"

#define TAG "ftp_stack"

#define MAX_BUF 2048
static char buf[MAX_BUF + 1];

static int last_cmd = SARA_R5_FTP_COMMAND_INVALID;
static int last_result = -1;
static volatile bool got_urc = false;
static bool login_ok = false;
static bool logout_ok = false;
static bool change_dir_ok = false;
static bool mkdir_ok = false;
static bool ftp_file_upload_ok = false;
static bool file_dwnd_ok = false;
static bool ftp_list_ok = false;

static void ftp_cmd_callback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    last_cmd = cmd;
    last_result = result;
    if (result == 0) {
        int e1, e2;
        r5.getFTPprotocolError(&e1, &e2);
        ESP_LOGE(TAG, "FTP op failed: %d, %d", e1, e2);
    }

    switch (cmd) {
        case SARA_R5_FTP_COMMAND_LOGOUT:
            logout_ok = (result == 1);
            break;
        case SARA_R5_FTP_COMMAND_LOGIN:
            login_ok = (result == 1);
            break;
        case SARA_R5_FTP_COMMAND_CHANGE_DIR:
            change_dir_ok = (result == 1);
            break;
        case SARA_R5_FTP_COMMAND_MKDIR:
            mkdir_ok = (result==1);
            break;
        case SARA_R5_FTP_COMMAND_PUT_FILE:
            ftp_file_upload_ok = (result==1);
            break;
        case SARA_R5_FTP_COMMAND_LS:
            ftp_list_ok = (result==1);
            break;
    }

    got_urc = true;
}

bool ftp_login(void) {
    ESP_LOGI(TAG, "Starting modem and login");

    if ( ! connect_to_internet()) {
      ESP_LOGE(TAG, "Could not connect to internet");
      return false;
    }

    DeviceConfig &config = DeviceConfig::get();
    std::string &host = config.getFtpHost();
    std::string &user = config.getFtpUser();
    std::string &password = config.getFtpPassword();

    SARA_R5_error_t err = r5.setFTPserver(host.c_str());
    if (err) {
        ESP_LOGE(TAG, "Failed to set FTP server hostname");
        return false;
    }
    delay(20);

    err = r5.setFTPcredentials(user.c_str(), password.c_str());
    if (err) {
        ESP_LOGE(TAG, "Failed to set FTP credentials");
        return false;
    }
    delay(20);

    if (r5.setFTPtimeouts(180, 120, 120)) {
        ESP_LOGW(TAG, "Setting FTP timeouts failed.");
    }
    delay(20);

    r5.setFTPCommandCallback(ftp_cmd_callback);

    if (r5.connectFTP() != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Connection or ftp_login to FTP server failed");
        return false;
    }
    delay(20);

    // Wait for UFTPC URC to show connection success/failure.
    got_urc = false;
    last_cmd = SARA_R5_FTP_COMMAND_INVALID;
    login_ok = false;
    while ( ! got_urc && last_cmd != SARA_R5_FTP_COMMAND_LOGIN) {
        r5.bufferedPoll();
        delay(20);
    }

    if ( ! login_ok) {
        ESP_LOGE(TAG, "Connection or ftp_login to FTP server failed");
        return false;
    }

    ESP_LOGI(TAG, "Connected to FTP server");
    return true;
}

bool ftp_logout(void) {
    r5.disconnectFTP();
    delay(20);

    got_urc = false;
    while ( ! got_urc) {
        r5.bufferedPoll();
    }
    return logout_ok;
}

bool ftp_get(const char * filename) {
    SARA_R5_error_t err = r5.ftpGetFile(filename);
    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
        return false;
    }

    ESP_LOGI(TAG, "Waiting for FTP download URC");
    last_cmd = SARA_R5_FTP_COMMAND_INVALID;
    got_urc = false;
    while (last_cmd != SARA_R5_FTP_COMMAND_GET_FILE && ( ! got_urc)) {
        r5.bufferedPoll();
        delay(100);
    }

    ESP_LOGI(TAG, "FTP download URC result: %d", last_result);
    return last_result == 1;
}
bool ftp_create_remote_dir() {
    DeviceConfig &config = DeviceConfig::get();

    snprintf(g_buffer, MAX_G_BUFFER, "%s/node_%s",ftp_file_upload_dir, config.node_id);
    ESP_LOGI(TAG, "path to create %s", g_buffer);

    SARA_R5_error_t err = r5.ftpCreateDirectory(g_buffer);
    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
        return false;
    }

    got_urc = false;
    while ( ! got_urc) {
        r5.bufferedPoll();
        delay(20);
    }

    if (!mkdir_ok) {
        ESP_LOGE(TAG, "Failed to create directory for file upload");
        return false;
    }

    return true;
}

bool ftp_change_dir() {
    DeviceConfig &config = DeviceConfig::get();
    snprintf(g_buffer, MAX_G_BUFFER, "%s/node_%s",ftp_file_upload_dir, config.node_id);

    //Change into directory for upload
    SARA_R5_error_t err = r5.ftpChangeWorkingDirectory(g_buffer);

    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Error returned from R5 stack in change_dir: %d", err);
        return false;
    }

    // Wait for UFTPC URC to show connection success/failure.
    got_urc = false;
    while ( ! got_urc) {
        r5.bufferedPoll();
        delay(20);
    }

    if (!change_dir_ok) {
        ESP_LOGE(TAG, "Change directory failed");
        return false;
    }

    return true;
}

bool ftp_upload_file(const String& filename) {
    DeviceConfig &config = DeviceConfig::get();

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
    size_t CHUNK_SIZE = size - 1000; // Allow for space remaining on the modem
    ESP_LOGI(TAG, "Each block with be of size %uz", CHUNK_SIZE);

    size_t num_chunks = std::ceil(file_size / CHUNK_SIZE);
    ESP_LOGI(TAG, "Size of file to upload is %zu, will be split into %zu chunks", file_size, num_chunks + 1);

    //Create unique directory on ftp server for sd card files
    bool dir_created = ftp_create_remote_dir();
    if (!dir_created) {
        //Don't return as dir may not have been created because it already exists
        ESP_LOGE(TAG, "Could not create remote dir");
    }

    bool dir_change = ftp_change_dir();
    if (!dir_change) {
        ESP_LOGE(TAG, "Could not change dir");
        return false;
    }

    static const size_t filename_size = 50;
    char chunk_filename[filename_size + 1]; // Filename for chunk

    size_t file_position = 0;
    for (int i = 0; i <= num_chunks; i++) {
        size_t bytes_read_chnk = 0; // Number of bytes read into the current chunk

        snprintf(chunk_filename, filename_size, "%s_chunk_%d_%s", config.node_id, i, filename.c_str());

        while ((bytes_read_chnk < CHUNK_SIZE) && (file_position < file_size)) {

            size_t bytes_to_read = MAX_G_BUFFER;
            if (CHUNK_SIZE-bytes_read_chnk < MAX_G_BUFFER) {
                bytes_to_read = CHUNK_SIZE - bytes_read_chnk;
            }

            size_t bytes_read = SDCardInterface::read_file(path_name.c_str(), g_buffer, bytes_to_read, file_position);
            if (bytes_read == 0) {
                ESP_LOGE(TAG, "File bytes_read failed");
                return false;
            }

            SARA_R5_error_t err = r5.appendFileContents(chunk_filename, g_buffer, bytes_read);
            if (err != SARA_R5_ERROR_SUCCESS) {
                ESP_LOGE(TAG, "Error: %d", err);
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
                r5.deleteFile(chunk_filename);
                return false;
            }

            SARA_R5_error_t err = r5.ftpPutFile(chunk_filename, chunk_filename);
            if (err == SARA_R5_ERROR_SUCCESS) {
                break;
            }

            retry++;
            ESP_LOGE(TAG, "Upload failed, retrying");
        }

        got_urc = false;
        while ( ! got_urc) {
            r5.bufferedPoll();
            delay(20);
        }

        if (! ftp_file_upload_ok) {
            ESP_LOGE(TAG, "Upload failed");
            r5.deleteFile(chunk_filename);
        }

        ESP_LOGI(TAG, "Uploaded chunk to ftp, deleting chunk from fs");
        //Delete chunk from modem file system
        SARA_R5_error_t err = r5.deleteFile(chunk_filename);
        if (err != SARA_R5_ERROR_SUCCESS) {
            ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
            return false;
        }

    }

    return true;
}
