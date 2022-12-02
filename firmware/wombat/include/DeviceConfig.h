#ifndef WOMBAT_DEVICECONFIG_H
#define WOMBAT_DEVICECONFIG_H

#include <Arduino.h>

class DeviceConfig {
public:
    static constexpr size_t MAX_CONFIG_STR = 32;

    uint8_t mac[6];
    char node_id[13];
    char mqtt_topic_template[MAX_CONFIG_STR+1];

    static DeviceConfig& get() {
        static DeviceConfig instance;
        return instance;
    }

    void reset();
    void load();
    void save();

    uint32_t getBootCount(void);

    uint16_t getMeasureInterval() { return measure_interval; }

    void setMeasureInterval(const uint16_t minutes);

    uint16_t getUplinkInterval() { return uplink_interval; }

    void setUplinkInterval(const uint16_t minutes);

    void setMeasurementAndUplinkIntervals(const uint16_t measurement_seconds, const uint16_t uplink_seconds);

    void setMqttHost(const std::string& host) { mqttHost = host; }
    void setMqttPort(uint16_t port) { mqttPort = port; }
    void setMqttUser(const std::string& user) { mqttUser = user; }
    void setMqttPassword(const std::string& password) { mqttPassword = password; }

    std::string& getMqttHost() { return mqttHost; }
    uint16_t getMqttPort() { return mqttPort; }
    std::string& getMqttUser() { return mqttUser; }
    std::string& getMqttPassword() { return mqttPassword; }

    void dumpConfig(Stream& stream);

private:
    DeviceConfig();

    // Avoid operations that would make copies of the singleton instance.
    DeviceConfig(const DeviceConfig&) = delete;
    DeviceConfig& operator=(const DeviceConfig&) = delete;

    // How often to read the sensors, in seconds.
    uint16_t measure_interval;

    // How often to uplink the data, in seconds. This must be a multiple of
    // measure_interval.
    uint16_t uplink_interval;

    std::string mqttHost;
    uint16_t mqttPort;
    std::string mqttUser;
    std::string mqttPassword;
};


#endif //WOMBAT_DEVICECONFIG_H
