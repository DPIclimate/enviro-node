#include <Arduino.h>

#include <lmic.h>
#include <hal/hal.h>

#define USE_SERIAL
#define serial Serial

#include <SDI12.h>
#include <dpiclimate-12.h>

#include <davis.h>

Davis davis(A1, 12);

#define SDI12_POWER_PIN 9
#define SDI12_DATA_PIN 10

SDI12 sdi12(SDI12_DATA_PIN);
DPIClimate12 dpi12(sdi12);

static bool joined = false;

static bool got_downlink = false;
static uint8_t downlink_port;
static uint8_t downlink_data[32];
static uint8_t downlink_length = 0;

static const uint8_t PROGMEM DEVEUI[8]= { 0x1C, 0x5E, 0x05, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
static const uint8_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t PROGMEM APPKEY[16] = { 0x0A, 0xBA, 0xE7, 0x45, 0xE0, 0xFE, 0x86, 0x3C, 0x9C, 0x3C, 0x33, 0x6C, 0x55, 0x7D, 0x0A, 0xA3 };

void os_getArtEui (uint8_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (uint8_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (uint8_t* buf) { memcpy_P(buf, APPKEY, 16);}

// Pin mapping for Adafruit Feather M0 LoRa, etc.
// /!\ By default Adafruit Feather M0's pin 6 and DIO1 are not connected.
// You must ensure they are connected.
const lmic_pinmap lmic_pins = {
        .nss = 8,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = 4,
        .dio = {3, 6, LMIC_UNUSED_PIN},
        .rxtx_rx_active = 0,
        .rssi_cal = 8,              // LBT cal for the Adafruit Feather M0 LoRa, in dB
        .spi_freq = 8000000,
};

// A buffer for printing log messages.
static constexpr int MAX_MSG = 256;
static char msg[MAX_MSG];

// A printf-like function to print log messages prefixed by the current
// LMIC tick value. Don't call it before os_init();
void log_msg(const char *fmt, ...) {
#ifdef USE_SERIAL
    //snprintf(msg, MAX_MSG, "%04d-%02d-%02d %02d:%02d:%02d / ", rtc.getYear()+2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    //serial.write(msg, strlen(msg));
    //snprintf(msg, MAX_MSG, "% 012ld: ", os_getTime());
    //serial.write(msg, strlen(msg));
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, MAX_MSG, fmt, args);
    va_end(args);
    serial.write(msg, strlen(msg));
    serial.println();
#endif
}

void receiveMessage(u1_t port, u1_t *data, u1_t length) {
    downlink_port = port;
    for (int i = 0; i < length; i++) {
        downlink_data[i] = data[i];
    }

    got_downlink = true;
    downlink_length = length;
}

void print_buffer(uint8_t *buffer, uint8_t length) {
    if (buffer == 0 || length < 1) {
        return;
    }

    char str[] = { 0, 0, 0 };
    bool first = false;
    for (uint8_t i = 0; i < length; i++) {
        if ( ! first) {
            serial.print(" ");
        }

        snprintf(str, 3, "%02x", buffer[i]);
        serial.print(str);
        first = false;
    }

    serial.println();
}

void check_for_downlink() {
    got_downlink = false;

    // Any data to be received?
    if (LMIC.dataLen != 0 || LMIC.dataBeg != 0) {
        // Data was received. Extract port number if any.
        u1_t bPort = 0;
        if (LMIC.txrxFlags & TXRX_PORT) {
            bPort = LMIC.frame[LMIC.dataBeg - 1];
        }

        log_msg("DL: dataLen = %d, dataBeg = %d, TXRX_PORT = %d, port = %d", LMIC.dataLen, LMIC.dataBeg, LMIC.txrxFlags & TXRX_PORT, bPort);
        print_buffer(LMIC.frame, LMIC.dataBeg);
        if (bPort != 0) {
            receiveMessage(bPort, LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        }
    }
}

static osjob_t sendjob;

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINING:
            log_msg("EV_JOINING");
            //wdt_enable();
            joined = false;
            //timeOk = false;
            break;
        case EV_JOINED:
            log_msg("EV_JOINED");

            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            joined = true;

            //os_setCallback(&sendjob, do_send);

            break;
        case EV_JOIN_FAILED:
            log_msg("EV_JOIN_FAILED");
            break;
        case EV_TXCOMPLETE:
            digitalWrite(LED_BUILTIN, LOW);
            log_msg("EV_TXCOMPLETE (includes waiting for RX windows)");
            //wdt_disable();
/*
            check_for_downlink();
*/
            // The uplink/downlink is done so now it is ok to check for a sleep window.
            //check_deadline = true;
            break;
        case EV_TXSTART:
            digitalWrite(LED_BUILTIN, HIGH);
            //wdt_enable();
            got_downlink = false;
            log_msg("EV_TXSTART");
            break;
        case EV_JOIN_TXCOMPLETE:
            log_msg("EV_JOIN_TXCOMPLETE: no JoinAccept");
            break;
    }
}

void setup() {
    sdi12.begin();

    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    joined = false;

    Serial.begin(115200);
    while ( ! Serial);

    Serial.println("X");

    LMIC_startJoining();
}

sensor_list sl;

void readSdi12() {
    pinMode(SDI12_POWER_PIN, OUTPUT);
    delay(1000);

    /*
    dpi12.scan_bus(sl);

    for (int i = 0; i < sl.count; i++) {
        dpi12.do_measure(sl.sensors[i].address);
    }
     */

    dpi12.do_measure('0');
    dpi12.do_measure('1');
    pinMode(SDI12_POWER_PIN, INPUT);
}

void loop() {

    while ( ! joined) {
        os_runloop_once();
    }

    Serial.print("Dir: "); Serial.print(davis.getDirectionRaw());
    Serial.print(", deg: "); Serial.print(davis.getDirectionDegrees());

    davis.startSpeedMeasurement();
    delay(3000);
    uint32_t c = davis.stopSpeedMeasurement();
    Serial.print(", count: "); Serial.print(c);
    Serial.print(", mph: "); Serial.print(davis.getSpeedMph(c, 3000));
    Serial.print(", kph: "); Serial.print(davis.getSpeedKph(c, 3000));

    Serial.println();

    readSdi12();
    delay(5000);
}
