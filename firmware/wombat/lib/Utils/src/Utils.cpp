//
// Created by David Taylor on 15/11/2022.
//

#include <Arduino.h>
#include "Utils.h"
#include <string.h>

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
