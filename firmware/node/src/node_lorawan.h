#ifndef NODE_LORAWAN_H
#define NODE_LORAWAN_H

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "node_config.h"


class Node_LoRaWAN {

public:
    Node_LoRaWAN();
    ~Node_LoRaWAN();

    static void do_send(osjob_t* tx_job,
                 const int8_t* payload,
                 uint8_t payload_size,
                 uint8_t port);

    static void do_send(const int8_t* payload,
                        uint8_t payload_size,
                        uint8_t port);

    static bool state();

};

#endif //NODE_LORAWAN_H
