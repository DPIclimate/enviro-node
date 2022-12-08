#ifndef WOMBAT_UTILS_H
#define WOMBAT_UTILS_H

#include <stddef.h>
#include <ArduinoJson.h>
#include <dpiclimate-12.h>

extern size_t stripLeadingWS(char *str);
extern size_t stripTrailingWS(char *str);
extern size_t stripWS(char *str);

extern size_t readFromStreamUntil(Stream& stream, char delim, char* buffer, size_t max);
extern void streamPassthrough(Stream* s1, Stream* s2);

extern int waitForChar(Stream& stream, uint32_t timeout);

JsonObjectConst getSensorDefn(const char* vendor, const char* model);
JsonObjectConst getSensorDefn(const sensor_info& info);

extern void enable12V(void);
extern void disable12V(void);

#endif //WOMBAT_UTILS_H
