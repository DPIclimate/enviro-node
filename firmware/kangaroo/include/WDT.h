#ifndef KANGAROO_WDT_H
#define KANGAROO_WDT_H

extern void log_msg(const char *fmt, ...);

void wdt_init();
void wdt_enable();
void wdt_disable();
void wdt_reset();

#endif //KANGAROO_WDT_H
