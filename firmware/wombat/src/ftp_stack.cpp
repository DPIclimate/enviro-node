#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"
#include "sd-card/interface.h"

#define TAG "ftp_stack"

#define MAX_BUF 2048
static char buf[MAX_BUF + 1];

static int last_cmd = -1;
static int last_result = -1;
static volatile bool got_urc = false;
static bool login_ok = false;
static bool logout_ok = false;
static bool change_dir_ok = false;
static bool mkdir_ok = false;
static bool ftp_file_upload_ok = false;
static bool file_dwnd_ok = false;

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

    }

    got_urc = true;
}

bool ftp_login(void) {
    ESP_LOGI(TAG, "Starting modem and login");

    DeviceConfig &config = DeviceConfig::get();
    std::string &host = config.getFtpHost();
    std::string &user = config.getFtpUser();
    std::string &password = config.getFtpPassword();

    SARA_R5_error_t err = r5.setFTPserver(host.c_str());
    if (err) {
        ESP_LOGW(TAG, "Failed to set FTP server hostname");
        return false;
    }
    delay(20);

    err = r5.setFTPcredentials(user.c_str(), password.c_str());
    if (err) {
        ESP_LOGW(TAG, "Failed to set FTP credentials");
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
    while ( ! got_urc) {
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
    last_cmd = -1;
    got_urc = false;
    while (last_cmd != SARA_R5_FTP_COMMAND_GET_FILE && ( ! got_urc)) {
        r5.bufferedPoll();
        delay(100);
    }

    ESP_LOGI(TAG, "FTP download URC result: %d", last_result);
    return last_result == 1;
}
bool ftp_create_remote_dir(){
    DeviceConfig &config = DeviceConfig::get();

    snprintf(g_buffer, MAX_G_BUFFER, "%s/node_%s",ftp_file_upload_dir, config.node_id);
    ESP_LOGI(TAG, "path to create %s", g_buffer);

    SARA_R5_error_t err = r5.ftpCreateDirectory(g_buffer);
    if (err != SARA_R5_ERROR_SUCCESS){
        ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
        return false;
    }

    got_urc = false;
    while ( ! got_urc) {
        r5.bufferedPoll();
        delay(20);
    }

    if(!mkdir_ok){
        ESP_LOGE(TAG, "Failed to create directory for file upload");
        return false;
    }

    return true;
}

bool ftp_change_dir(){

    DeviceConfig &config = DeviceConfig::get();
    snprintf(g_buffer, MAX_G_BUFFER, "%s/node_%s",ftp_file_upload_dir, config.node_id);

    //Change into directory for upload
    SARA_R5_error_t err = r5.ftpChangeWorkingDirectory(g_buffer);

    if (err != SARA_R5_ERROR_SUCCESS){
        ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
        return false;
    }

    // Wait for UFTPC URC to show connection success/failure.
    got_urc = false;
    while ( ! got_urc) {
        r5.bufferedPoll();
        delay(20);
    }

    if(!change_dir_ok){
        ESP_LOGE(TAG, "Change directory failed");
        return false;
    }

    return true;
}

bool ftp_upload_file() {

    //Create unique directory on ftp server for sd card files
    DeviceConfig &config = DeviceConfig::get();

    //TODO check if file already exists on ftp server
//    bool dir_created=ftp_create_remote_dir();
//    if (!dir_created){
//        ESP_LOGE(TAG, "Could not create remote dir");
//        return false;
//    }
    const String& filename = "data2.json";
    bool dir_change = ftp_change_dir();
    if (!dir_change) {
        ESP_LOGE(TAG, "Could not change dir");
        return false;
    }

    //Copy chunk to sara filesytem
    const String& path_name = "/" + filename;
    int file_size = SDCardInterface::get_file_size(path_name.c_str());

    //Size of each file to be uploaded to ftp server, modem file size is 900 0000 bytes
    const int CHUNK_SIZE = 100000;

    //Number of chunks SD card file needs to be split into
    int num_chunks = std::ceil(file_size / CHUNK_SIZE);
    ESP_LOGI(TAG, "Size of file to upload is %d, will be split into %d chunks", file_size, num_chunks+1);

    uint8_t filename_size = 30;
    char chnk_filename[filename_size]; //Filename for chunk

    int file_position = 0;
    for (int i=0; i <= num_chunks; i++) {

        int bytes_read_chnk = 0;//Number of bytes read into the current chunk

        snprintf(chnk_filename, filename_size, "%s_chunk_%d_%s", config.node_id, i, filename.c_str());

        while ((bytes_read_chnk < CHUNK_SIZE) && (file_position < file_size)) {

            uint32_t bytes_to_read = MAX_G_BUFFER;
            if (CHUNK_SIZE-bytes_read_chnk < MAX_G_BUFFER){
                bytes_to_read = CHUNK_SIZE - bytes_read_chnk;
            }

            ESP_LOGI(TAG, "Writing file %s, size to put into buffer %d", chnk_filename, bytes_to_read);

            int bytes_read = SDCardInterface::read_file(path_name.c_str(), g_buffer, bytes_to_read, file_position);
            if (bytes_read == -1){
                ESP_LOGE(TAG, "File bytes_read failed");
                return false;
            }

            SARA_R5_error_t err = r5.appendFileContents(chnk_filename, g_buffer, bytes_read);
            if (err != SARA_R5_ERROR_SUCCESS){
                ESP_LOGE(TAG, "Error: %d", err);
            }

            file_position+=bytes_read;
            bytes_read_chnk+=bytes_read;

            ESP_LOGI(TAG, "Bytes written %d, bytes written in chunk %d", file_position, bytes_read_chnk);
        }

        ESP_LOGI(TAG, "Written chunk %s, now uploading", chnk_filename);
        //Upload chunk to ftp server

        uint8_t retry = 0;
        while (true){

            if (retry > 3){
                ESP_LOGE(TAG, "Upload failed, giving up");
                r5.deleteFile(chnk_filename);
                return false;
            }

            SARA_R5_error_t err = r5.ftpPutFile(chnk_filename, chnk_filename);
            if (err == SARA_R5_ERROR_SUCCESS){
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

        if(! ftp_file_upload_ok){
            ESP_LOGE(TAG, "Upload failed");
            r5.deleteFile(chnk_filename);
        }

        ESP_LOGI(TAG, "Uploaded chunk to ftp, deleting chunk from fs");
        //Delete chunk from modem file system
        SARA_R5_error_t err = r5.deleteFile(chnk_filename);
        if (err != SARA_R5_ERROR_SUCCESS){
            ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
            return false;
        }

    }

    return true;
}



