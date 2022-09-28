#ifndef NODE_SDCARD_H
#define NODE_SDCARD_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "node_config.h"

class Node_SDCard {
private:
    static const uint8_t SD_CS = 20;

public:
    Node_SDCard();
    ~Node_SDCard();

    static void add_file(const char* filepath, const char* contents);

    #ifdef DEBUG
        void list_directory(File dir, uint8_t num_tabs);
        static void read_file(const char* filepath);
    #endif

};

#endif //NODE_SDCARD_H
