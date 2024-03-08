#ifndef WOMBAT_UTILS_H
#define WOMBAT_UTILS_H

#include <ArduinoJson.h>
#include <dpiclimate-12.h>

#include "globals.h"

size_t readFromStreamUntil(Stream& stream, const char delim, char * const buffer, const size_t max);
void streamPassthrough(Stream* s1, Stream* s2);

int waitForChar(Stream& stream, uint32_t timeout);

JsonObjectConst getSensorDefn(const char* const vendor, const char* const model);
JsonObjectConst getSensorDefn(const size_t sensor_idx, const sensor_list& sensors);

void enable12V(void);
void disable12V(void);

const char* iso8601(void);
void log_to_sdcard(const char * msg);
void log_to_sdcardf(const char *fmt, ...);

/**
 * Reads a file from the SPIFFS filesystem into buffer.
 *
 * @param filename The name of the file to read.
 * @param buffer The buffer to read the file into.
 * @param max_length The size of buffer in bytes.
 * @param bytes_read [OUT] How many bytes were read into buffer.
 * @return 0 on success, otherwise a failure.
 */
int read_spiffs_file(const char* filename, char* buffer, size_t max_length, size_t &bytes_read);

/**
 * Reads a file from the R5 filesystem into buffer.
 *
 * This function assumes sizeof(buffer) >= length.
 *
 * @param filename The name of the file to read.
 * @param buffer The buffer to read the file into.
 * @param length The size of the file in bytes.
 * @param bytes_read [OUT] How many bytes were read into buffer.
 * @param r5_err [OUT] The error code returned from the R5 library, if an error ocurred.
 * @return 0 on success, otherwise a failure.
 */
int read_r5_file(const String& filename, char* buffer, size_t length, size_t &bytes_read, SARA_R5_error_t& r5_err);

bool wait_for_at(void);
bool connect_to_internet(void);

int get_version_string(char *buffer, size_t length);

template <typename T>
struct URC {
    bool valid { false };
    T command { static_cast<T>(-1) };
    int result { -1 };
    int err1 { 0 };
    int err2 { 0 };

    URC()= default;
    URC(bool v, T c, int r) {
        valid = v;
        command = c;
        result = r;
    }
};

template <typename T>
class CommandURCVector : public std::vector<URC<T>> {
public:
    // Constructor - allocate space for 6 URCs.
    CommandURCVector() : std::vector<URC<T>>() {}

    // Add a pair to the vector
    void addPair(const T& value, int intValue) {
        this->emplace_back(true, value, intValue);
    }

    /**
     * Check if the given command URC has been received. If so, put the result into result and
     * remove the URC from the list and return true. Otherwise return false.
     *
     * @param command the command to look for.
     * @param result the result of the command if it was found, otherwise unchanged.
     * @return true if the command URC was found, otherwise false.
     */
    bool hasURC(const T command, int *result) {
        auto iter = this->begin();
        while (iter != this->end()) {
            ESP_LOGI("CommandURCVector", "valid = %d, command = %d, result = %d, err1 = %d, err2 = %d", iter->valid, iter->command, iter->result, iter->err1, iter->err2);
            if (iter->command == command) {
                *result = iter->result;
                log_to_sdcardf("valid = %d, command = %d, result = %d, err1 = %d, err2 = %d", iter->valid, iter->command, iter->result, iter->err1, iter->err2);
                iter->valid = false;
                this->erase(iter);
                return true;
            }

            iter++;
        }

        return false;
    }

    bool waitForURC(const T command, int *result, int retries, const long delay_ms) {
        bool found_urc = false;
        while (retries > 0) {
            r5.bufferedPoll();
            found_urc = hasURC(command, result);
            if (found_urc) {
                break;
            }

            delay(delay_ms);
            retries--;
        }

        ESP_LOGI("CommandURCVector", "command = %d, result = %d, found_urc = %d, retries = %d", command, *result, found_urc, retries);
        log_to_sdcardf("command = %d, result = %d, found_urc = %d, retries = %d", command, *result, found_urc, retries);
        return found_urc;
    }
};

#endif //WOMBAT_UTILS_H
