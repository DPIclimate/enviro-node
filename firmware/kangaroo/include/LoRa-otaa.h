#ifndef LORA_OTAA_H
#define LORA_OTAA_H

#include <hal/hal.h>

static const uint8_t PROGMEM DEVEUI[8]= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void os_getArtEui (uint8_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (uint8_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (uint8_t* buf) { memcpy_P(buf, APPKEY, 16);}

// Pin mapping for Adafruit Feather M0 LoRa, etc.
// /!\ By default Adafruit Feather M0's pin 6 and DIO1 are not connected.
// You must ensure they are connected.
const lmic_pinmap lmic_pins = {
        .nss = LORA_CS_PIN,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = 4,
        .dio = {3, 6, LMIC_UNUSED_PIN},
        .rxtx_rx_active = 0,
        .rssi_cal = 8,              // LBT cal for the Adafruit Feather M0 LoRa, in dB
        .spi_freq = 8000000,
};

#endif // LORA_OTAA_H
