#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <Arduino.h>
#include <cstdarg>

#define DEBUG // Startup in debug mode (includes logging output)
#define NODE_LORAWAN // Use LoRaWAN

#define POWER_12V_EN G5 // Pin number for enabling 12V output

#ifdef DEBUG
    static const uint8_t LOG_LEN = 255;
    static char msg[LOG_LEN];
    static void log_msg(const char* fmt, ...){
        va_list args;
        va_start(args, fmt);
        vsniprintf(msg, sizeof(msg), fmt, args);
        va_end(args);
        Serial.println(msg);
    }
#endif

#endif //NODE_CONFIG_H
