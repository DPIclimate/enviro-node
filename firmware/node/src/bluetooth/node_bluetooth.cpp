#include "bluetooth/node_bluetooth.h"

static bool initialised = false;
uint8_t txValue;

void Node_BluetoothLE::begin(){
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
        #error Bluetooth is not enabled.
    #endif

    /* Device */
    // Initialise device
    bt_handler.device->init("Enviro-Node");

    /* Server */
    // Construct server on device
    bt_handler.server = BLEDevice::createServer();
    // Set callbacks for server
    bt_handler.server->setCallbacks(new Node_BluetoothLE_Server_Callbacks);

    /* Services */
    // New service on the server
    bt_handler.service = bt_handler.server->createService(SERVICE_UUID);

    /* Characteristics */
    // TX characteristic
    bt_handler.TxCharacteristic = bt_handler
            .service->createCharacteristic(CHARACTERISTIC_UUID_TX,
                                           BLECharacteristic::PROPERTY_NOTIFY);
    bt_handler.TxCharacteristic->addDescriptor(new BLE2902());

    // RX characteristic
    bt_handler.RxCharacteristic = bt_handler
            .service->createCharacteristic(CHARACTERISTIC_UUID_RX,
                                           BLECharacteristic::PROPERTY_WRITE);

    // Set callbacks for a characteristic
    bt_handler.RxCharacteristic->setCallbacks(
            new Node_BluetoothLE_Characteristic_Callbacks);


    // Start service
    bt_handler.service->start();

    // Start server with UUID
    bt_handler.server->getAdvertising()->addServiceUUID(
            bt_handler.service->getUUID());
    bt_handler.server->getAdvertising()->start();

    #ifdef DEBUG
        log_msg("[DEBUG]: Bluetooth started.");
    #endif

    device_connected = false;
    current_device = false;
    initialised = true;
}


[[noreturn]] void Node_BluetoothLE::read_write_blocking() const{
    #ifdef DEBUG
        log_msg("[DEBUG]: Started read-write Bluetooth method (blocking).");
    #endif

    while(true){
        if(device_connected){
            bt_handler.TxCharacteristic->setValue(&txValue, 1);
            bt_handler.TxCharacteristic->notify();
            txValue++;
            delay(100);
        }

        if(!device_connected && current_device){
            bt_handler.server->startAdvertising();
            current_device = device_connected;
        }

        if(device_connected && !current_device){
            current_device = device_connected;
        }
    }
}