//
// Created by David Taylor on 15/11/2022.
//

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
