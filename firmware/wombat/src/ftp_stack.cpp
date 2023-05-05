
#include <SPIFFS.h>
#include "CAT_M1.h"
#include "Utils.h"
#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"
#include "cli/CLI.h"

#define TAG "ftp_stack"

#define MAX_RSP 64
static char rsp[MAX_RSP + 1];

#define MAX_BUF 2048
static char buf[MAX_BUF + 1];

static int last_cmd = -1;
static int last_result = -1;
static volatile bool got_urc = false;
static bool login_ok = false;
static bool logout_ok = false;

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
    got_urc = false;
    while ( ! got_urc) {
        r5.bufferedPoll();
        delay(100);
    }

    ESP_LOGI(TAG, "FTP download URC result: %d", last_result);
    return last_result == 1;
}
