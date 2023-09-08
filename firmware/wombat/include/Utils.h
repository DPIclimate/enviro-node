#ifndef WOMBAT_UTILS_H
#define WOMBAT_UTILS_H

#include <stddef.h>
#include <ArduinoJson.h>
#include <dpiclimate-12.h>

#include "globals.h"

const char *stripTrailingZeros(const float value);
size_t stripLeadingWS(char *str);
size_t stripTrailingWS(char *str);
size_t stripWS(char *str);

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
