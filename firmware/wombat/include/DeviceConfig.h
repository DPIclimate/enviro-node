/**
 * @file DeviceConfig.h
 *
 * @brief Node configuration options
 *
 * @date December 2022
 */
#ifndef WOMBAT_DEVICE_CONFIG_H
#define WOMBAT_DEVICE_CONFIG_H

#include <Arduino.h>
#include <ArduinoJSON.h>

/**
 * @brief Node configuration options for both getting and setting values.
 *
 * Handles all device configuration commands. This includes measurement and
 * uplink intervals, MQTT configuration and SDI-12 configuration. Values are
 * stored in SPIFFS storage.
 */
class DeviceConfig {
public:
    //! Maximum length of a configuration string
    static constexpr size_t MAX_CONFIG_STR = 32;

    // mac and node_id are populated in the constructor.
    uint8_t mac[6];
    char node_id[13];
    char mqtt_topic_template[MAX_CONFIG_STR+1];

    /**
     * @brief Provides an instance to other functions (MQTT and uplink
     * intervals) to access (set, get) device configuration.
     *
     * @return A static instance of the current device configuration.
     */
    static DeviceConfig& get() {
        static DeviceConfig instance;
        return instance;
    }

    //! Reset device configuration values back to their default.
    void reset();

    //! Load device configuration values from SPIFFS storage.
    void load();

    //! Save device configuration values to SPIFFS storage.
    void save();

    uint32_t getBootCount(void);

    uint16_t getMeasureInterval() { return measure_interval; }

    void setMeasureInterval(const uint16_t minutes);

    uint16_t getUplinkInterval() { return uplink_interval; }

    void setUplinkInterval(const uint16_t minutes);

    void setMeasurementAndUplinkIntervals(const uint16_t measurement_seconds,
                                          const uint16_t uplink_seconds);

    //! Set the MQTT hostname
    void setMqttHost(const std::string& host) { mqttHost = host; }
    //! Set the MQTT port
    void setMqttPort(uint16_t port) { mqttPort = port; }
    //! Set the MQTT broker username
    void setMqttUser(const std::string& user) { mqttUser = user; }
    //! Set the MQTT broker password
    void setMqttPassword(const std::string& password) { mqttPassword = password; }

    //! Get the MQTT hostname
    std::string& getMqttHost() { return mqttHost; }
    //! Get the MQTT port
    uint16_t getMqttPort() { return mqttPort; }
    //! Get the MQTT broker username
    std::string& getMqttUser() { return mqttUser; }
    //! Get the MQTT broker password
    std::string& getMqttPassword() { return mqttPassword; }

    float getSleepAdjustment() { return sleep_adjustment; }
    void setSleepAdjustment(float _sleep_adjustment) {
        sleep_adjustment = _sleep_adjustment;
    }

    void dumpConfig(Stream& stream);

    const JsonDocument& getSDI12Defns(void);

    static const char* getMsgFilePrefix(void) {
        return "msg_";
    }

private:
    DeviceConfig();

    //! Avoid operations that would make copies of the singleton instance.
    DeviceConfig(const DeviceConfig&) = delete;
    DeviceConfig& operator=(const DeviceConfig&) = delete;

    //! Multiplier for the sleep time to account for clock speed variations.
    float sleep_adjustment = 1.005;
    //! How often to read the sensors, in seconds.
    uint16_t measure_interval;
    //! How often to uplink the data, in seconds.
    uint16_t uplink_interval;
    //! MQTT hostname
    std::string mqttHost;
    //! MQTT port
    uint16_t mqttPort;
    //! MQTT broker username
    std::string mqttUser;
    //! MQTT broker password
    std::string mqttPassword;

    //DynamicJsonDocument sdi12Defns;
};


#endif //WOMBAT_DEVICE_CONFIG_H
