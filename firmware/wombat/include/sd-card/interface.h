#ifndef NODE_SDCARD_H
#define NODE_SDCARD_H

#include "Arduino.h"
#include "SD.h"
#include "SPI.h"

#define DEBUG

class SDCardInterface {
private:
    // NOTE: This is a dummy. The SD card hardware is actually connected
    // to I/O 2 on the TCA9534 I/O expander. The SD library wants a pin
    // to toggle for CS so we give it an unconnected one, and toggle the
    // actual CS pin before and after calling SD card library functions.
    static const uint8_t SD_CS = 4;

public:
    SDCardInterface();
    ~SDCardInterface();

    static void add_file(const char* filepath, const char* contents);

    #ifdef DEBUG
        void list_directory(File dir, uint8_t num_tabs);
        static void read_file(const char* filepath);
    #endif

};

#endif //NODE_SDCARD_H
