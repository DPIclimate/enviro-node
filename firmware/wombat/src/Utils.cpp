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
    while ((millis() - start) < timeout && a < 1) {
        delay(1); // Let the MCU do something else.
        a = stream.available();
    }

    const uint32_t end = millis();
    ESP_LOGD(TAG, "Delta from start of read: %lu ms, UART available = %d", (end - start), a);

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

    struct tm t;
    memset(&t, 0, sizeof(t));
    time_t now;
    time(&now);
    gmtime_r(&now, &t);

    snprintf(iso8601_buf, sizeof(iso8601_buf)-1, "%04d-%02d-%02dT%02d:%02d:%02dZ", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    return iso8601_buf;
}

/**
 * @brief Waits until an AT command to the R5 modem returns OK, or a timeout is reached.
 *
 * This function will try 5 times, pausing for 1 second between attempts.
 *
 * This function uses g_buffer.

 * @return true if the modem responds, otherwise false.
 */
static bool wait_for_at(void) {
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

    if (!cat_m1.is_powered()) {
        ESP_LOGI(TAG, "Enabling R5 VCC");
        cat_m1.power_supply(true);
    }

    ESP_LOGI(TAG, "Looking for response to AT command");
    if ( ! wait_for_at()) {
        cat_m1.restart();
        if ( ! wait_for_at()) {
            ESP_LOGE(TAG, "Cannot talk to SARA R5");
            return false;
        }
    }

    r5.invertPowerPin(true);
    r5.autoTimeZoneForBegin(true);

    r5.enableAtDebugging();
    //r5.enableDebugging();

    // This is relatively benign - it enables the network indicator GPIO pin, set error message format, etc.
    // It does close all open sockets, but there should not be any open sockets at this point so that is ok.
    r5_ok = r5.begin(LTE_Serial, 115200);
    if ( ! r5_ok) {
        ESP_LOGE(TAG, "SARA-R5 begin failed");
        return false;
    }

    // If we've been through this function all the way (so the time is set) and we are connected
    // to the internet, return quickly.
    int reg_status = r5.registration();
    if (reg_status == SARA_R5_REGISTRATION_HOME && already_called) {
        ESP_LOGI(TAG, "Already connected to internet");
        return true;
    }

    // Hardware flow control pins not connected on the Wombat and R5 does not support software flow control.
    r5.setFlowControl(SARA_R5_DISABLE_FLOW_CONTROL);

    // Only needs to be done one, but the Sparkfun library reads this value before setting so
    // it is quick enough to call this every time.
    if ( ! r5.setNetworkProfile(MNO_TELSTRA)) {
        ESP_LOGE(TAG, "Error setting network operator profile");
        r5_ok = false;
        return false;
    }

    // Network registration takes 4 seconds at best.
    ESP_LOGI(TAG, "Waiting for network registration");
    delay(4000);

    for (int i = 0; i < 3; i++) {
        int attempts = 0;
        while (reg_status != SARA_R5_REGISTRATION_HOME && attempts < 4) {
            reg_status = r5.registration();
            if (reg_status == SARA_R5_REGISTRATION_INVALID) {
                ESP_LOGI(TAG, "ESP registration query failed");
                return false;
            }

            ESP_LOGI(TAG, "ESP registration status = %d", reg_status);
            r5.bufferedPoll();
            delay(4000);
            attempts++;
        }
    }

    if (reg_status != SARA_R5_REGISTRATION_HOME) {
        ESP_LOGE(TAG, "Failed to register with network");
        return false;
    }

    // Work in UTC.
    setenv("TZ", "UTC", 1);
    tzset();

    // Get the time from the R5 modem. This is given in local time with the
    // timezone given as the number of 15 minute offsets from UTC, eg +40 for AEST
    // and +44 for AEDT.
    // Example return from r5.clock(): 23/02/13,08:07:57+44
    constexpr size_t time_buffer_sz = 20;
    char time_buffer[time_buffer_sz + 1];
    memset(time_buffer, 0, sizeof(time_buffer_sz));
    String time_str = r5.clock();
    strncpy(time_buffer, time_str.c_str(), time_buffer_sz);
    time_buffer[time_buffer_sz] = 0;
    ESP_LOGI(TAG, "Modem response: %s", time_buffer);

    // Parse the response into a tm structure which will be used to convert to an epoch value.
    // Do not take account of the timezone or daylight savings. This means we'll be creating an
    // epoch time using the local time, but pretending it is UTC.
    tm _tm;
    int tz;

    sscanf(time_buffer, "%2d/%2d/%2d,%d:%d:%d%d", &_tm.tm_year, &_tm.tm_mon, &_tm.tm_mday, &_tm.tm_hour, &_tm.tm_min, &_tm.tm_sec, &tz);
    _tm.tm_year += 100; // tm.tm_year is years since 1900.
    _tm.tm_isdst = 0;   // No daylight savings.

    // Calculate an adjustment for the epoch value based upon the timezone information supplied
    // by the modem. For example, in summer in NSW the modem will say the time it has provided is
    // +44 from UTC. This means it is 44 15-minute intervals ahead.
    int dst_offset_secs = tz * 15 * -1 * 60;
    ESP_LOGI(TAG, "Network time: %02d/%02d/%04d %02d:%02d:%02d %02d", _tm.tm_mday, _tm.tm_mon, _tm.tm_year, _tm.tm_hour, _tm.tm_min, _tm.tm_sec, tz);
    _tm.tm_mon -= 1;    // tm.tm_month is months since January, so from 0 - 11.

    struct timeval tv;
    tv.tv_usec = 0;

    // Get the epoch value by converting the values in the tm structure. This will make tv.tv_sec equal to
    // the current local time, but assume it is UTC. So giving it the tm generated from the response 23/02/13,08:07:57+44
    // means tv.tv_sec epoch will represent 23/02/13,08:07:57 UTC.
    tv.tv_sec = mktime(&_tm);

    // To fix that, apply the timezone offset calculated above. After this is applied tv_tv.sec will represent
    // 23/02/12,21:07:57 UTC.
    tv.tv_sec += dst_offset_secs;
    ESP_LOGI(TAG, "Epoch: %ld", tv.tv_sec);

    // Now set the system clock.
    settimeofday(&tv, nullptr);

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

    already_called = true;
    return true;
}
