
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

static volatile int lastCmd = -1;
static volatile int lastResult = -1;
static volatile bool ftpGotURC = false;
static volatile bool ftpLoginOk = false;
static volatile bool ftpLogoutOk = false;

static void ftp_cmd_callback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    lastCmd = cmd;
    lastResult = result;
    if (result == 0) {
        int e1, e2;
        r5.getFTPprotocolError(&e1, &e2);
        ESP_LOGE(TAG, "FTP op failed: %d, %d", e1, e2);
    }

    switch (cmd) {
        case SARA_R5_FTP_COMMAND_LOGOUT:
            ftpLogoutOk = (result == 1);
            break;
        case SARA_R5_FTP_COMMAND_LOGIN:
            ftpLoginOk = (result == 1);
            break;
    }

    ftpGotURC = true;
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
    ftpGotURC = false;
    while ( ! ftpGotURC) {
        r5.bufferedPoll();
        delay(20);
    }

    if ( ! ftpLoginOk) {
        ESP_LOGE(TAG, "Connection or ftp_login to FTP server failed");
        return false;
    }

    ESP_LOGI(TAG, "Connected to FTP server");

    return true;
}

bool ftp_logout(void) {
    r5.disconnectFTP();
    delay(20);

    ftpGotURC = false;
    while (!ftpGotURC) {
        r5.bufferedPoll();
    }
    return ftpLogoutOk;
}

bool ftp_get(const char * filename) {
    SARA_R5_error_t err = r5.ftpGetFile(filename);
    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Error returned from R5 stack: %d", err);
        return false;
    }

    if ( ! SPIFFS.begin()) {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
        return false;
    }

    constexpr size_t MAX_FNAME = 32;
    static char spi_filename[MAX_FNAME + 1];
    snprintf(spi_filename, MAX_FNAME, "/%s", filename);
    File f = SPIFFS.open(spi_filename, FILE_WRITE);

    constexpr size_t max_buf = 256;
    uint8_t buf[max_buf];

    while (true) {
        if (waitForChar(LTE_Serial, 6000) < 1) {
            break;
        }

        int i = 0;
        while (i < max_buf) {
            int ch = LTE_Serial.read();
            if (ch < 0) {
                break;
            }

            buf[i++] = ch & 0xFF;
        }

        f.write(buf, i);
        if (i < max_buf) {
            break;
        }
    }

    f.close();

    ESP_LOGI(TAG, "File as written to SPIFFS");
    f = SPIFFS.open(spi_filename, FILE_READ);
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
    SPIFFS.end();

    ESP_LOGI(TAG, "End of file as written to SPIFFS");
    return true;
}
