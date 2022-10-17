#ifndef NODE_LORAWAN_H
#define NODE_LORAWAN_H

#include "lmic.h"
#include "hal/hal.h"
#include "SPI.h"
#include "node_config.h"


class Node_LoRaWAN {

public:
    Node_LoRaWAN() = default;
    ~Node_LoRaWAN() = default;


    static void begin();

    static void do_send(osjob_t* j,
                        const int8_t* payload,
                        uint8_t payload_size,
                        uint8_t port = 1);

    static void do_send(const int8_t* payload,
                        uint8_t payload_size,
                        uint8_t port = 1);

    static bool state();

};

#endif //NODE_LORAWAN_H
