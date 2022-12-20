#ifndef WOMBAT_UTILS_H
#define WOMBAT_UTILS_H

#include <stddef.h>
#include <ArduinoJson.h>
#include <dpiclimate-12.h>

size_t stripLeadingWS(char *str);
size_t stripTrailingWS(char *str);
size_t stripWS(char *str);

size_t readFromStreamUntil(Stream& stream, char delim, char* buffer, size_t max);
void streamPassthrough(Stream* s1, Stream* s2);

int waitForChar(Stream& stream, uint32_t timeout);

JsonObjectConst getSensorDefn(const char* const vendor, const char* const model);
JsonObjectConst getSensorDefn(const size_t sensor_idx, const sensor_list& sensors);

void enable12V(void);
void disable12V(void);

bool time_ok(void);
const char* iso8601(void);

bool connect_to_internet(void);

#endif //WOMBAT_UTILS_H
