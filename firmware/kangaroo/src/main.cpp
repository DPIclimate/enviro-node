#include <Arduino.h>
#include "davis.h"
#include <dpiclimate-12.h>
#include <SD.h>
#include <lmic.h>
#include "node_config.h"

#include <RTCZero.h>

static constexpr int MAX_MSG = 256;

Davis davis(WIND_DIR_PIN, WIND_SPEED_PIN);
SDI12 mySDI12(SDI12_DATA_PIN);
DPIClimate12 dpi12(mySDI12);
RTCZero rtc;

// This flag is used to decide whether to make a network time request when
// an uplink is set. The network time request adds 1 byte to the payload.
// Once a network time response has been received this is set to true.
// It could be reset once a day or so to try and keep clock drift in check.
static bool timeOk = false;

// This value is set when a network time response has been received. When
// rtc.getDay() returns a different value it means a day has passed since
// the last clock sync, so send another network time request to try and
// avoid too much clock drift.
//
// Initialised to a value that cannot be returned by rtc.getDay(), but
// it will be set in the network time response callback before it is
// checked anyway.
static uint8_t timeReqDay = 200;

// If true, timestamped readings will be written to the SD card.
static bool useSdCard = false;

// A buffer for SD card data messages.
static char sdCardMsg[MAX_MSG];

uint8_t payloadBuffer[18];

constexpr float AD_VOLTS = 3.3f;
constexpr int AD_BIT_SIZE = 10;
static float ad_step = 0;

static osjob_t sendjob;
void do_send(osjob_t* j);
void lmic_request_network_time_cb(void * pUserData, int flagSuccess);
void sensors();

// A buffer for printing log messages.
static char msg[MAX_MSG];

// A printf-like function to print log messages prefixed by the current
// LMIC tick value. Don't call it before os_init();
void log_msg(const char *fmt, ...) {
#ifdef USE_SERIAL
    snprintf(msg, MAX_MSG, "%04d-%02d-%02d %02d:%02d:%02d: ", rtc.getYear()+2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    serial.write(msg, strlen(msg));
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, MAX_MSG, fmt, args);
    va_end(args);
    serial.write(msg, strlen(msg));
    serial.println();
#endif
}

void EIC_ISR(void) {
    log_msg("ISR");
}

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINING:
            digitalWrite(LED_BUILTIN, HIGH);
            log_msg("EV_JOINING");
            timeOk = false;
            break;

        case EV_JOINED:
            digitalWrite(LED_BUILTIN, LOW);
            log_msg("EV_JOINED");
            LMIC_setLinkCheckMode(0);
            break;

        case EV_JOIN_FAILED:
            log_msg("EV_JOIN_FAILED");
            break;

        case EV_TXCOMPLETE:
            digitalWrite(LED_BUILTIN, LOW);
            log_msg("EV_TXCOMPLETE (includes waiting for RX windows)");
            break;

        case EV_TXSTART:
            digitalWrite(LED_BUILTIN, HIGH);
            log_msg("EV_TXSTART");
            break;

        case EV_JOIN_TXCOMPLETE:
            log_msg("EV_JOIN_TXCOMPLETE: no JoinAccept");
            break;
    }
}

void do_send(osjob_t* j) {
    log_msg("============================================================");
    if (LMIC_queryTxReady()) {
        sensors();

        // This code ensures a network time request is sent once a day to
        // control clock drift.
        //
        // It is done after reading the sensors because the SD card log file
        // contains a flag to show whether the RTC has been set from the network.
        //
        // This code sets timeOk to false once a day, so those entries in the
        // log file would be marked as not having a valid timestamp if this code
        // was before the call to sensors().
        if (timeOk) {
            if (rtc.getDay() != timeReqDay) {
                log_msg("Daily network time request scheduled.");
                timeOk = false;
            }
        }

        // Keep asking for the time until the server provides it.
        if ( ! timeOk) {
            log_msg("Adding DeviceTimeReq MAC command to uplink.");
            LMIC_requestNetworkTime(lmic_request_network_time_cb, 0);
        }

        LMIC_setTxData2(1, payloadBuffer, 7, 0);
    } else {
        log_msg("LMIC busy, not sending");
    }
}

static lmic_time_reference_t lmicTimeRef;

void lmic_request_network_time_cb(void * pUserData, int flagSuccess) {
    if (flagSuccess != 0 && LMIC_getNetworkTimeReference(&lmicTimeRef)) {
        log_msg("Received network time response.");
        // GPS time is reported in seconds, and RTCZero also works with seconds in epoch values.
        // 315964800 is the number of seconds between the UTC epoch and the GPS epoch, when GPS started.
        // 18 is the GPS-UTC offset.
        constexpr uint32_t adjustment = 315964800 - 18;
        uint32_t ts = lmicTimeRef.tNetwork + adjustment;

        ostime_t now_ticks = os_getTime();
        ostime_t delta_ticks = now_ticks - lmicTimeRef.tLocal;
        float delta_s = ((float)osticks2ms(delta_ticks)) / 1000.0f;
        uint32_t ds = std::roundf(delta_s);
        ts += ds;
        rtc.setEpoch(ts);

        timeOk = true;
        timeReqDay = rtc.getDay();
    }
}

void sensors() {
    static constexpr uint32_t speedDelayInMs = 15000;

    uint32_t rawDir = davis.getDirectionRaw();
    float windDir = davis.getDirectionDegrees();
    int16_t windDeg = (int16_t) windDir;

    log_msg("Wind direction: %lu / %f (%d)", rawDir, windDir, windDeg);

    log_msg("Measuring wind speed");
    davis.startSpeedMeasurement();
    delay(speedDelayInMs);
    uint32_t windCount = davis.stopSpeedMeasurement();

    FLOAT windKph;
    windKph.value = davis.getSpeedKph(windCount, speedDelayInMs);

    log_msg("Wind speed: count = %d, kph = %.2f", windCount, windKph.value);

    // Deactivate LoRa radio CS to avoid SPI bus contention.
    pinMode(LORA_CS_PIN, OUTPUT);
    digitalWrite(LORA_CS_PIN, HIGH);

    if (useSdCard) {
        File dataFile = SD.open("datalog.txt", O_CREAT | O_WRITE | O_APPEND);
        if (dataFile) {
            snprintf(sdCardMsg, MAX_MSG, "%04d-%02d-%02dT%02d:%02d:%02dZ,%d,%lu,%u,%lu,%.2f", rtc.getYear()+2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds(), timeOk, rawDir, windDeg, windCount, windKph.value);
            log_msg(sdCardMsg);
            dataFile.println(sdCardMsg);
            dataFile.close();
        }
    }

    // Prepare uplink payload.
    memset(payloadBuffer, 0, sizeof(payloadBuffer));
    int i = 0;

    payloadBuffer[i++] = (windDeg >>  8) & 0xff;
    payloadBuffer[i++] =  windDeg        & 0xff;

    payloadBuffer[i++] = windKph.bytes[3];
    payloadBuffer[i++] = windKph.bytes[2];
    payloadBuffer[i++] = windKph.bytes[1];
    payloadBuffer[i++] = windKph.bytes[0];

    uint32_t batteryReading = analogRead(VBATT);
    log_msg("measuredvbat as read: %lu", batteryReading);
    // The battery voltage goes through a voltage divider before connecting
    // to the analogue pin, so double it to get the original value.
    // Then scale it based upon the ADC step value.
    float measuredvbat = ((float)(batteryReading * 200)) * ad_step;
    log_msg("measuredvbat adjusted: %f", measuredvbat);

    // The decoder is expecting the battery voltage * 100 to be encoded
    // in a uint8_t. This encoding only handles voltages up to about 3.6
    // before the uint8_t overflows.
    payloadBuffer[i++] = (uint8_t)(measuredvbat) - 127;
}

//#define SLEEP_1_MINUTE
//#define SLEEP_5_MINUTES
#define SLEEP_15_MINUTES
//#define SLEEP_1_HOUR

/*
 * This function is used to set the alarm to the absolute times used
 * for sending sensor data.
 */
void set_next_absolute_alarm() {
    uint8_t mm = rtc.getMinutes();
    uint8_t hh = rtc.getHours();

#ifdef SLEEP_1_MINUTE
    mm += 1;
    if (mm > 59) {
        mm = mm % 60;
        hh = (hh + 1) % 24;
    }
#endif
#ifdef SLEEP_5_MINUTES
    mm += 5;
    if (mm > 59) {
        mm = mm % 60;
        hh = (hh + 1) % 24;
    }
#endif
#ifdef SLEEP_15_MINUTES
    if (mm < 15) {
        mm = 15;
    } else if (mm < 30) {
        mm = 30;
    } else if (mm < 45) {
        mm = 45;
    } else {
        mm = 0;
    }

    if (mm == 0) {
        hh = (hh + 1) % 24;
    }
#endif
#ifdef SLEEP_1_HOUR
    mm = 0;
    hh = (hh + 1) % 24;
#endif

    log_msg("Abs wake at %02u:%02u:%02u", hh, mm, 0);

    rtc.setAlarmTime(hh, mm, 0);
    rtc.enableAlarm(RTCZero::MATCH_HHMMSS);
}

void setup() {
    serial.begin(115200);
    while ( ! serial);

    serial.println("=== BOOT ===");

    uint8_t reset_cause = PM->RCAUSE.reg;
    if (reset_cause & PM_RCAUSE_POR) {
        log_msg("Power on reset");
    } else if (reset_cause & PM_RCAUSE_WDT) {
        log_msg("WDT reset");
    } else if (reset_cause & (PM_RCAUSE_BOD12 | PM_RCAUSE_BOD33)) {
        log_msg("BOD reset");
    } else if (reset_cause & PM_RCAUSE_SYST) {
        log_msg("System reset request");
    } else if (reset_cause & PM_RCAUSE_EXT) {
        log_msg("External reset");
    } else {
        log_msg("reset_cause = %ud", reset_cause);
    }

    // Get Arduino core interrupt code initialised. This sets clock gen 0 to feed the EIC. This clock gen is not fed from
    // an oscillator that runs during standby mode, so the interrupt won't be triggered in standby mode.
    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON), EIC_ISR, RISING);  // Attach interrupt to pin 6 with an ISR and when the pin state CHANGEs

    // Now set up XOSC32K to run during standby, and feed the EIC from clock gen 1 (which is configured to use XOSC32K).
    // This allows the pin interrupts to be generated to wake from standby mode.
    SYSCTRL->XOSC32K.reg |=  (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND); // set external 32k oscillator to run when idle or sleep mode is chosen
    REG_GCLK_CLKCTRL  |= GCLK_CLKCTRL_ID(GCM_EIC) |  // generic clock multiplexer id for the external interrupt controller
                         GCLK_CLKCTRL_GEN_GCLK1 |    // generic clock 1 which is xosc32k
                         GCLK_CLKCTRL_CLKEN;         // enable it
    while (GCLK->STATUS.bit.SYNCBUSY);               // write protected, wait for sync

    PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU;  // Enable Idle0 mode - sleep CPU clock only
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;   // Enable Standby or "deep sleep" mode

    pinMode(PUSH_BUTTON, INPUT);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    pinMode(SDI12_POWER, OUTPUT);
    digitalWrite(SDI12_POWER, LOW);
    pinMode(SDI12_POWER, INPUT);

    // Deactivate LoRa radio CS to avoid SPI bus contention.
    pinMode(LORA_CS_PIN, OUTPUT);
    digitalWrite(LORA_CS_PIN, HIGH);

    useSdCard = SD.begin(SDCARD_CS_PIN);
    log_msg("SD card initialised: %d", useSdCard);

    mySDI12.begin();
    rtc.begin(false);

    os_init();
    LMIC_reset();

    os_setCallback(&sendjob, do_send);

    // Disable NVM power reduction during standby, errata 1.5.8.
    NVMCTRL->CTRLB.bit.SLEEPPRM = 3;

    ad_step = AD_VOLTS / pow(2.0f, AD_BIT_SIZE);
}

bool printLMICBusyMsg = true;
bool printLMICFreeMsg = true;

void loop() {
    os_runloop_once();

    // Allow LMIC to run in a tight-loop if it is busy. A side-effect of
    // using this function is that when responding to MAC commands, LMIC
    // returns false for this function meaning the sketch does not go to
    // standby mode for the ~19s between the downlink containing the
    // MAC commands and the uplink responding to them.
    if ( ! LMIC_queryTxReady()) {
        if (printLMICBusyMsg) {
            digitalWrite(LED_BUILTIN, HIGH);
            printLMICBusyMsg = false;
            printLMICFreeMsg = true;
        }
        return;
    }

    if (printLMICFreeMsg) {
        digitalWrite(LED_BUILTIN, LOW);
        printLMICFreeMsg = false;
        printLMICBusyMsg = true;
    }

    set_next_absolute_alarm();

    log_msg("Sleeping.");
    serial.flush();

    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON), EIC_ISR, RISING);  // Attach interrupt to pin 6 with an ISR and when the pin state CHANGEs
    __WFI();  // wake from interrupt
    detachInterrupt(digitalPinToInterrupt(PUSH_BUTTON));
    rtc.disableAlarm();

    log_msg("Awake.");
    serial.flush();

    os_setCallback(&sendjob, do_send);
}
