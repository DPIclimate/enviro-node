/**
 * @file server.h
 *
 * @brief Handle the setup and maintenance of the BLE server and connected
 * clients.
 *
 * @date December 2022
 */
#ifndef WOMBAT_BLUETOOTH_SERVER_H
#define WOMBAT_BLUETOOTH_SERVER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "bluetooth/uart.h"
#include "bluetooth/device.h"
#include "bluetooth/power_monitoring.h"
#include "cli/CLI.h"

class BluetoothServer {
public:
    static void begin();

    inline static bool device_connected = false;

    static bool is_device_connected();
    static void notify_device(const char* message);

};

class BluetoothServerCallbacks: public BLEServerCallbacks,
                                public BluetoothServer {
    /**
     * @brief Occurs client connects to BLE server.
     *
     * Plays the connected tone for audible output. Sets the device_connected
     * variable to true.
     *
     * @param pServer Pointer to the BLE server.
     */
    void onConnect(BLEServer* pServer) override {
        completed_tone();
        device_connected = true;
    }

    /**
     * @brief Occurs client disconnects from BLE server.
     *
     * Plays the disconnected tone for audible output. Sets the device_connected
     * variable to false.
     *
     * @param pServer Pointer to the BLE server.
     */
    void onDisconnect(BLEServer* pServer) override {
        error_tone();
        device_connected = false;
    }
};

#endif //WOMBAT_BLUETOOTH_SERVER_H
