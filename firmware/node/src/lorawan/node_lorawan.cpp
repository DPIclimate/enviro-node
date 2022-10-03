#include "node_lorawan.h"

u1_t PROGMEM APPEUI[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x01 };

u1_t PROGMEM DEVEUI[8] = { 0x51, 0x58, 0x04, 0xD0, 0x7E, 0xD5,
                            0xB3, 0x70 };

u1_t PROGMEM APPKEY[16] = { 0xF5, 0x07, 0xB7, 0x41, 0xF1, 0x7D, 0x47, 0x8C, 0x84, 0x93, 0x73, 0x8F, 0x24, 0xB2, 0x54, 0xCB };

static bool msg_state = false;
static osjob_t lmic_job;

Node_LoRaWAN::Node_LoRaWAN() {
    #ifdef DEBUG
        log_msg("[DEBUG]: LoRaWAN startup.");
    #endif

    os_init();
    LMIC_reset(); // Clear LMIC.osjob
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); // Plus minus 1% error
    LMIC_setDrTxpow(DR_SF7, 14); // Startup spreading factor

    // Disable adaptive data rate and link-check mode
    LMIC_setLinkCheckMode(0);
    LMIC_setAdrMode(0); // Disable adaptive data rate

    #ifdef DEBUG
        log_msg("[DEBUG]: LoRaWAN startup complete.");
    #endif
}

Node_LoRaWAN::~Node_LoRaWAN() = default;

void Node_LoRaWAN::do_send(osjob_t* tx_job,
                           const int8_t* payload,
                           const uint8_t payload_size,
                           const uint8_t port){

    msg_state = false;

    #ifdef DEBUG
        log_msg("[DEBUG]: Queuing payload to send over LoRaWAN.");
    #endif

    // Check if RX or TX event is pending
    if(LMIC.opmode & OP_TXRXPEND) {
        #ifdef DEBUG
            log_msg("[ERROR]: Job could not be completed. Transaction "
                    "already pending completion.");
        #endif
    } else {
        LMIC_setTxData2(port, (uint8_t*)payload, payload_size, 0);
        #ifdef DEBUG
            log_msg("[DEBUG]: LoRaWAN payload successfully queued.");
        #endif
    }
}

void Node_LoRaWAN::do_send(const int8_t* payload,
                           const uint8_t payload_size,
                           const uint8_t port) {
    do_send(&lmic_job, payload, payload_size, port);
}

bool Node_LoRaWAN::state(){
    return  msg_state;
}

// Helper function to print out LoRaWAN device config when in DEBUG mode
void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16) {
        Serial.print('0');
    }
    Serial.print(v, HEX);
}

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
                u4_t netid = 0;
                devaddr_t devaddr = 0;
                u1_t nwkKey[16];
                u1_t artKey[16];
                LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
                Serial.print("netid: ");
                Serial.println(netid, DEC);
                Serial.print("devaddr: ");
                Serial.println(devaddr, HEX);
                Serial.print("AppSKey: ");
                for (size_t i=0; i<sizeof(artKey); ++i) {
                    if (i != 0)
                        Serial.print("-");
                    printHex2(artKey[i]);
                }
                Serial.println("");
                Serial.print("NwkSKey: ");
                for (size_t i=0; i<sizeof(nwkKey); ++i) {
                    if (i != 0)
                        Serial.print("-");
                    printHex2(nwkKey[i]);
                }
                Serial.println();
            }
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            // Transmission was competed successfully
            msg_state = true;
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            log_msg("Unknown event: %d\n", (unsigned)ev);
            break;
    }
}

// Generic functions required for LMIC to specify application and device config
void os_getArtEui(u1_t* buf) {
    memcpy_P(buf, APPEUI, 8);
}

void os_getDevEui(u1_t* buf) {
    memcpy_P(buf, DEVEUI, 8);
}

void os_getDevKey(u1_t* buf) {
    memcpy_P(buf, APPKEY, 16);
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
        //.rssi_cal = 0,
        //.spi_freq = 8000000,
};
