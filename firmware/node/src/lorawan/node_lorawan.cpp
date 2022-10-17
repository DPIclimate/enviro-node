#include "lorawan/node_lorawan.h"

static bool msg_state = false;
static osjob_t sendjob;

void Node_LoRaWAN::begin(){
    os_init();
    LMIC_reset();
    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7,14);
    LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);

    #ifdef DEBUG
        log_msg("[DEBUG]: LoRaWAN startup complete.");
    #endif
}


void Node_LoRaWAN::do_send(osjob_t* j,
                           const int8_t* payload,
                           const uint8_t payload_size,
                           const uint8_t port){
    msg_state = false;

    if (LMIC.opmode & OP_TXRXPEND){
        #ifdef DEBUG
        log_msg("[ERROR]: LoRaWAN job could not be completed. Transaction "
                "already pending completion.");
        #endif
    } else {
        LMIC_setTxData2(port, (uint8_t*)payload, payload_size, 0);
        #ifdef DEBUG
            log_msg("[DEBUG]: LoRaWAN payload queued.");
        #endif
    }
}

void Node_LoRaWAN::do_send(const int8_t* payload,
                           const uint8_t payload_size,
                           const uint8_t port) {
    do_send(&sendjob, payload, payload_size, port);

    while(!Node_LoRaWAN::state()){
        os_runloop_once();
    }

}

bool Node_LoRaWAN::state(){
    return  msg_state;
}

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINING:
            digitalWrite(LED_BUILTIN, HIGH);
            #ifdef DEBUG
                log_msg("[DEBUG]: EV_JOINING");
            #endif
            break;

        case EV_JOINED:
            digitalWrite(LED_BUILTIN, LOW);
            LMIC_setLinkCheckMode(0);
            #ifdef DEBUG
                log_msg("[DEBUG]: EV_JOINED");
            #endif
            break;

        case EV_JOIN_FAILED:
            #ifdef DEBUG
                log_msg("[DEBUG]: EV_JOIN_FAILED");
            #endif
            break;

        case EV_TXCOMPLETE:
            msg_state = true;
            digitalWrite(LED_BUILTIN, LOW);
            #ifdef DEBUG
                log_msg("[DEBUG]: EV_TXCOMPLETE (inc. waiting for RX windows)");
            #endif
            break;

        case EV_TXSTART:
            digitalWrite(LED_BUILTIN, HIGH);
            #ifdef DEBUG
                log_msg("[DEBUG]: EV_TXSTART");
            #endif
            break;

        case EV_JOIN_TXCOMPLETE:
            #ifdef DEBUG
                log_msg("[DEBUG]: EV_JOIN_TXCOMPLETE: no JoinAccept");
            #endif
            break;

        default:
            #ifdef DEBUG
                log_msg("[ERROR]: EV_UNKNOWN (error code %d)", ev);
            #endif
            break;
    }
}

/*
 * Pins used for SPI interface with LoRaWAN radio.
 * RST = Reset pin - G1
 * NSS = Chip select - G0
 * DIO = Radio pins - (DIO0 - G2), (DIO1 - G3), (DIO2 - G4)
 * Not shown = MOSI, MISO & SCK
 */
const lmic_pinmap lmic_pins = {
        .nss = G0,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = G1,
        .dio = { G2, G3, G4 },
        .rxtx_rx_active = 0,
        .rssi_cal = 8,
        .spi_freq = 8000000,
};

void os_getArtEui (uint8_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (uint8_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (uint8_t* buf) { memcpy_P(buf, APPKEY, 16);}

