#ifndef WOMBAT_CLI_H
#define WOMBAT_CLI_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "cli/FreeRTOS_CLI.h"
#include "cli/peripherals/cat-m1.h"
#include "cli/peripherals/sdi12.h"
#include "cli/peripherals/bluetooth.h"
#include "cli/device_config/acquisition_intervals.h"

#define CLI_TAG "cli"

class CLI {
public:
    static const uint8_t MAX_CLI_CMD_LEN = 48;
    static const uint8_t MAX_CLI_MSG_LEN = 255;

    static void init();
    static void repl(Stream& io);
};

#endif //WOMBAT_CLI_H
