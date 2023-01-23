/**
 * @file power_monitoring.h
 *
 * @brief Setup BluetoothLE power monitoring information.
 *
 * Power monitoring includes battery status and solar status. Both are attached
 * to a BluetoothLE power monitoring service and served by the BluetoothLE
 * server.
 *
 * @date December 2022
 */
#ifndef WOMBAT_BLUETOOTH_POWER_MONITORING_H
#define WOMBAT_BLUETOOTH_POWER_MONITORING_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "power_monitoring/solar.h"
#include "power_monitoring/battery.h"

//! Battery service UUID
#define BATT_SERVICE_UUID       (BLEUUID(0x180Fu))

//! Battery charge in percent characteristic UUID
#define BATT_CHAR_LEVEL_UUID    (BLEUUID(0x2A19u))
//! Battery voltage characteristic UUID
#define BATT_VOLTAGE_UUID       (BLEUUID(0x2B18u))
//! Battery current characteristic UUID
#define BATT_CURRENT_UUID       (BLEUUID(0x2AEEu))

//! Solar voltage characteristic UUID
#define SOLAR_VOLTAGE_UUID      (BLEUUID("4af56a5f-5688-4ff6-9008-adcc6c1992e1"))
//! Solar current characteristic UUID
#define SOLAR_CURRENT_UUID      (BLEUUID("a85332c1-5477-4e23-a782-c48443f0dda3"))


/**
 * @brief BluetoothLE power service.
 *
 * Sets up a BluetoothLE power information service to serve on a BluetoothLE
 * server.
 *
 * @note Not all UUID's of characteristics within this service are shared by
 * other BLE clients. The only UUID that is uniform with BLE clients is the
 * battery charge level. Other UUID's are specific to themselves. For more
 * information see below.
 *
 * @see For more information on UUID's and the preassigned UUID's for various
 * services see https://www.bluetooth.com/wp-content/uploads/2022/12/Assigned_Numbers_Released-2022-12-20.pdf
 *
 */
class BluetoothPowerService{
public:
    explicit BluetoothPowerService(BLEServer* server);
    ~BluetoothPowerService() = default;
};


/**
 * @brief Solar voltage BluetoothLE characteristic callback.
 *
 * This callback is used to overwrite BluetoothLE characteristic callback
 * functions in order to service unique information for each characteristic.
 */
class BluetoothSolarVoltageCallbacks: public BLECharacteristicCallbacks {

    /**
     * @brief Write solar voltage value over BLE when requested.
     *
     * This function is called when a client requests its information over BLE.
     * The solar voltage is read, it is converted from a float into a uint8_t
     * array then passed back to the client using the notify() function.
     *
     * @note It is up to the client to transform the array back to a float.
     *
     * @param pCharacteristic Pointer to a BLE characteristic.
     */
    void onRead(BLECharacteristic* pCharacteristic) override{
        float voltage = SolarMonitor::get_voltage();

        char sVolts[10];
        snprintf(sVolts, sizeof(sVolts), "%.2f", voltage);

        pCharacteristic->setValue(sVolts);
        pCharacteristic->notify();
    }
};

/**
 * @brief Solar current BluetoothLE characteristic callback.
 *
 * This callback is used to overwrite BluetoothLE characteristic callback
 * functions in order to service unique information for each characteristic.
 */
class BluetoothSolarCurrentCallbacks: public BLECharacteristicCallbacks {
    /**
     * @brief Write solar current value over BLE when requested.
     *
     * This function is called when a client requests its information over BLE.
     * The solar current is read, it is converted from a float into a uint8_t
     * array then passed back to the client using the notify() function.
     *
     * @note It is up to the client to transform the array back to a float.
     *
     * @param pCharacteristic Pointer to a BLE characteristic.
     */
    void onRead(BLECharacteristic* pCharacteristic) override{
        float current = SolarMonitor::get_current();

        char sCurrent[10]{};
        snprintf(sCurrent, sizeof(sCurrent), "%.2f", current);


        pCharacteristic->setValue(sCurrent);
        pCharacteristic->notify();
    }
};

/**
 * @brief Battery percentage BluetoothLE characteristic callback.
 *
 * This callback is used to overwrite BluetoothLE characteristic callback
 * functions in order to service unique information for each characteristic.
 */
class BluetoothBatteryPercentCallbacks: public BLECharacteristicCallbacks {
    /**
     * @brief Write battery percentage value over BLE when requested.
     *
     * This function is called when a client requests its information over BLE.
     * The battery voltage is read. This is then converted to a percentage of
     * max (4.2 V (hardcoded)). If the battery voltage exceeds 4.2 volts for
     * some reason, the battery voltage will be represented as 100 %. Finally,
     * the battery percentage is set and the client is notified that the
     * characteristic has updated.
     *
     * @note It is up to the client to transform the array back to a float.
     *
     * @param pCharacteristic Pointer to a BLE characteristic.
     */
    void onRead(BLECharacteristic* pCharacteristic) override {
        float max_voltage = 4.2;
        float voltage = BatteryMonitor::get_voltage();
        uint8_t batt_percent = 0;

        if (voltage > 0) {
            batt_percent = (uint8_t)((voltage / max_voltage) * 100);
        }

        if(batt_percent > 100){
            batt_percent = 100;
        }

        char cBatt[10]{};
        snprintf(cBatt, sizeof(cBatt), "%d", batt_percent);

        pCharacteristic->setValue(cBatt);
        pCharacteristic->notify();
    }
};

/**
 * @brief Battery voltage BluetoothLE characteristic callback.
 *
 * This callback is used to overwrite BluetoothLE characteristic callback
 * functions in order to service unique information for each characteristic.
 */
class BluetoothBatteryVoltageCallbacks: public BLECharacteristicCallbacks {
    /**
     * @brief Write battery voltage value over BLE when requested.
     *
     * This function is called when a client requests its information over BLE.
     * The battery voltage is read, it is converted from a float into a uint8_t
     * array then passed back to the client using the notify() function.
     *
     * @note It is up to the client to transform the array back to a float.
     *
     * @param pCharacteristic Pointer to a BLE characteristic.
     */
    void onRead(BLECharacteristic* pCharacteristic) override{
        float voltage = BatteryMonitor::get_voltage();

        char bVolts[10]{};
        snprintf(bVolts, sizeof(bVolts), "%.2f", voltage);

        pCharacteristic->setValue(bVolts);
        pCharacteristic->notify();
    }
};

/**
 * @brief Battery current BluetoothLE characteristic callback.
 *
 * This callback is used to overwrite BluetoothLE characteristic callback
 * functions in order to service unique information for each characteristic.
 */
class BluetoothBatteryCurrentCallbacks: public BLECharacteristicCallbacks {

    /**
     * @brief Write battery current value over BLE when requested.
     *
     * This function is called when a client requests its information over BLE.
     * The battery current is read, it is converted from a float into a uint8_t
     * array then passed back to the client using the notify() function.
     *
     * @note It is up to the client to transform the array back to a float.
     *
     * @param pCharacteristic Pointer to a BLE characteristic.
     */
    void onRead(BLECharacteristic* pCharacteristic) override{
        float current = BatteryMonitor::get_current();

        char bCurrent[10];
        snprintf(bCurrent, sizeof(bCurrent), "%.2f", current);

        pCharacteristic->setValue(current);
        pCharacteristic->notify();
    }
};

#endif //WOMBAT_BLUETOOTH_POWER_MONITORING_H
