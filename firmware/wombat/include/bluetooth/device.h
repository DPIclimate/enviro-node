/**
 * @file device.h
 *
 * @brief Bluetooth device services and device information.
 *
 * @date December 2022
 */
#ifndef WOMBAT_BLUETOOTH_DEVICE_H
#define WOMBAT_BLUETOOTH_DEVICE_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define DEVICE_NAME                     "Wombat-ESP32"
#define DEVICE_FIRMWARE_REV             "0.1"
#define DEVICE_HARDWARE_REV             "1.0"
#define DEVICE_MANUFACTURER             "DPI-Climate"

#define DEVICE_SERVICE_UUID             (BLEUUID(0x180Au))
#define DEVICE_CHAR_NAME_UUID           (BLEUUID(0x2A00u))
#define DEVICE_CHAR_SERIAL_NUM_UUID     (BLEUUID(0x2A25u))
#define DEVICE_CHAR_FIRMWARE_REV_UUID   (BLEUUID(0x2A26u))
#define DEVICE_CHAR_HARDWARE_REV_UUID   (BLEUUID(0x2A27u))
#define DEVICE_CHAR_MANUFACTURER_UUID   (BLEUUID(0x2A29u))

/**
 * @brief Bluetooth device service. Enable device information to be shared over
 * BluetoothLE.
 *
 * Advertises device information over BluetoothLE when enabled. This includes
 * things such as the device name, firmware and hardware revisions. These
 * services are advertised using specific UUID's that correspond to client
 * UUID's. This enables BluetoothLE clients to identify a device and its
 * metadata via shared UUID's.
 *
 * @see For more information on UUID's and the preassigned UUID's for various
 * services see https://www.bluetooth.com/wp-content/uploads/2022/12/Assigned_Numbers_Released-2022-12-20.pdf
 */
class BluetoothDeviceService {
public:
    explicit BluetoothDeviceService(BLEServer* server);
    ~BluetoothDeviceService() = default;
};

#endif //WOMBAT_BLUETOOTH_DEVICE_H
