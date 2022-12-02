#ifndef WOMBAT_MQTT_CLI_H
#define WOMBAT_MQTT_CLI_H

#include "DeviceConfig.h"

class CLIMQTT {
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    inline static const std::string cmd = "mqtt";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);

    static void dump(Stream& stream);
};

#endif //WOMBAT_MQTT_CLI_H
