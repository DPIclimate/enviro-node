/**
 * \file Utils.cpp
 * \brief Global utility functions.
*/

#include <Arduino.h>
#include "Utils.h"

#include <SPIFFS.h>

#include "DeviceConfig.h"
#include "TCA9534.h"
#include "CAT_M1.h"
#include "globals.h"
#include "sd-card/interface.h"

#define TAG "utils"

bool getNTPTime(SARA_R5 &r5);

/**
 * @brief Read characters from stream into buffer until the delim char is encountered.
 *
 * This function will respect the backspace character (0x08) by moving the 'virtual cursor' back
 * one character in buffer, if buffer is not empty.
 *
 * Up to max - 1 characters will be read to ensure the resulting string is null terminated.
 *
 * @param stream the Stream to read from.
 * @param delim the character that causes reading to stop and the function to return.
 * @param buffer the buffer to write characters into.
 * @param max the maximum number of characters to be read.
 * @return the length of the string read from stream.
 */
size_t readFromStreamUntil(Stream& stream, const char delim, char * const buffer, const size_t max) {
    char ch;
    size_t len = 0;

    if (buffer == nullptr || max < 1 || delim == 0) {
        return 0;
    }

    size_t actual_max = max - 1;
    buffer[0] = 0;
    while (len < actual_max) {
        while ( ! stream.available()) {
            yield();
        }

        while (stream.available()) {
            ch = stream.read();

            stream.write(ch);
            if (ch == delim) {
                return len;
            }

            // Handle backspaces.
            if (ch == 8 && len > 0) {
                buffer[len] = 0;
                len--;
                continue;
            }

            buffer[len] = ch;
            len++;
            buffer[len] = 0;
        }
    }

    return len;
}

void streamPassthrough(Stream* s1, Stream* s2) {
    int ch;
    char cmd[32];

    while (true) {
        if (s1->available()) {
            ch = s1->peek();
            if (ch == 0x04) {
                return;
            }
            memset(cmd, 0, sizeof(cmd));
            readFromStreamUntil(*s1, '\n', cmd, sizeof(cmd));
            wombat::stripWS(cmd);
            if (strlen(cmd) > 0) {
                s2->write(ch);
            }
        }
        if (s2->available()) {
            ch = s2->read();
            if (ch == 0x04) {
                return;
            }
            s1->write(ch);
        }

        yield();
    }
}

/**
 * @brief Wait up to timeout ms for any character to appear in stream.
 *
 * @param stream the Stream to read from.
 * @param timeout the number of ms to wait for a character.
 * @return the character read from stream, or -1 if the timeout occurred.
 */
int waitForChar(Stream& stream, uint32_t timeout) {
    const uint32_t start = millis();
    int a = stream.available();
    while (a < 1 && (millis() - start) < timeout) {
        delay(1); // Let the MCU do something else.
        a = stream.available();
    }

    //const uint32_t end = millis();
    //ESP_LOGD(TAG, "Delta from start of read: %lu ms, UART available = %d", (end - start), a);

    return a > 0 ? a : -1;
}

JsonObjectConst getSensorDefn(const char* const vendor, const char* const model) {
    const JsonDocument& sdi12Defns = DeviceConfig::get().getSDI12Defns();
    JsonObjectConst obj = sdi12Defns[vendor][model];
    return obj;
}

JsonObjectConst getSensorDefn(const size_t sensor_idx, const sensor_list& sensors) {
    char vendor[LEN_VENDOR+1];
    char model[LEN_MODEL+1];
    DPIClimate12::get_vendor(vendor, sensor_idx, sensors);
    DPIClimate12::get_model(model, sensor_idx, sensors);

    return getSensorDefn(vendor, model);
}

extern TCA9534 io_expander;

static bool v12_enabled = false;

/// \brief Enable the SDI-12 power line if it is not already enabled.
void enable12V(void) {
    if ( ! v12_enabled) {
        ESP_LOGI(TAG, "Enabling SDI-12 power line");
        io_expander.output(6, TCA9534::Level::H);
        delay(1000);
        v12_enabled = true;
    }
}

/// \brief Disable the SDI-12 power line.
void disable12V(void) {
    ESP_LOGI(TAG, "Disabling SDI-12 power line");
    io_expander.output(6, TCA9534::Level::L);
    v12_enabled = false;
}

// yyyy-mm-ddThh:mm:ssZ
static char iso8601_buf[24];

const char* iso8601(void) {
    memset(iso8601_buf, 0, sizeof(iso8601_buf));

    struct tm t{};
    memset(&t, 0, sizeof(t));
    time_t now;
    time(&now);
    gmtime_r(&now, &t);

    snprintf(iso8601_buf, sizeof(iso8601_buf)-1, "%04d-%02d-%02dT%02d:%02d:%02dZ", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    return iso8601_buf;
}

constexpr size_t MAX_SD_CARD_MSG = 255;
static char sd_card_msg[MAX_SD_CARD_MSG + 1];

void log_to_sdcard(const char *msg) {
    if ( ! SDCardInterface::is_ready() || msg == nullptr) {
        return;
    }

    snprintf(sd_card_msg, MAX_SD_CARD_MSG, "%s: %s\n", iso8601(), msg);
    SDCardInterface::append_to_file(sd_card_logfile_name, sd_card_msg);
}

void log_to_sdcardf(const char *fmt, ...) {
    snprintf(sd_card_msg, MAX_SD_CARD_MSG, "%s: ", iso8601());
    size_t ts_len = strnlen(sd_card_msg, MAX_SD_CARD_MSG - 1);

    va_list args;
    va_start(args, fmt);
    vsnprintf(&sd_card_msg[ts_len], MAX_SD_CARD_MSG - ts_len - 2, fmt, args);
    va_end(args);

    strcat(sd_card_msg, "\n");
    SDCardInterface::append_to_file(sd_card_logfile_name, sd_card_msg);
}

int read_spiffs_file(const char* const filename, char* buffer, const size_t max_length, size_t &bytes_read) {
    String fn_str(filename);
    if (*filename != '/') {
        fn_str = "/" + fn_str;
    }

    File file = SPIFFS.open(fn_str);
    if (file.isDirectory()) {
        ESP_LOGE(TAG, "%s is a directory", filename);
        file.close();
        return -1;
    }

    const size_t len = file.available();
    if (len > max_length) {
        ESP_LOGE(TAG, "File too long (%lu > %lu)", len, max_length);
        file.close();
        return -2;
    }

    ESP_LOGI(TAG, "Reading %lu bytes in total", len);
    bytes_read = 0;
    size_t chunk_sz = 128;
    while (bytes_read < len) {
        const size_t left = len - bytes_read;
        if (left < chunk_sz) {
            chunk_sz = left;
        }

        const size_t r_b = file.readBytes(&buffer[bytes_read], chunk_sz);
        if (r_b != chunk_sz) {
            ESP_LOGE(TAG, "Short read on SPIFFS file chunk");
            file.close();
            return -3;
        }

        bytes_read += r_b;
        delay(250);
    }

    file.close();
    return 0;
}

int read_r5_file(const String& filename, char* const buffer, const size_t length, size_t &bytes_read, SARA_R5_error_t& r5_err) {
    bytes_read = 0;
    size_t chunk_sz = 128;
    r5_err = SARA_R5_ERROR_SUCCESS;
    while (bytes_read < length) {
        const size_t left = length - bytes_read;
        if (left < chunk_sz) {
            chunk_sz = left;
        }

        size_t r_b;
        r5_err = r5.getFileBlock(filename, &buffer[bytes_read], bytes_read, chunk_sz, r_b);
        if (r5_err) {
            return -1;
        }

        if (r_b != chunk_sz) {
            return -3;
        }

        bytes_read += r_b;
        delay(100);
        r5.bufferedPoll();
    }

    return 0;
}

/**
 * @brief Waits until an AT command to the R5 modem returns OK, or a timeout is reached.
 *
 * This function will try 5 times, pausing for 1 second between attempts.
 *
 * This function uses g_buffer.

 * @return true if the modem responds, otherwise false.
 */
bool wait_for_at(void) {
    int attempts = 5;

    while (attempts > 0) {
        LTE_Serial.write("AT\r");
        delay(100);
        g_buffer[0] = 0;
        const char *rsp = g_buffer;
        if (LTE_Serial.available()) {
            int i = 0;
            while (LTE_Serial.available() && i < (MAX_G_BUFFER-1)) {
                int ch = LTE_Serial.read();
                // After power up there is often a 0x00 on the serial line from the modem.
                if (ch < 1) {
                    continue;
                }

                // Between 1 and 9 inclusive means noise.
                if (ch < 10) {
                    // Should only be reading CR, LF, and A-Z.
                    ESP_LOGI(TAG, "wait_for_at: %02x", ch);
                    continue;
                }

                g_buffer[i++] = (char)(ch & 0xFF);
                g_buffer[i] = 0;
            }

            ESP_LOGI(TAG, "[%s]", rsp);
            if (i > 3) {
                if (g_buffer[i-1] == '\n' && g_buffer[i-2] == '\r' && g_buffer[i-3] == 'K') {
                    return true;
                }
            }
        }

        delay(1000);
        attempts--;
    }

    return false;
}

/**
 * @brief Get the R5 modem connected to the internet.
 *
 * This function sets the ESP32 and modem RTCs from the network time using NTP.
 *
 * If this function returns true, the modem has registered to the network, brought up a
 * packet switched profile, and obtained an IP address. MQTT, FTP, etc can be used.
 *
 * @return true if the connection succeeds, otherwise false.
 */
bool connect_to_internet(void) {
    static bool already_called = false;

    log_to_sdcard("connect_to_internet");

    if ( ! cat_m1.make_ready()) {
        ESP_LOGE(TAG, "Could not initialise modem");
        log_to_sdcard("[E] Could not initialise modem");
        return false;
    }

    IPAddress ip_addr(0, 0, 0, 0);

    // If we've been through this function all the way (so the time is set) and we are connected
    // to the internet, return quickly.
    int reg_status = r5.registration();
    delay(20);
    if (reg_status == SARA_R5_REGISTRATION_HOME) {
        if (r5.getNetworkAssignedIPAddress(0, &ip_addr) == SARA_R5_ERROR_SUCCESS) {
            if (already_called && ip_addr[0] != 0) {
                ESP_LOGI(TAG, "Already connected to internet");
                log_to_sdcard(" Already connected to internet");
                return true;
            }
        }
    }

    // Hardware flow control pins not connected on the Wombat and R5 does not support software flow control.
    r5.setFlowControl(SARA_R5_DISABLE_FLOW_CONTROL);
    delay(20);

    // Only needs to be done one, but the Sparkfun library reads this value before setting so
    // it is quick enough to call this every time.
    if ( ! r5.setNetworkProfile(MNO_TELSTRA)) {
        delay(20);
        ESP_LOGE(TAG, "Error setting network operator profile");
        r5_ok = false;
        log_to_sdcard("[E] Error setting network operator profile, r5_ok now false");
        return false;
    }
    delay(20);

    // Network registration takes 4 seconds at best.
    ESP_LOGI(TAG, "Waiting for network registration");
    int attempts = 0;
    while (reg_status != SARA_R5_REGISTRATION_HOME && attempts < 45) {
        reg_status = r5.registration();
        delay(20);
        if (reg_status == SARA_R5_REGISTRATION_INVALID) {
            ESP_LOGI(TAG, "ESP registration query failed");
            log_to_sdcard("[E] ESP registration query failed");
            return false;
        }

        ESP_LOGI(TAG, "ESP registration status = %d", reg_status);
        r5.bufferedPoll();
        if (reg_status == SARA_R5_REGISTRATION_HOME) {
            break;
        }

        delay(2000);
        attempts++;
    }

    if (reg_status != SARA_R5_REGISTRATION_HOME) {
        ESP_LOGE(TAG, "Failed to register with network");
        log_to_sdcard("[E] Failed to register with network");
        return false;
    }

    log_to_sdcard("Getting network time");

    // Work in UTC.
    setenv("TZ", "UTC", 1);
    tzset();

    ESP_LOGI(TAG, "RTC time before ntp: %s", iso8601());
    log_to_sdcardf("RTC time before ntp: %s", iso8601_buf); // Look out, this is a side-effect of calling iso8601.

    log_to_sdcard("Starting PDP");

    // Enable the packet switching layer.
    // These commands come from the SARA R4/R5 Internet applications development guide
    // ss 2.3, table Profile Activation: SARA R5.
    r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_PROTOCOL, 0);
    delay(20);
    r5.bufferedPoll();
    r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_MAP_TO_CID, 1);
    delay(20);
    r5.bufferedPoll();
    r5.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE);
    delay(20);
    r5.bufferedPoll();

    ESP_LOGI(TAG, "Attempting NTP query");
    if ( ! getNTPTime(r5)) {
        ESP_LOGW(TAG, "NTP query failed");
        log_to_sdcard("NTP query failed");
    }

    ESP_LOGI(TAG, "RTC time after ntp: %s", iso8601());
    log_to_sdcardf("RTC time after ntp: %s", iso8601_buf); // Look out, this is a side-effect of calling iso8601.

    log_to_sdcard("cti returning");

    already_called = true;
    return true;
}
