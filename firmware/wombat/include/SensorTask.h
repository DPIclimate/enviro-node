#ifndef WOMBAT_SENSORTASK_H
#define WOMBAT_SENSORTASK_H

constexpr size_t MAX_SENSORS = 10;
static constexpr uint8_t MAX_VALUES = 32;

void initSensors(void);

void sensorTask(void);

#endif //WOMBAT_SENSORTASK_H
