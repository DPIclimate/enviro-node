#include "bluetooth/server.h"

static BLEServer* server;
static char message_buffer[UINT8_MAX];

void BluetoothServer::begin(){
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
        #error Bluetooth is not enabled.
    #endif

    // Initialise device
    BLEDevice::init("ESP32 Wombat");

    // Construct server on device
    server = BLEDevice::createServer();
    // Set callbacks for server
    server->setCallbacks(new BluetoothServerCallbacks);

    // Services and characteristics (callbacks incorporated)
    (BluetoothUartService(server)); // UART communications
    (BluetoothDeviceService(server)); // Device information
    (BluetoothPowerService(server)); // Device power (battery & solar)

    // Start server advertising
    server->getAdvertising()->start();

    device_connected = false;
}

bool BluetoothServer::is_device_connected(){
    return device_connected;
}

void BluetoothServer::notify_device(const char* message){
    if(device_connected){
        server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
                UART_CHAR_TX_UUID)->setValue(message);
        server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
                UART_CHAR_TX_UUID)->notify();
    } else {
        Serial.println("ERROR no Bluetooth device connected.");
    }
}