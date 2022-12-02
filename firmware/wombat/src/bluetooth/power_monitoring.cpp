#include <BLE2904.h>
#include "bluetooth/power_monitoring.h"

/*
 * Notes
 *
 * The battery level service specification only defines the battery level characteristic,
 * and no optional characteristics. So we're not complying with the specification here.
 *
 * When adding descriptors, the descriptor must be be allocated on the heap because addDescriptor()
 * copies the pointer, not the object, so the object must not disappear as it will if allocated on
 * the stack.
 *
 *  0x2901 is the textual description of the characteristic.
 *
 * 0x2902 client characteristic configuration - something to do with read/write/notify, I don't understand it yet.
 * See section 3.3.3.3 of the core specification v5.3,
 * https://www.bluetooth.com/specifications/specs/core-specification-5-3/
 *
 * 0x2904 is the format of the characteristic - data type (eg string, float, uint32) and unit (v, kg, etc).
 *
 * Most BLE clients seem to handle the services & characteristics better if the BLEUUID(uint16_t) form of UUID is
 * used. They are more often able to recognise what is being sent. I cannot think why this would be.
 */
BluetoothPowerService::BluetoothPowerService(BLEServer *server) {

    BLEService* service = server->createService(BATT_SERVICE_UUID);

    // Battery percentage
    BLECharacteristic* batt_level = service->createCharacteristic(BATT_CHAR_LEVEL_UUID, BLECharacteristic::PROPERTY_READ);
//    BLEDescriptor batt_level_desc(BLEUUID((uint16_t)0x2902));
//    batt_level_desc.setValue("Battery Percentage");
//    batt_level->addDescriptor(&batt_level_desc);
    batt_level->setCallbacks(new BluetoothBatteryPercentCallbacks);

    // Battery
    BLECharacteristic* batt_voltage = service->createCharacteristic(
            BATT_VOLTAGE_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor *batt_voltage_desc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    batt_voltage_desc->setValue("Battery Voltage");
    batt_voltage->addDescriptor(batt_voltage_desc);

    BLE2904 *batt_v_pres = new BLE2904();
    batt_v_pres->setFormat(BLE2904::FORMAT_UINT16);
    batt_v_pres->setExponent(-2);      // Multiply the characteristic's value by 10^-2.
    batt_v_pres->setUnit(0x2728u);     // Volts
    batt_v_pres->setNamespace(0x01u);  // Bluetooth SIG - the only valid value
    batt_v_pres->setDescription(0);    // Who knows?
    batt_voltage->addDescriptor(batt_v_pres);

    batt_voltage->setCallbacks(new BluetoothBatteryVoltageCallbacks);

    BLECharacteristic* batt_current = service->createCharacteristic(
            BATT_CURRENT_UUID, BLECharacteristic::PROPERTY_READ);
//    BLEDescriptor batt_current_desc(BLEUUID((uint16_t)0x2902));
//    batt_current_desc.setValue("Battery Current");
//    batt_current->addDescriptor(&batt_current_desc);
    batt_current->setCallbacks(new BluetoothBatteryCurrentCallbacks);

    // Solar
    BLECharacteristic* solar_voltage = service->createCharacteristic(
            SOLAR_VOLTAGE_UUID, BLECharacteristic::PROPERTY_READ);
//    BLEDescriptor solar_voltage_desc(BLEUUID((uint16_t)0x2902));
//    solar_voltage_desc.setValue("Solar Voltage");
//    solar_voltage->addDescriptor(&solar_voltage_desc);
    solar_voltage->setCallbacks(new BluetoothSolarVoltageCallbacks());

    BLECharacteristic* solar_current = service->createCharacteristic(
            SOLAR_CURRENT_UUID, BLECharacteristic::PROPERTY_READ);
//    BLEDescriptor solar_current_desc(BLEUUID((uint16_t)0x2902));
//    solar_current_desc.setValue("Solar Current");
//    solar_current->addDescriptor(&solar_current_desc);
    solar_current->setCallbacks(new BluetoothSolarCurrentCallbacks);

    // Start service
    service->start();

    // Start advertising service
    server->getAdvertising()->addServiceUUID(service->getUUID());
}
