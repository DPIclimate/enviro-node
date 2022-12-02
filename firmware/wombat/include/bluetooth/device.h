#ifndef WOMBAT_BLUETOOTH_DEVICE_H
#define WOMBAT_BLUETOOTH_DEVICE_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define DEVICE_NAME                     "Wombat-ESP32"
#define DEVICE_FIRMWARE_REV             "0.1"
#define DEVICE_HARDWARE_REV             "0.1"
#define DEVICE_MANUFACTURER             "DPI-Climate"

#define DEVICE_SERVICE_UUID             (BLEUUID(0x180Au))
#define DEVICE_CHAR_NAME_UUID           (BLEUUID(0x2A00u))
#define DEVICE_CHAR_SERIAL_NUM_UUID     (BLEUUID(0x2A25u))
#define DEVICE_CHAR_FIRMWARE_REV_UUID   (BLEUUID(0x2A26u))
#define DEVICE_CHAR_HARDWARE_REV_UUID   (BLEUUID(0x2A27u))
#define DEVICE_CHAR_MANUFACTURER_UUID   (BLEUUID(0x2A29u))

class BluetoothDeviceService {
public:
    explicit BluetoothDeviceService(BLEServer* server);
    ~BluetoothDeviceService() = default;
};

#endif //WOMBAT_BLUETOOTH_DEVICE_H
