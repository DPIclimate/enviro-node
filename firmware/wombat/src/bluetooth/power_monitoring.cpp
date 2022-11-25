#include "bluetooth/power_monitoring.h"


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

    // Battery
    BLECharacteristic* batt_voltage = service->createCharacteristic(
            BATT_VOLTAGE_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor batt_voltage_desc(BLEUUID((uint16_t)0x2902));
    batt_voltage_desc.setValue("Battery Voltage");
    batt_voltage->addDescriptor(&batt_voltage_desc);
    batt_voltage->setCallbacks(new BluetoothBatteryVoltageCallbacks);

    BLECharacteristic* batt_current = service->createCharacteristic(
            BATT_CURRENT_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor batt_current_desc(BLEUUID((uint16_t)0x2902));
    batt_current_desc.setValue("Battery Current");
    batt_current->addDescriptor(&batt_current_desc);
    batt_current->setCallbacks(new BluetoothBatteryCurrentCallbacks);

    // Solar
    BLECharacteristic* solar_voltage = service->createCharacteristic(
            SOLAR_VOLTAGE_UUID, BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor solar_voltage_desc(BLEUUID((uint16_t)0x2902));
    solar_voltage_desc.setValue("Solar Voltage");
    solar_voltage->addDescriptor(&solar_voltage_desc);
    solar_voltage->setCallbacks(new BluetoothSolarVoltageCallbacks());

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