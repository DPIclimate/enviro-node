#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <Arduino.h>
#include <cstdarg>

#define DEBUG               // Startup in debug mode (includes logging output)
#define NODE_LORAWAN        // Use LoRaWAN
#define NODE_CAT_M1         // Use CAT-M1
#define NODE_BLUETOOTH      // Use Bluetooth
#define NODE_WIFI           // Use WiFi

#define POWER_12V_EN G5     // Pin for enabling 12V output

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

static const uint8_t PROGMEM APPEUI[8]= { 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00 };

static const uint8_t PROGMEM DEVEUI[8]= { 0xDA, 0x66, 0x05, 0xD0,
                                          0x7E, 0xD5, 0xB3, 0x70 };

static const uint8_t PROGMEM APPKEY[16] = { 0xC0, 0xC2, 0xF2, 0x87,
                                            0x21, 0x97, 0xD2, 0x5A,
                                            0x58, 0x5A, 0xC4, 0xFD,
                                            0xA6, 0x42, 0x10, 0xEA };

#endif //NODE_CONFIG_H
