#include "sdcard/node_sdcard.h"
#include <esp_log.h>

#define TAG "sd"

Node_SDCard::Node_SDCard() {
    ESP_LOGI(TAG, "%d is valid: %d", SD_CS, GPIO_IS_VALID_GPIO(SD_CS));

    if(!SD.begin(SD_CS)){
        ESP_LOGE(TAG, "SD card not found.");
    } else {
        ESP_LOGD(TAG, "SD card found and initialised.");
    }
}

Node_SDCard::~Node_SDCard() = default;

#ifdef DEBUG
// Recursive function to get SD-Cards directory structure
void Node_SDCard::list_directory(File dir, uint8_t num_spaces) { /* NOLINT */

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
            Node_SDCard::list_directory(item, num_spaces++);
        } else {
            // Files (with sizes)
            ESP_LOGD(TAG, " (%d bytes)", item.size());
        }

        item.close();
    }
}
#endif

void Node_SDCard::add_file(const char* filepath, const char* contents){
    ESP_LOGD(TAG, "Adding file '%s' to SD-Card.", filepath);

    // Open will add a new file if it doesn't exist
    File root = SD.open(filepath, FILE_WRITE);

    // Check if file is open
    if(root) {
       root.println(contents);
    } else {
        ESP_LOGE(TAG, "Unable to create file on SD-Card.");
    }

    root.close();
}

void Node_SDCard::read_file(const char* filepath){
    ESP_LOGD(TAG, "Reading file '%s' on SD-Card.", filepath);

    File root = SD.open(filepath);

    if(root) {
        ESP_LOGD(TAG, "'%s': ", filepath);
        while(root.available()){
            Serial.write(root.read());
        }

        root.close();
    } else {
        ESP_LOGE(TAG, "[ERROR]: Unable to read file: '%s'");
    }
}
