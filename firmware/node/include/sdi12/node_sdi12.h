#ifndef NODE_SDI12_H
#define NODE_SDI12_H

#include "bluetooth/node_ble.h"
#include <esp32-sdi12.h>

#define SDI12_DATA_PIN D0

class Node_SDI12 : public ESP32_SDI12 {
public:
    Node_SDI12() : ESP32_SDI12(SDI12_DATA_PIN){
        begin();
    }
    ~Node_SDI12() = default;
    static void test_read();

};

class Node_SDI12_Callbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if(rxValue.length() >= 2){
            // Do something
        }
    }
};


#endif //NODE_SDI12_H
