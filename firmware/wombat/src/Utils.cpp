/**
 * \file Utils.cpp
 * \brief Global utility functions.
*/

#include <Arduino.h>
#include "Utils.h"
#include <string.h>
#include "DeviceConfig.h"
#include "TCA9534.h"
#include "CAT_M1.h"
#include "globals.h"
#include "sd-card/interface.h"

#define TAG "utils"


/**
 * @brief Returns a pointer to a string representation of value, with trailing zeros removed.
 *
 * The default %f formatter is used to get the initial string representation, so 6 digits after the
 * decimal point. From that, trailing zeros are removed.
 *
 * The pointer returned from this function must not be passed to free.
 * This function is not re-entrant.
 *
 * @param value the value to convert to a string.
 * @return a pointer to a string representation of value, with trailing zeros removed.
 */
const char *stripTrailingZeros(const float value) {
    static char float_buf[20];

    int len = snprintf(float_buf, sizeof(float_buf)-1, "%f", value);
    len--;
    while (len > 0) {
        if (float_buf[len] != '0') {
            break;
        }

        // Don't strip zeros immediately after the decimal point, ie don't
        // convert '1.0' to '1.'
        if (len > 1 && float_buf[len - 1] == '.') {
            break;
        }

        float_buf[len] = 0;
        len--;
    }

    return float_buf;
}

/**
 * @brief Removes leading whitespace characters from str.
 *
 * Any character with a value of less than space is considered to be whitespace.
 *
 * @param str the string whose leading whitespace is stripped.
 * @return the new length of str, or 0 if str is null or a zero-length string.
 */
size_t stripLeadingWS(char *str) {
    if (str == nullptr || *str == 0) {
        return 0;
    }

    char *p = str;
    while (*p <= ' ') {
        p++;
    }

    char *s = str;
    if (p != str) {
        while (*p != 0) {
            *s++ = *p++;
        }

        *s = 0;
    }

    return strlen(str);
}

/**
 * @brief Removes trailing whitespace characters from str.
 *
 * Any character with a value of less than space is considered to be whitespace.
 *
 * @param str the string whose trailing whitespace is stripped.
 * @return the new length of str, or 0 if str is null or a zero-length string.
 */
size_t stripTrailingWS(char *str) {
    if (str == nullptr || *str == 0) {
        return 0;
    }

    // Strip trailing whitespace. response_buffer + len would point to the trailing null
    // so subtract one from that to get to the last character.
    size_t len = strlen(str);
    if (len == 0) {
        return len;
    }

    char *p = str + len - 1;
    while (p >= str && *p != 0 && *p <= ' ') {
        *p = 0;
        p--;
    }

    // Why 1 is added here: imagine the string is "X"; this means p == msg_buf but the string
    // length is 1, not 0.
    len = p - str + 1;
    return len;
}

/**
 * @brief Removes leading and trailing whitespace characters from str.
 *
 * Any character with a value of less than space is considered to be whitespace.
 *
 * @param str the string whose leading and trailing whitespace is stripped.
 * @return the new length of str, or 0 if str is null or a zero-length string.
 */
size_t stripWS(char *str) {
    stripTrailingWS(str);
    return stripLeadingWS(str);
}

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
            stripWS(cmd);
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
                int ch = (char)(LTE_Serial.read() & 0xFF);
                if (ch > 0) {
                    g_buffer[i++] = ch;
                    g_buffer[i] = 0;
                }
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
 * Attempt to get the current time from the modem. Times reported before 2023 from the modem are assumed to be in
 * error because this firmware went into production in 2023.
 *
 * @param tv on return, tv.tv_sec contains a UNIX style epoch value calculated from the local time provided by the modem.
 * @return true if the time from the modem is valid, otherwise false.
 */
bool get_network_time(struct timeval & tv) {
    int retries = 3;
    constexpr size_t time_buffer_sz = 20;
    char time_buffer[time_buffer_sz + 1];
    // Try a few times with a pause. The modem doesn't always get the network time quickly enough for an immediate
    // query.
    while (retries > 0) {
        // Get the time from the R5 modem. This is given in local time with the
        // timezone given as the number of 15 minute offsets from UTC, eg +40 for AEST
        // and +44 for AEDT.
        // Example return from r5.clock(): 23/02/13,08:07:57+44
        memset(time_buffer, 0, sizeof(time_buffer_sz));
        String time_str = r5.clock();
        delay(20);
        strncpy(time_buffer, time_str.c_str(), time_buffer_sz);
        time_buffer[time_buffer_sz] = 0;

        ESP_LOGI(TAG, "Time from modem: %s", time_buffer);
        log_to_sdcardf("Time from modem: %s", time_buffer);

        int tz;
        int year;
        struct tm tm{};

        int field_count = sscanf(time_buffer, "%2d/%2d/%2d,%d:%d:%d%d", &year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &tz);
        // The firmware was put into production in 2023, so earlier times must be invalid. This is to capture the modem
        // default value of '15/01/01,00:00:09+00'.
        if (field_count == 7 && year > 22) {
            // Parse the response into a tm structure which will be used to convert to an epoch value.
            // Do not take account of the timezone or daylight savings. This means we'll be creating an
            // epoch time using the local time, but pretending it is UTC. The timezone offset is calculated
            // and applied later to make it true UTC.
            tm.tm_year = year + 100; // tm.tm_year is years since 1900, and modem gives 2-digit year since 2000.
            tm.tm_isdst = 0;   // No daylight savings.

            ESP_LOGI(TAG, "Network time: %02d/%02d/%04d %02d:%02d:%02d %02d", tm.tm_mday, tm.tm_mon, tm.tm_year + 1900,
                     tm.tm_hour, tm.tm_min, tm.tm_sec, tz);

            tm.tm_mon -= 1;    // tm.tm_month is months since January, so from 0 - 11.


            // No microseconds in the calculated time.
            tv.tv_usec = 0;

            // Get the epoch value by converting the values in the tm structure. This will make tv.tv_sec equal to
            // the current local time, but assume it is UTC. So giving it the tm generated from the response 23/02/13,08:07:57+44
            // means tv.tv_sec epoch will represent 23/02/13,08:07:57 UTC.
            tv.tv_sec = mktime(&tm);

            // Calculate an adjustment for the epoch value based upon the timezone information supplied
            // by the modem. The modem reports timezone offsets in units of 15 minutes.
            int dst_offset_secs = tz * 15 * -1 * 60;
            tv.tv_sec += dst_offset_secs;
            ESP_LOGI(TAG, "1970 epoch calculated from modem time: %ld", tv.tv_sec);

            return true;
        }

        ESP_LOGE(TAG, "Time too early or could not parse, wait and retry");
        log_to_sdcard("[E] Time too early or could not parse, wait and retry");

        delay(1000);
        retries--;
    }

    ESP_LOGE(TAG, "Failed to parse time from modem");
    log_to_sdcard("[E] Failed to parse time from modem");
    return false;
}

/**
 * @brief Get the R5 modem connected to the internet.
 *
 * This function sets the ESP32 RTC from the network time.
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

    ESP_LOGI(TAG, "RTC time now: %s", iso8601());
    log_to_sdcardf("RTC time now: %s", iso8601_buf); // Look out, this is a side-effect of calling iso8601.

    bool time_set = false;
    struct timeval tv{};

    if (get_network_time(tv)) {
        // tv.tv_sec is a time_t so difftime can be used to compare it with 'now' and get the difference in seconds.
        // If the time from the ESP32 RTC is later than the time from the modem (tv.tv_sec) then timedelta will be
        // negative, so assume the modem is giving the wrong time and don't set the RTC from the modem time.
        time_t now = time(nullptr);
        double timedelta = difftime(tv.tv_sec, now);
        ESP_LOGI(TAG, "timedelta = %f", timedelta);
        if (timedelta > 0.0) {
            // Now set the system clock.
            settimeofday(&tv, nullptr);
            time_set = true;
        } else {
            ESP_LOGW(TAG, "timedelta < 0");
        }
    }

    if ( ! time_set) {
        ESP_LOGI(TAG, "Assuming bad time from modem, not setting RTC");
        log_to_sdcard("Assuming bad time from modem, not setting RTC");
    }

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

    log_to_sdcard("cti returning");

    already_called = true;
    return true;
}

int get_version_string(char *buffer, size_t length) {
    if (buffer == nullptr) {
        return -1;
    }

    int i = snprintf(buffer, length, "%u.%u.%u", ver_major, ver_minor, ver_update);
    return i;
}
