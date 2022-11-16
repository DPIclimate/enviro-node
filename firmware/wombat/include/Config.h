#ifndef WOMBAT_CONFIG_H
#define WOMBAT_CONFIG_H

#include <Arduino.h>

class Config {
public:
    static constexpr size_t MAX_CONFIG_STR = 32;

    uint8_t mac[6];
    char node_id[13];
    char mqtt_topic_template[MAX_CONFIG_STR+1];

    static Config& get() {
        static Config instance;
        return instance;
    }

    void reset();
    void load();
    void save();

    uint16_t getMeasureInterval() { return measure_interval; }

    void setMeasureInterval(const uint16_t minutes);

    uint16_t getUplinkInterval() { return uplink_interval; }

    void setUplinkInterval(const uint16_t minutes);

    void setMeasurementAndUplinkIntervals(const uint16_t measurement_seconds, const uint16_t uplink_seconds);

    void dumpConfig(Stream& stream);

private:
    Config();

    // Avoid operations that would make copies of the singleton instance.
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // How often to read the sensors, in seconds.
    uint16_t measure_interval;

    // How often to uplink the data, in seconds. This must be a multiple of
    // measure_interval.
    uint16_t uplink_interval;
};


#endif //WOMBAT_CONFIG_H
