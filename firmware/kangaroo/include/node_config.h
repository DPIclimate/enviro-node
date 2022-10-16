#ifndef KANGAROO_NODE_CONFIG_H
#define KANGAROO_NODE_CONFIG_H

#define USE_SERIAL
#define serial Serial1

#define PUSH_BUTTON      5
#define LORA_CS_PIN      8
#define SDI12_POWER      9
#define SDI12_DATA_PIN  10
#define SDCARD_CS_PIN   11
#define WIND_SPEED_PIN  12
#define WIND_DIR_PIN    A1
#define VBATT           A7

// Must be included so the LMIC pin map is defined. LORA_CS_PIN must be defined before this file is included.
#include <LoRa-otaa.h>

/*

SAM D21 Clock / TCC notes.

GCLK(1) uses XOSC32K and drives the EIC & TCC 4/5. XOSC32K set to run in standby mode. The EIC is used for
the LoRa Test button to wake from standby, and TCC 4/5 is used as the source of LMIC systicks.

*/
#endif // KANGAROO_NODE_CONFIG_H
