#include "bluetooth/device.h"
#include "DeviceConfig.h"

static BLECharacteristic* addROCharacteristic(BLEService* service, BLEUUID uuid, const char* value) {
    BLECharacteristic* pCharacteristic = service->createCharacteristic(BLEUUID(uuid), BLECharacteristic::PROPERTY_READ);
    pCharacteristic->setValue(value);
    return pCharacteristic;
}

BluetoothDeviceService::BluetoothDeviceService(BLEServer *server) {
    BLEService* service = server->createService(DEVICE_SERVICE_UUID);
    addROCharacteristic(service, DEVICE_CHAR_NAME_UUID, DEVICE_NAME);
    addROCharacteristic(service, DEVICE_CHAR_SERIAL_NUM_UUID, DeviceConfig::get().node_id);
    addROCharacteristic(service, DEVICE_CHAR_FIRMWARE_REV_UUID, DEVICE_FIRMWARE_REV);
    addROCharacteristic(service, DEVICE_CHAR_HARDWARE_REV_UUID, DEVICE_HARDWARE_REV);
    addROCharacteristic(service, DEVICE_CHAR_MANUFACTURER_UUID, DEVICE_MANUFACTURER);

    service->start();

    server->getAdvertising()->addServiceUUID(service->getUUID());
}
