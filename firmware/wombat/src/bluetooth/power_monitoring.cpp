/**
 * @file power_monitoring.cpp
 *
 * @brief BluetoothLE power monitoring service setup and attached to BLE server.
 */
#include <BLE2904.h>
#include "bluetooth/power_monitoring.h"

/**
 * @brief BluetoothLE service for power information.
 *
 * Sets up and attaches the BLE power information service to the BLE server.
 * Several characteristics are attached see power_monitoring.h for a list of
 * these characteristics.
 *
 * @note When adding descriptors, the descriptor must be be allocated on the
 * heap because addDescriptor() copies the pointer, not the object, so the
 * object must not disappear as it will if allocated on the stack.
 *
 * @see https://www.bluetooth.com/specifications/specs/core-specification-5-3/
 *
 * @todo
 * Ensure this method is working and add descriptors for voltage according
 * to the bluetooth specification.
 *
 * @param server Pointer to the BluetoothLE server.
 */
BluetoothPowerService::BluetoothPowerService(BLEServer *server) {

    BLEService* service = server->createService(BATT_SERVICE_UUID);

    // Battery percentage
    BLECharacteristic* batt_level = service->createCharacteristic(
            BATT_CHAR_LEVEL_UUID, BLECharacteristic::PROPERTY_READ |
                                  BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor batt_level_desc(BLEUUID((uint16_t)0x2902));
    batt_level_desc.setValue("Battery Percentage");
    batt_level->addDescriptor(&batt_level_desc);
    batt_level->setCallbacks(new BluetoothBatteryPercentCallbacks);

    // Battery voltage
    BLECharacteristic* batt_voltage = service->createCharacteristic(
            BATT_VOLTAGE_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor batt_voltage_desc(BLEUUID((uint16_t)0x2902));
    batt_voltage_desc.setValue("Battery Voltage");
    batt_voltage->addDescriptor(&batt_voltage_desc);
    batt_voltage->setCallbacks(new BluetoothBatteryVoltageCallbacks);

    // Battery current
    BLECharacteristic* batt_current = service->createCharacteristic(
            BATT_CURRENT_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor batt_current_desc(BLEUUID((uint16_t)0x2902));
    batt_current_desc.setValue("Battery Current");
    batt_current->addDescriptor(&batt_current_desc);
    batt_current->setCallbacks(new BluetoothBatteryCurrentCallbacks);

    // Solar voltage
    BLECharacteristic* solar_voltage = service->createCharacteristic(
            SOLAR_VOLTAGE_UUID, BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor solar_voltage_desc(BLEUUID((uint16_t)0x2902));
    solar_voltage_desc.setValue("Solar Voltage");
    solar_voltage->addDescriptor(&solar_voltage_desc);
    solar_voltage->setCallbacks(new BluetoothSolarVoltageCallbacks);

    // Solar current
    BLECharacteristic* solar_current = service->createCharacteristic(
            SOLAR_CURRENT_UUID, BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor solar_current_desc(BLEUUID((uint16_t)0x2902));
    solar_current_desc.setValue("Solar Current");
    solar_current->addDescriptor(&solar_current_desc);
    solar_current->setCallbacks(new BluetoothSolarCurrentCallbacks);

    // Start service
    service->start();

    // Start advertising service
    server->getAdvertising()->addServiceUUID(service->getUUID());
}
