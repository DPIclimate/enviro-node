#ifndef NODE_SDCARD_H
#define NODE_SDCARD_H

#include "Arduino.h"
#include "SD.h"
#include "SPI.h"

#define DEBUG

class SDCardInterface {
private:
    static const uint8_t SD_CS = 4;
    static bool sd_ok;

public:
    SDCardInterface();
    ~SDCardInterface();

    /// Enable the SD card and mount it.
    static bool begin(void);

    /// Create a file with the given content.
    static void add_file(const char* filepath, const char* contents);

    /// Append contents to the file at filepath.
    static void append_to_file(const char* filepath, const char* contents);

    /// Read and print the content file filepath.
    /// TODO: Fix this to write the content to a provided buffer.
    static void read_file(const char* filepath, Stream& stream);

    /// Delete the file at filepath.
    static void delete_file(const char* filepath);

    /// Returns true if an SD card was detected.
    static bool is_ready(void);

    #ifdef DEBUG
        void list_directory(File dir, uint8_t num_tabs);
    #endif

};

#endif //NODE_SDCARD_H
