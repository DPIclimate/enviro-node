#ifndef WOMBAT_GLOBALS_H
#define WOMBAT_GLOBALS_H

#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "CAT_M1.h"

#ifdef ALLOCATE_GLOBALS
#define EXTERN /**/
#else
#define EXTERN extern

extern const char* commit_id;
extern const char* repo_status;
extern  uint16_t ver_major;
extern  uint16_t ver_minor;
extern  uint16_t ver_update;

extern bool spiffs_ok;
#endif

/// Convert an IO expander pin number (1, 2, ...) to an pin number usable with digitalWrite (0x81, 0x82, ...)
/// Note that digitalWrite has been redefined to understand that pin numbers >= 0x80 should be toggled on the IO
/// expander IC, not the ESP32.
#define IO_TO_ARDUINO(p) (0x80 + p)

/// Convert an Arduino pin number usable with digitalWrite (0x81, 0x82, ...) to an IO expander pin number (1, 2, ...)
#define ARDUINO_TO_IO(p) (p & 0x7F)

/// Set HIGH to enable the SD card, LOW to disable the card. This pin drives a transistor that connects or disconnects
/// the SD card GND pin.
#define SD_CARD_ENABLE 0x85

/// The size for a buffer to hold the string representation of an integer or float.
#define MAX_NUMERIC_STR_SZ 32

/// The maximum safe size for g_buffer.
#define MAX_G_BUFFER 65536

/// A buffer for use anywhere in the main task, it is MAX_G_BUFFER+1 bytes long.
/// Use MAX_G_BUFFER as the length when writing strings to g_buffer, but use
/// sizeof(g_buffer) in memset to guarantee a terminating null.
///
/// Do not use in other tasks.
EXTERN char g_buffer[MAX_G_BUFFER + 1];

EXTERN bool r5_ok;

constexpr char sd_card_datafile_name[] = "/data.json";
constexpr char sd_card_logfile_name[] = "/log.txt";
constexpr char send_fw_version_name[] = "/send_fw_version";

void shutdown(void);

#ifdef ALLOCATE_GLOBALS
/// A global SARA R5 modem object.
SARA_R5 r5(LTE_PWR_ON, -1);
#else
extern SARA_R5 r5;
#endif

extern char* script;

#endif //WOMBAT_GLOBALS_H
