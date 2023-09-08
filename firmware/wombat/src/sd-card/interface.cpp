#include "sd-card/interface.h"
#include "globals.h"
#include <esp_log.h>

#define TAG "sd"

SDCardInterface::SDCardInterface() = default;
SDCardInterface::~SDCardInterface() = default;

bool SDCardInterface::sd_ok = false;

bool SDCardInterface::is_ready(void) {
    return sd_ok;
}

bool SDCardInterface::begin(void) {
    // Connect the SD card GND so it powers up.
    digitalWrite(SD_CARD_ENABLE, HIGH);

    sd_ok = SD.begin(SD_CS);
    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not found.");
    } else {
        ESP_LOGD(TAG, "SD card found and initialised.");
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            ESP_LOGW(TAG, "Could not determine SD card type");
            sd_ok = false;
        } else {
            uint64_t cardSize = SD.cardSize() / (1024 * 1024);
            ESP_LOGI(TAG, "SD Card Size: %lluMB", cardSize);
        }
    }

    return sd_ok;
}

#ifdef DEBUG
// Recursive function to get SD-Cards directory structure
void SDCardInterface::list_directory(File dir, uint8_t num_spaces) { /* NOLINT */

    // Prevent overflow (shouldn't normally occur but ok to check)
    if(num_spaces == 255){
        ESP_LOGE(TAG, "Directory iterations exceeded.");
        return;
    }

    while(true){
        File item = dir.openNextFile();
        if(!item){
            // No more files
            break;
        }

        for(uint8_t i = 0; i < num_spaces; i++){
            Serial.print("  ");
        }

        Serial.print(item.name());

        if(item.isDirectory()){
            Serial.println("/");
            SDCardInterface::list_directory(item, num_spaces++);
        } else {
            // Files (with sizes)
            ESP_LOGD(TAG, " (%d bytes)", item.size());
        }

        item.close();
    }
}
#endif

void SDCardInterface::add_file(const char* filepath, const char* contents) {
    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not initialised");
        return;
    }

    if (filepath == nullptr || strnlen(filepath, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return;
    }

    if (contents == nullptr || strnlen(contents, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return;
    }

    ESP_LOGD(TAG, "Adding file '%s' to SD-Card.", filepath);

    // Open will add a new file if it doesn't exist
    File fp = SD.open(filepath, FILE_WRITE);
    if (fp) {
        fp.println(contents);
        fp.close();
    } else {
        ESP_LOGE(TAG, "Unable to create file on SD-Card");
    }
}

void SDCardInterface::append_to_file(const char* filepath, const char* contents) {
    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not initialised");
        return;
    }

    if (filepath == nullptr || strnlen(filepath, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return;
    }

    File fp = SD.open(filepath, FILE_APPEND, true);
    if (fp) {
        fp.print(contents);
        fp.close();
    }
}

void SDCardInterface::read_file(const char* filepath, Stream &stream) {
    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not initialised");
        return;
    }

    if (filepath == nullptr || strnlen(filepath, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return;
    }

    ESP_LOGD(TAG, "Reading file '%s' on SD-Card.", filepath);

    File fp = SD.open(filepath);
    if (fp) {
        ESP_LOGD(TAG, "'%s': ", filepath);
        while (fp.available()) {
            stream.write(fp.read());
        }

        fp.close();
    } else {
        ESP_LOGE(TAG, "Unable to read file: '%s'", filepath);
    }
}

size_t SDCardInterface::read_file(const char *filepath, char * buffer, const size_t buffer_size, const size_t file_location) {
    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not initialised");
        return -1;
    }

    if (filepath == nullptr || strnlen(filepath, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return 0;
    }

    ESP_LOGI(TAG, "Reading file '%s' on SD-Card.", filepath);

    File fp = SD.open(filepath);
    
    if (fp) {
        ESP_LOGD(TAG, "'%s': ", filepath);
        fp.seek(file_location);

        size_t bytes_read = fp.readBytes(buffer, buffer_size);
        fp.close();
        ESP_LOGI(TAG,"Finished read, read num of bytes %d", bytes_read);
        return bytes_read;

    } else {
        ESP_LOGE(TAG, "Unable to read file: '%s'", filepath);
        return 0;
    }
}

size_t SDCardInterface::get_file_size(const char *filepath) {

    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not initialised");
        return 0;
    }

    if (filepath == nullptr || strnlen(filepath, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return 0;
    }

    File fp=SD.open(filepath);
    ESP_LOGI(TAG, "File size is %d", fp.size());
    return fp.size();
}

void SDCardInterface::delete_file(const char* filepath) {
    if ( ! sd_ok) {
        ESP_LOGE(TAG, "SD card not initialised");
        return;
    }

    if (filepath == nullptr || strnlen(filepath, 1) == 0) {
        ESP_LOGE(TAG, "Empty filepath");
        return;
    }

    ESP_LOGD(TAG, "Deleting file '%s' on SD-Card.", filepath);
    if ( ! SD.remove(filepath)) {
        ESP_LOGW(TAG, "Delete failed on file: %s", filepath);
    }
}
