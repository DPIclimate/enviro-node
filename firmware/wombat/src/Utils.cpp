#include <Arduino.h>
#include "Utils.h"
#include <string.h>
#include "DeviceConfig.h"
#include "TCA9534.h"

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
    uint32_t start = millis();
    int a = stream.available();
    while ((millis() - start) < timeout && a < 1) {
        delay(1); // Let the MCU do something else.
        a = stream.available();
    }

    uint32_t end = millis();
    ESP_LOGD(TAG, "Delta from start of read: %lu ms, UART available = %d", (end - start), a);

    return a > 0 ? a : -1;
}

JsonObjectConst getSensorDefn(const char* vendor, const char* model) {
    const JsonDocument& sdi12Defns = DeviceConfig::get().getSDI12Defns();
    JsonObjectConst obj = sdi12Defns[vendor][model];
    return obj;
}

JsonObjectConst getSensorDefn(const sensor_info& info) {
    char vendor[LEN_VENDOR+1];
    char model[LEN_MODEL+1];

    memset(vendor, 0, sizeof(vendor));
    const char *ivptr = reinterpret_cast<const char *>(info.vendor);
    strncpy(vendor, ivptr, LEN_VENDOR);
    stripTrailingWS(vendor);
    memset(model, 0, sizeof(model));
    const char *imptr = reinterpret_cast<const char *>(info.model);
    strncpy(model, imptr, LEN_MODEL);
    stripTrailingWS(model);

    return getSensorDefn(vendor, model);
}

extern TCA9534 io_expander;

void enable12V(void) {
    io_expander.output(6, TCA9534::Level::H);
    delay(500);
}

void disable12V(void) {
    io_expander.output(6, TCA9534::Level::L);
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

    snprintf(iso8601_buf, sizeof(iso8601_buf)-1, "%04d-%02d-%02dT%02d:%02d:%02dZ", t.tm_year+1900, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    return iso8601_buf;
}
