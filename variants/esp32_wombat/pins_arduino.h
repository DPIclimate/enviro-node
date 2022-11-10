#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_MANUFACTURER "DPI Climate"
#define USB_PRODUCT "DPI Climate ESP32 Wombat"
#define USB_SERIAL ""

#define EXTERNAL_NUM_INTERRUPTS 46
#define NUM_DIGITAL_PINS        48
#define NUM_ANALOG_INPUTS       20

#define analogInputToDigitalPin(p)  (((p)<20)?(analogChannelToDigitalPin(p)):-1)
#define digitalPinToInterrupt(p)    (((p)<48)?(p):-1)
#define digitalPinHasPWM(p)         (p < 46)

static const uint8_t LED_BUILTIN = 13;
#define BUILTIN_LED  LED_BUILTIN // backward compatibility

// static const uint8_t TX = 43;
// static const uint8_t RX = 44;

#define TX0 34
#define RX0 33

#define TX1 36
#define RX1 11

static const uint8_t SDA = 37;
static const uint8_t SCL = 10;

static const uint8_t MOSI  = 30;
static const uint8_t MISO  = 31;
static const uint8_t SCK   = 29;

static const uint8_t SDI_12 = 4;
static const uint8_t PIEZO = 28;

#endif /* Pins_Arduino_h */