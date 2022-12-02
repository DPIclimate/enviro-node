#ifndef WOMBAT_UTILS_H
#define WOMBAT_UTILS_H

#include <stddef.h>

extern size_t stripLeadingWS(char *str);
extern size_t stripTrailingWS(char *str);
extern size_t stripWS(char *str);

extern size_t readFromStreamUntil(Stream& stream, char delim, char* buffer, size_t max);
extern void streamPassthrough(Stream* s1, Stream* s2);

extern int waitForChar(Stream& stream, uint32_t timeout);

#endif //WOMBAT_UTILS_H
