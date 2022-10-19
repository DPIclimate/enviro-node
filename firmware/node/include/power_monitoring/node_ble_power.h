#ifndef NODE_BLE_POWER_H
#define NODE_BLE_POWER_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "node_config.h"
#include "node_power_monitoring.h"
#include "bluetooth/node_ble_config.h"

class Node_BLE_Power_Service {
public:
    explicit Node_BLE_Power_Service(BLEServer* server);
    ~Node_BLE_Power_Service() = default;
};


class Node_BLE_Battery_Power_Callbacks: public BLECharacteristicCallbacks {

    void onRead(BLECharacteristic* pCharacteristic) override{
        // Battery percentage
        float max_voltage = 4.2;
        float voltage = Node_PowerMonitoring::getBatteryVoltage();
        uint8_t batt_percent = 0;
        if(voltage > 0){
            batt_percent = (uint8_t)((voltage / max_voltage) * 100);
        }

        pCharacteristic->setValue(&batt_percent, 1);
        pCharacteristic->notify();
    }

};

class Node_BLE_Battery_Voltage_Callbacks: public BLECharacteristicCallbacks {

    void onRead(BLECharacteristic* pCharacteristic) override{
        // Battery percentage
        float voltage = Node_PowerMonitoring::getBatteryVoltage();

        uint16_t voltage_value = (uint16_t)(voltage * 100.0f);
        uint8_t voltage_bytes[2];
        voltage_bytes[0] = voltage_value;
        voltage_bytes[1] = voltage_value >> 8;

        pCharacteristic->setValue(voltage_bytes, 2);
        pCharacteristic->notify();
    }

};

class Node_BLE_Battery_Current_Callbacks: public BLECharacteristicCallbacks {

    void onRead(BLECharacteristic* pCharacteristic) override{
        // Battery percentage
        float current = Node_PowerMonitoring::getBatteryCurrent();

        uint16_t current_value = (uint16_t)(current * 100.0f);
        uint8_t current_bytes[2];
        current_bytes[0] = current_value;
        current_bytes[1] = current_value >> 8;

        pCharacteristic->setValue(current_bytes, 2);
        pCharacteristic->notify();
    }

};


class Node_BLE_Solar_Voltage_Callbacks: public BLECharacteristicCallbacks {

    void onRead(BLECharacteristic* pCharacteristic) override{
        // Battery percentage
        float voltage = Node_PowerMonitoring::getSolarVoltage();

        uint16_t voltage_value = (uint16_t)(voltage * 100.0f);
        uint8_t voltage_bytes[2];
        voltage_bytes[0] = voltage_value;
        voltage_bytes[1] = voltage_value >> 8;

        pCharacteristic->setValue(voltage_bytes, 2);
        pCharacteristic->notify();
    }

};

class Node_BLE_Solar_Current_Callbacks: public BLECharacteristicCallbacks {

    void onRead(BLECharacteristic* pCharacteristic) override{
        // Battery percentage
        float current = Node_PowerMonitoring::getSolarCurrent();

        uint16_t current_value = (uint16_t)(current * 100.0f);
        uint8_t current_bytes[2];
        current_bytes[0] = current_value;
        current_bytes[1] = current_value >> 8;

        pCharacteristic->setValue(current_bytes, 2);
        pCharacteristic->notify();
    }

};

#endif //NODE_BLE_POWER_H
