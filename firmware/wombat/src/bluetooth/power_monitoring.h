#ifndef WOMBAT_BLUETOOTH_POWER_MONITORING_H
#define WOMBAT_BLUETOOTH_POWER_MONITORING_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "power_monitoring/solar.h"
#include "power_monitoring/battery.h"

#define BATT_SERVICE_UUID               "0000180F-0000-1000-8000-00805F9B34FB"
#define BATT_CHAR_LEVEL_UUID            "00002A19-0000-1000-8000-00805F9B34FB"
#define BATT_VOLTAGE_UUID               "00002B18-0000-1000-8000-00805F9B34FB"
#define BATT_CURRENT_UUID               "00002AEE-0000-1000-8000-00805F9B34FB"
#define SOLAR_VOLTAGE_UUID              "4af56a5f-5688-4ff6-9008-adcc6c1992e1"
#define SOLAR_CURRENT_UUID              "a85332c1-5477-4e23-a782-c48443f0dda3"

class BluetoothPowerService{
public:
    explicit BluetoothPowerService(BLEServer* server);
    ~BluetoothPowerService() = default;
};

// Solar voltage
class BluetoothSolarVoltageCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override{
        float voltage = SolarPowerMonitoring::get_voltage();

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
        float current = SolarPowerMonitoring::get_current();

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
    void onRead(BLECharacteristic* pCharacteristic) override{
        float max_voltage = 4.2;
        float voltage = BatteryPowerMonitoring::get_voltage();
        uint8_t batt_percent = 0;
        if(voltage > 0){
            batt_percent = (uint8_t)((voltage / max_voltage) * 100);
        }

        pCharacteristic->setValue(&batt_percent, 1);
        pCharacteristic->notify();
    }
};

// Battery voltage
class BluetoothBatteryVoltageCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override{
        float voltage = BatteryPowerMonitoring::get_voltage();

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
        float current = BatteryPowerMonitoring::get_current();

        uint16_t current_value = (uint16_t)(current * 100.0f);
        uint8_t current_bytes[2];
        current_bytes[0] = current_value;
        current_bytes[1] = current_value >> 8;

        pCharacteristic->setValue(current_bytes, 2);
        pCharacteristic->notify();
    }
};

#endif //WOMBAT_BLUETOOTH_POWER_MONITORING_H
