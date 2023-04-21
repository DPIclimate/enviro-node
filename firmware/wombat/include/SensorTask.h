#ifndef WOMBAT_SENSORTASK_H
#define WOMBAT_SENSORTASK_H

#include <ArduinoJson.h>

constexpr size_t MAX_SENSORS = 10;
static constexpr uint8_t MAX_VALUES = 32;

void init_sensors(void);
bool read_sensor(const char addr, JsonArray& timeseries_array);
void sensor_task(void);

#endif //WOMBAT_SENSORTASK_H
