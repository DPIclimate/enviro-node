#include <Arduino.h>
#include "Utils.h"
#include <string.h>
#include "DeviceConfig.h"
#include "TCA9534.h"
#include "CAT_M1.h"
#include "globals.h"

#define TAG "utils"

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

size_t stripWS(char *str) {
    stripTrailingWS(str);
    return stripLeadingWS(str);
}

size_t readFromStreamUntil(Stream& stream, char delim, char* buffer, size_t max) {
    char ch;
    size_t len = 0;
    while (len < max) {
        while ( ! stream.available()) {
            yield();
        }

        while (stream.available()) {
            ch = stream.read();
            if (ch == delim) {
                return len;
            }

            buffer[len] = ch;
            len++;
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

// Check if the time has been set.
bool time_ok(void) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    time_t now;
    time(&now);
    gmtime_r(&now, &t);

    return (t.tm_year+1900) >= 2022 && (t.tm_mon+1) >= 12;
}

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

bool connect_to_internet(void) {
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
    r5.autoTimeZoneForBegin(false);

    r5.enableAtDebugging();
    r5.enableDebugging();

    // This is relatively benign - it enables the network indicator GPIO pin, set error message format, etc.
    // It does close all open sockets, but there should not be any open sockets at this point so that is ok.
    r5_ok = r5.begin(LTE_Serial, 115200);
    if ( ! r5_ok) {
        ESP_LOGE(TAG, "SARA-R5 begin failed");
        return false;
    }

    // Only needs to be done one, but the Sparkfun library reads this value before setting so
    // it is quick enough to call this every time.
    if ( ! r5.setNetworkProfile(MNO_TELSTRA)) {
        ESP_LOGE(TAG, "Error setting network operator profile");
        r5_ok = false;
        return false;
    }
    delay(20);

    int reg_status = 0;
    while (reg_status != SARA_R5_REGISTRATION_HOME) {
        reg_status = r5.registration();
        if (reg_status == SARA_R5_REGISTRATION_INVALID) {
            ESP_LOGI(TAG, "ESP registration query failed");
            return false;
        }

        ESP_LOGI(TAG, "ESP registration status = %d", reg_status);
        delay(2000);
    }

    // These commands come from the SARA R4/R5 Internet applications development guide
    // ss 2.3, table Profile Activation: SARA R5.
    r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_PROTOCOL, 0);
    delay(20);
    r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_MAP_TO_CID, 1);
    delay(20);
    r5.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE);
    delay(20);

    return true;
}
