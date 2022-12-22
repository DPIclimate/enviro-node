/**
 * @file server.cpp
 *
 * @brief Setup and start BluetoothLE server.
 *
 * @date December 2022
 */
#include "bluetooth/server.h"

//! BluetoothLE service object
static BLEServer* server;

/**
 * @brief Start bluetooth server and attach several information services.
 *
 * Bluetooth must be enabled on the device. This is checked on startup. A
 * device instance is created using the DEVICE_NAME constant found in
 * device.h. The BLE server is created and several callbacks based on various
 * services are attached to the server. Advertising is then enabled to allow
 * clients to connect to the device.
 */
void BluetoothServer::begin(){
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
        #error Bluetooth is not enabled.
    #endif

    // Initialise device
    BLEDevice::init(DEVICE_NAME);

    // Construct server on device
    server = BLEDevice::createServer();
    // Set callbacks for server
    server->setCallbacks(new BluetoothServerCallbacks());

    // Services and characteristics (callbacks incorporated)
    // It is ok these are created on the stack because
    // the constructors do some heap allocations and pass those objects into the BLE
    // stack. So the constructors are just side-effect generators and do not need to live on.
    BluetoothUartService s1(server); // UART communications
    BluetoothDeviceService s2(server); // Device information
    BluetoothPowerService s3(server); // Device power (battery & solar)

    // Start server advertising
    server->getAdvertising()->start();

    // Device is not connected on startup
    device_connected = false;
}

/**
 * @brief Check if device is connected over BluetoothLE.
 *
 * @return Device connected status, true = connected.
 */
bool BluetoothServer::is_device_connected() {
    return device_connected;
}

/**
 * @brief Notify device over BLE that a UART message is available.
 *
 * This method allows for CLI messages and other log output to be transmitted
 * over BLEUart rather than just to hardware UART.
 *
 * @param message Message to send to BLE client.
 */
void BluetoothServer::notify_device(const char* message) {
    if (device_connected) {
        server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
                UART_CHAR_TX_UUID)->setValue(message);
        server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
                UART_CHAR_TX_UUID)->notify();
    }
}
