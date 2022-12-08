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

JsonObjectConst getSensorDefn(const char* vendor, const char* model);
JsonObjectConst getSensorDefn(const sensor_info& info);

void enable12V(void);
void disable12V(void);

const char* iso8601(void);

#endif //WOMBAT_UTILS_H
