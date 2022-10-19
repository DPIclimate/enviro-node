#include "node_config.h"

Node_Config::Node_Config(BLEServer* server){

    BLEService* service = server->createService(DEVICE_SERVICE_UUID);

    BLECharacteristic* device_name = service->createCharacteristic(
            DEVICE_CHAR_NAME_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor device_name_desc(BLEUUID((uint16_t)0x2902));
    device_name_desc.setValue("Device Name");
    device_name->addDescriptor(&device_name_desc);
    device_name->setValue(DEVICE_NAME);

    BLECharacteristic* serial_num = service->createCharacteristic(
            DEVICE_CHAR_SERIAL_NUM_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor serial_num_desc(BLEUUID((uint16_t)0x2902));
    serial_num_desc.setValue("Serial Number");
    serial_num->addDescriptor(&serial_num_desc);
    serial_num->setValue(DEVICE_SERIAL);

    BLECharacteristic* firmware_rev = service->createCharacteristic(
            DEVICE_CHAR_FIRMWARE_REV_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor firmware_rev_desc(BLEUUID((uint16_t)0x2902));
    firmware_rev_desc.setValue("Firmware Rev.");
    firmware_rev->addDescriptor(&firmware_rev_desc);
    firmware_rev->setValue(DEVICE_FIRMWARE_REV);

    BLECharacteristic* hardware_rev = service->createCharacteristic(
            DEVICE_CHAR_HARDWARE_REV_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor hardware_rev_desc(BLEUUID((uint16_t)0x2902));
    hardware_rev_desc.setValue("Hardware Rev.");
    hardware_rev->addDescriptor(&hardware_rev_desc);
    hardware_rev->setValue(DEVICE_HARDWARE_REV);

    BLECharacteristic* manufacturer = service->createCharacteristic(
            DEVICE_CHAR_MANUFACTURER_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor manufacturer_des(BLEUUID((uint16_t)0x2902));
    hardware_rev_desc.setValue("Manufacturer");
    manufacturer->addDescriptor(&manufacturer_des);
    manufacturer->setValue(DEVICE_MANUFACTURER);

    service->start();

    server->getAdvertising()->addServiceUUID(service->getUUID());
}
