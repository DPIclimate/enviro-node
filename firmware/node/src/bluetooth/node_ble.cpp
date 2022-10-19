#include "bluetooth/node_ble.h"

uint8_t txValue = 0;

void Node_BluetoothLE::begin(){
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
        #error Bluetooth is not enabled.
    #endif

    // Initialise device
    BLEDevice::init(DEVICE_NAME);

    // Construct server on device
    server = BLEDevice::createServer();
    // Set callbacks for server
    server->setCallbacks(new Node_BluetoothLE_Server_Callbacks);

    // Services and characteristics (callbacks incorporated)
    (Node_BLE_UART_Service(server));
    (Node_BLE_Power_Service(server));
    (Node_Config(server));

    // Start server advertising
    server->getAdvertising()->start();

    #ifdef DEBUG
        log_msg("[DEBUG]: Bluetooth started.");
    #endif

    device_connected = false;
    current_device = false;
}


[[noreturn]] void Node_BluetoothLE::read_write_blocking() const{
    #ifdef DEBUG
        log_msg("[DEBUG]: Started read-write Bluetooth method (blocking).");
    #endif

    while(true){
        if(device_connected){

            server->getServiceByUUID(
                    BATT_SERVICE_UUID)->getCharacteristic(
                            BATT_CURRENT_UUID)->getValue();

            server->getServiceByUUID(
                    BATT_SERVICE_UUID)->getCharacteristic(
                            BATT_CURRENT_UUID)->notify();
            delay(10);
        }

        if(!device_connected && current_device){
            server->startAdvertising();
            current_device = device_connected;
        }

        if(device_connected && !current_device){
            current_device = device_connected;
        }
    }
}