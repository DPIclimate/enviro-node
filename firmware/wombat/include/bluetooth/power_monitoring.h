#ifndef WOMBAT_BLUETOOTH_POWER_MONITORING_H
#define WOMBAT_BLUETOOTH_POWER_MONITORING_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "power_monitoring/solar.h"
#include "power_monitoring/battery.h"

#define BATT_SERVICE_UUID               (BLEUUID(0x180Fu))
#define BATT_CHAR_LEVEL_UUID            (BLEUUID(0x2A19u))
#define BATT_VOLTAGE_UUID               (BLEUUID(0x2B18u))
#define BATT_CURRENT_UUID               (BLEUUID(0x2AEEu))
#define SOLAR_VOLTAGE_UUID              (BLEUUID("4af56a5f-5688-4ff6-9008-adcc6c1992e1"))
#define SOLAR_CURRENT_UUID              (BLEUUID("a85332c1-5477-4e23-a782-c48443f0dda3"))

class BluetoothPowerService{
public:
    explicit BluetoothPowerService(BLEServer* server);
    ~BluetoothPowerService() = default;
};

// Solar voltage
class BluetoothSolarVoltageCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override{
        float voltage = SolarMonitor::get_voltage();

        uint16_t voltage_value = (uint16_t)(voltage * 100.0f);
        uint8_t voltage_bytes[2];
        voltage_bytes[0] = voltage_value;
        voltage_bytes[1] = voltage_value >> 8;

        pCharacteristic->setValue(voltage_bytes, 2);
        pCharacteristic->notify();
    }
};

// Solar current
class BluetoothSolarCurrentCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override{
        float current = SolarMonitor::get_current();

        uint16_t current_value = (uint16_t)(current * 100.0f);
        uint8_t current_bytes[2];
        current_bytes[0] = current_value;
        current_bytes[1] = current_value >> 8;

        pCharacteristic->setValue(current_bytes, 2);
        pCharacteristic->notify();
    }
};

// Battery percentage
class BluetoothBatteryPercentCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override {
        float max_voltage = 4.2;
        float voltage = BatteryMonitor::get_voltage();
        uint8_t batt_percent = 0;
        if (voltage > 0) {
            batt_percent = (uint8_t)((voltage / max_voltage) * 100);
        }

        pCharacteristic->setValue(&batt_percent, 1);
        pCharacteristic->notify();
    }
};

// Battery voltage
class BluetoothBatteryVoltageCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override{
        float voltage = BatteryMonitor::get_voltage();

        uint16_t voltage_value = (uint16_t)(voltage * 100.0f);
        uint8_t voltage_bytes[2];
        voltage_bytes[0] = voltage_value;
        voltage_bytes[1] = voltage_value >> 8;

        pCharacteristic->setValue(voltage_bytes, 2);
        pCharacteristic->notify();
    }
};

// Battery current
class BluetoothBatteryCurrentCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override{
        float current = BatteryMonitor::get_current();

        uint16_t current_value = (uint16_t)(current * 100.0f);
        uint8_t current_bytes[2];
        current_bytes[0] = current_value;
        current_bytes[1] = current_value >> 8;

        pCharacteristic->setValue(current_bytes, 2);
        pCharacteristic->notify();
    }
};

#endif //WOMBAT_BLUETOOTH_POWER_MONITORING_H
