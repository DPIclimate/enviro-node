/**
 * @file device.cpp
 *
 * @brief Setup BluetoothLE device information services and characteristics.
 */
#include "bluetooth/device.h"
#include "DeviceConfig.h"

/**
 * @brief Add read-only characteristic to BluetoothLE service.
 *
 * Attach a characteristic such as device name, revision number etc. to an
 * existing BluetoothLE service.
 *
 * @param service BluetoothLE service to attach characteristic to.
 * @param uuid UUID of characteristic.
 * @param value Value to publish.
 * @return Pointer to the assigned characteristic.
 */
static BLECharacteristic* addReadOnlyCharacteristic(BLEService* service,
                                                    BLEUUID uuid,
                                                    const char* value) {
    BLECharacteristic* pCharacteristic = service->createCharacteristic(
            BLEUUID(uuid), BLECharacteristic::PROPERTY_READ);
    pCharacteristic->setValue(value);
    return pCharacteristic;
}

/**
 * @brief Create and start a device information service on the BluetoothLE
 * server.
 *
 * Creates a bluetooth service for device information and attaches several
 * characteristics to the service. The service is then started and attached
 * to the BluetoothLE server.
 *
 * @param server BluetoothLE server.
 */
BluetoothDeviceService::BluetoothDeviceService(BLEServer *server) {
    BLEService* service = server->createService(DEVICE_SERVICE_UUID);

    // Add device name
    addReadOnlyCharacteristic(service, DEVICE_CHAR_NAME_UUID, DEVICE_NAME);

    // Add device serial number
    addReadOnlyCharacteristic(service, DEVICE_CHAR_SERIAL_NUM_UUID,
                              DeviceConfig::get().node_id);

    // Add device firmware revision number
    addReadOnlyCharacteristic(service, DEVICE_CHAR_FIRMWARE_REV_UUID,
                              DEVICE_FIRMWARE_REV);

    // Add device hardware revision number
    addReadOnlyCharacteristic(service, DEVICE_CHAR_HARDWARE_REV_UUID,
                              DEVICE_HARDWARE_REV);

    // Add device manufacturer ID
    addReadOnlyCharacteristic(service, DEVICE_CHAR_MANUFACTURER_UUID,
                              DEVICE_MANUFACTURER);

    service->start();

    // Attach device service to BluetoothLE server.
    server->getAdvertising()->addServiceUUID(service->getUUID());
}
