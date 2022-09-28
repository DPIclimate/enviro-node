#include "node_sdcard.h"


Node_SDCard::Node_SDCard() {
    if(!SD.begin(SD_CS)){
        #ifdef DEBUG
            log_msg("[ERROR]: SD card not found.");
        #endif
    } else {
        #ifdef DEBUG
            log_msg("[DEBUG]: SD card found and initialised.");
        #endif
    }
}

Node_SDCard::~Node_SDCard() = default;

#ifdef DEBUG
// Recursive function to get SD-Cards directory structure
void Node_SDCard::list_directory(File dir, uint8_t num_spaces) { /* NOLINT */

    // Prevent overflow (shouldn't normally occur but ok to check)
    if(num_spaces == 255){
        log_msg("[ERROR]: Directory iterations exceeded.");
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
            log_msg(" (%d bytes)", item.size());
        }

        item.close();
    }
}
#endif

void Node_SDCard::add_file(const char* filepath, const char* contents){
    #ifdef DEBUG
        log_msg("[DEBUG]: Adding file '%s' to SD-Card.", filepath);
    #endif

    // Open will add a new file if it doesn't exist
    File root = SD.open(filepath, FILE_WRITE);

    // Check if file is open
    if(root) {
       root.println(contents);
    } else {
        #ifdef DEBUG
            log_msg("[ERROR]: Unable to create file on SD-Card.");
        #endif
    }

    root.close();
}


#ifdef DEBUG
void Node_SDCard::read_file(const char* filepath){
    log_msg("[DEBUG]: Reading file '%s' on SD-Card.", filepath);

    File root = SD.open(filepath);

    if(root) {
        log_msg("'%s': ", filepath);
        while(root.available()){
            Serial.write(root.read());
        }

        root.close();
    } else {
        log_msg("[ERROR]: Unable to read file: '%s'");
    }
}
#endif




