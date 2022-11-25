#ifndef WOMBAT_BLUETOOTH_DEVICE_H
#define WOMBAT_BLUETOOTH_DEVICE_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define DEVICE_NAME                     "Wombat-ESP32"
#define DEVICE_SERIAL                   "1_2022"
#define DEVICE_FIRMWARE_REV             "0.1"
#define DEVICE_HARDWARE_REV             "0.1"
#define DEVICE_MANUFACTURER             "DPI-Climate"

#define DEVICE_SERVICE_UUID             "0000180A-0000-1000-8000-00805F9B34FB"
#define DEVICE_CHAR_NAME_UUID           "00002A00-0000-1000-8000-00805F9B34FB"
#define DEVICE_CHAR_SERIAL_NUM_UUID     "00002A25-0000-1000-8000-00805F9B34FB"
#define DEVICE_CHAR_FIRMWARE_REV_UUID   "00002A26-0000-1000-8000-00805F9B34FB"
#define DEVICE_CHAR_HARDWARE_REV_UUID   "00002A27-0000-1000-8000-00805F9B34FB"
#define DEVICE_CHAR_MANUFACTURER_UUID   "00002A29-0000-1000-8000-00805F9B34FB"

class BluetoothDeviceService {
public:
    explicit BluetoothDeviceService(BLEServer* server);
    ~BluetoothDeviceService() = default;
};

#endif //WOMBAT_BLUETOOTH_DEVICE_H
