#include <Arduino.h>
#include "davis.h"
#include <SD.h>
#include <lmic.h>
#include "node_config.h"
#include "WDT.h"

#include <RTCZero.h>

#define MAJOR 1
#define MINOR 0
#define REV   0

typedef union {
    float value;
    uint8_t bytes[4];
} FLOAT;

static constexpr int MAX_MSG = 512;

Davis davis(WIND_DIR_PIN, WIND_SPEED_PIN);
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

// This flag is used to decide whether to do a measure/uplink cycle after the Test LoRa button
// has been pressed. If the command line is used or nothing happens for 5 seconds after the
// first button press, then this flag will be true and the node will go back to sleep.
//
// If the button is pressed a second time within 5 seconds, or the button isn't pressed at all
// then the usual measure/uplink cycle is performed.
bool skipReadAndSend = false;

// If true, timestamped readings will be written to the SD card.
static bool useSdCard = false;

static const char *datalogFilename = "datalog.csv";

// A buffer for SD card data messages.
#define SD_CARD_MAX 384
static char sdCardMsg[SD_CARD_MAX];

uint8_t payloadBuffer[18];

constexpr float AD_VOLTS = 3.3f;
constexpr int AD_BIT_SIZE = 10;
static float ad_step = 0;

static osjob_t sendjob;
void do_send(osjob_t* j);
void lmic_request_network_time_cb(void * pUserData, int flagSuccess);
void sensors(void);
void doButtonProcessing(void);
void commandLine(void);

// A buffer for printing log messages.
static char msg[MAX_MSG];

// A printf-like function to print log messages prefixed by the current
// LMIC tick value. Don't call it before os_init();
void log_msg(const char *fmt, ...) {
#ifdef USE_SERIAL
    snprintf(msg, MAX_MSG, "%04d-%02d-%02d %02d:%02d:%02d ", rtc.getYear()+2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    serial.write(msg, strlen(msg));
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, MAX_MSG, fmt, args);
    va_end(args);
    serial.write(msg, strlen(msg));
    serial.println();
#endif
}

void blink(void) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
}

volatile bool buttonWake = false;
void buttonISR(void) {
    buttonWake = true;
    log_msg("buttonWake");
}

// This variable is used to keep track of LMIC state changes. When this differs from LMIC.opmode
// the LMIC state has changed, and this can be used as a flag to log the new state.
uint16_t lmicOpmode = LMIC.opmode;

void log_opmode(void) {
    if (lmicOpmode == 0) {
        log_msg("LMIC.opmode: OP_NONE");
    } else {
        memset(sdCardMsg, 0, SD_CARD_MAX);
        if (lmicOpmode & OP_SHUTDOWN) strncat(sdCardMsg, "OP_SHUTDOWN ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        if (lmicOpmode & OP_JOINING) strncat(sdCardMsg, "OP_JOINING ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        if (lmicOpmode & OP_TXDATA) strncat(sdCardMsg, "OP_TXDATA ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        if (lmicOpmode & OP_TXRXPEND) strncat(sdCardMsg, "OP_TXRXPEND ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        if (lmicOpmode & OP_RNDTX) strncat(sdCardMsg, "OP_RNDTX ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        if (lmicOpmode & OP_POLL) strncat(sdCardMsg, "OP_POLL ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        if (lmicOpmode & OP_NEXTCHNL) strncat(sdCardMsg, "OP_NEXTCHNL ", SD_CARD_MAX - strnlen(sdCardMsg, SD_CARD_MAX));
        log_msg("LMIC.opmode: %s (raw: %u)", sdCardMsg, lmicOpmode);
        memset(sdCardMsg, 0, SD_CARD_MAX);
    }
}

// This variable allows the failed event to be seen in loop(), where the LMIC state will be reset to avoid
// an endless join loop. Resetting the LMIC state in the EV_JOIN_FAILED event handler doesn't work because
// LMIC schedules another join attempt after the even handler returns.
static bool joinFailed = false;

// If the join fails, this cooldown counter allows the sketch to run through a few loops of reading the
// sensor data and writing it to the SD card without trying to join each time. Attempting a join every time
// when a gateway is down would waste a lot of power.
static uint8_t joinCooldown = 0;

static bool doSendAndMeasure = false;

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINING:
            digitalWrite(LED_BUILTIN, HIGH);
            log_msg("EV_JOINING");
            wdt_enable();
            timeOk = false;
            joinFailed = false;
            break;

        case EV_JOINED:
            wdt_disable();

            digitalWrite(LED_BUILTIN, LOW);
            log_msg("EV_JOINED");
            LMIC_setLinkCheckMode(0);
            joinCooldown = 0;
            doSendAndMeasure = true;
            break;

        case EV_JOIN_FAILED:
            log_msg("EV_JOIN_FAILED");
            // Signal loop() to reset LMIC so the current join attempts will be abandoned
            // and to try again later.
            joinFailed = true;
            break;

        case EV_TXCOMPLETE:
            wdt_disable();

            digitalWrite(LED_BUILTIN, LOW);
            log_msg("EV_TXCOMPLETE (includes waiting for RX windows)");
            break;

        case EV_TXSTART:
            digitalWrite(LED_BUILTIN, HIGH);
            log_msg("EV_TXSTART");
            wdt_enable();
            break;

        case EV_JOIN_TXCOMPLETE:
            log_msg("EV_JOIN_TXCOMPLETE: no JoinAccept");
            break;
    }
}

void do_send(void) {
    log_msg("============================================================");
    // Always do the sensor read because the sensors function also writes to the SD card.
    sensors();

    if (LMIC_queryTxReady()) {
        if (joinCooldown > 0) {
            log_msg("Skipping uplink because joinCooldown = %u", joinCooldown);
            joinCooldown = joinCooldown - 1;
            return;
        }

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
                log_msg("Daily network time request scheduled");
                timeOk = false;
            }
        }

        // Keep asking for the time until the server provides it.
        if ( ! timeOk) {
            log_msg("Adding DeviceTimeReq MAC command to uplink");
            LMIC_requestNetworkTime(lmic_request_network_time_cb, 0);
        }

        // Add some jitter to the uplink time to avoid overloading the gateway. 0 - 5s in 500ms jumps.
        uint32_t jitter = random(10) * 500;
        log_msg("Uplink jitter is %lu ms", jitter);
        delay(jitter);

        LMIC_setTxData2(1, payloadBuffer, 13, 0);
    } else {
        log_msg("LMIC busy, not sending");
    }
}

static lmic_time_reference_t lmicTimeRef;

void lmic_request_network_time_cb(void * pUserData, int flagSuccess) {
    if (flagSuccess != 0 && LMIC_getNetworkTimeReference(&lmicTimeRef)) {
        log_msg("Received network time response");
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

void sensors(void) {
    static constexpr uint32_t speedDelayInMs = 15000;
    static constexpr uint32_t dirReadDelayInMs = 500;
    static constexpr size_t dirReadCount = speedDelayInMs / dirReadDelayInMs;

    log_msg("Measuring wind speed & direction");
    davis.startSpeedMeasurement();

    /*
     * https://math.stackexchange.com/questions/44621/calculate-average-wind-direction
     *
     * Unit vector calc - does not take wind speed into account:
     *
        u_east = mean(sin(WD * pi/180))
        u_north = mean(cos(WD * pi/180))
        unit_WD = arctan2(u_east, u_north) * 180/pi
        unit_WD = (360 + unit_WD) % 360
     */
    float sumVx = 0.0f;
    float sumVy = 0.0f;
    float windRadians;

    // These are static to reduce the stack frame size.
    static uint32_t rawDirArray[dirReadCount]; // Raw output from the ADC
    static float degDirArray[dirReadCount];    // ADC output converted to 360 degrees
    static float values[dirReadCount];

    uint32_t rawDir;
    float windDir;

    for (size_t i = 0; i < dirReadCount; i++) {
        rawDir = davis.getDirectionRaw();
        windDir = davis.getDirectionDegrees(rawDir);

        rawDirArray[i] = rawDir;
        degDirArray[i] = windDir;

        windRadians = (windDir * PI) / 180.0f;

        float sx = sinf(windRadians);
        sumVx = sumVx + sx;
        float sy = cosf(windRadians);
        sumVy = sumVy + sy;

        values[i] = abs(atan2(sx, sy));

        //log_msg("Direction: %.2f, sx = %.2f, sy = %.2f, values[%d] = %.2f", windDir, sx, sy, i, values[i]);

        delay(dirReadDelayInMs);
    }

    uint32_t windCount = davis.stopSpeedMeasurement();

    //float meanVx = sumVx / dirReadCount;
    //float meanVy = sumVy / dirReadCount;

    float avgDirRadians = atan2f(sumVx, sumVy);
    float avgDirDeg = (avgDirRadians * 180.0f) / PI;

    //log_msg("avgDirDeg before fmodf: %.2f, avg dir radians: %.2f", avgDirDeg, avgDirRadians);

    avgDirDeg = fmodf((360 + avgDirDeg), 360);
    int16_t avgDeg = (int16_t)avgDirDeg;

    float stdSum = 0.0f;
    float absAvgDirRadians = abs(avgDirRadians);
    for (size_t i = 0; i < dirReadCount; i++) {
        stdSum += powf((values[i] - absAvgDirRadians), 2);
    }

    FLOAT stdDev;
    stdDev.value = sqrtf(stdSum / (dirReadCount - 1));  // std dev in radians
    stdDev.value = (stdDev.value * 180.0f) / PI;  // convert std dev to degrees

    log_msg("Avg wind direction over %lu s = %f (%d), std dev: %.2f", speedDelayInMs/1000, avgDirDeg, avgDeg, stdDev.value);

    FLOAT windKph;
    windKph.value = davis.getSpeedKph(windCount, speedDelayInMs);

    log_msg("Wind speed: count = %d, kph = %.2f", windCount, windKph.value);

    uint32_t batteryReading = analogRead(VBATT);
    log_msg("measuredvbat as read: %lu", batteryReading);
    // The battery voltage goes through a voltage divider before connecting
    // to the analogue pin, so double it to get the original value.
    // Then scale it based upon the ADC step value.
    float measuredvbat = ((float)(batteryReading * 200)) * ad_step;
    log_msg("measuredvbat adjusted: %f", measuredvbat);

    int yyyy = rtc.getYear() + 2000;
    uint8_t month = rtc.getMonth();
    uint8_t day = rtc.getDay();
    uint8_t hours = rtc.getHours();
    uint8_t minutes = rtc.getMinutes();
    uint8_t seconds = rtc.getSeconds();
    uint32_t epoch = rtc.getEpoch();

    // CSV line type 0 is most sensor readings.
    snprintf(sdCardMsg, SD_CARD_MAX, "%04d-%02d-%02dT%02d:%02d:%02dZ,%lu,%d,0,%d,%.2f,%lu,%.2f,%lu,%.2f", yyyy, month, day,
             hours, minutes, seconds, epoch, timeOk, avgDeg, stdDev.value, windCount, windKph.value, batteryReading, measuredvbat);
    log_msg("CSV entry: %s", sdCardMsg);

    if (useSdCard) {
        // Deactivate LoRa radio CS to avoid SPI bus contention.
        pinMode(LORA_CS_PIN, OUTPUT);
        digitalWrite(LORA_CS_PIN, HIGH);

        log_msg("Opening %s", datalogFilename);
        File dataFile = SD.open(datalogFilename, FILE_WRITE);
        if (dataFile) {
            // The first line to be written to the SD card was generated above and logged for debugging purposes
            // so it can be written immediately.
            dataFile.println(sdCardMsg);

            // The next line types are generated inside the SD card guards to avoid having separate string buffers for them.

            // CSV line type 1 is the raw wind direction values from the ADC. This is a separate line in case we decide to change the
            // number of raw readings we take.
            int lineLen = snprintf(sdCardMsg, SD_CARD_MAX, "%04d-%02d-%02dT%02d:%02d:%02dZ,%lu,%d,1,%u", yyyy, month, day,
                                   hours, minutes, seconds, epoch, timeOk, dirReadCount);

            int strLen = 0;
            char windDirStr[16];
            for (size_t i = 0; i < dirReadCount; i++) {
                strLen = snprintf(windDirStr, sizeof(windDirStr), ",%u", rawDirArray[i]);
                strncat(sdCardMsg, windDirStr, (SD_CARD_MAX - lineLen));
                lineLen = lineLen + strLen;
            }
            log_msg("CSV entry: %s", sdCardMsg);

            dataFile.println(sdCardMsg);

            // CSV line type 2 is the raw wind direction values from the ADC converted to 360 degrees. This is a separate line in case we decide to change the
            // number of raw readings we take.
            lineLen = snprintf(sdCardMsg, SD_CARD_MAX, "%04d-%02d-%02dT%02d:%02d:%02dZ,%lu,%d,2,%u", yyyy, month, day,
                               hours, minutes, seconds, epoch, timeOk, dirReadCount);

            strLen = 0;
            for (size_t i = 0; i < dirReadCount; i++) {
                strLen = snprintf(windDirStr, sizeof(windDirStr), ",%.2f", degDirArray[i]);
                size_t maxToCat = SD_CARD_MAX - lineLen;
                strncat(sdCardMsg, windDirStr, maxToCat);
                lineLen = lineLen + strLen;
            }
            log_msg("CSV entry: %s", sdCardMsg);

            dataFile.println(sdCardMsg);

            dataFile.flush();
            dataFile.close();
        } else {
            log_msg("Open failed");
        }
    }

    // Prepare uplink payload.
    memset(payloadBuffer, 0, sizeof(payloadBuffer));
    int i = 0;

    payloadBuffer[i++] = (avgDeg >>  8) & 0xff;
    payloadBuffer[i++] =  avgDeg        & 0xff;

    payloadBuffer[i++] = stdDev.bytes[3];
    payloadBuffer[i++] = stdDev.bytes[2];
    payloadBuffer[i++] = stdDev.bytes[1];
    payloadBuffer[i++] = stdDev.bytes[0];

    payloadBuffer[i++] = (windCount >>  8) & 0xff;
    payloadBuffer[i++] =  windCount        & 0xff;

    payloadBuffer[i++] = windKph.bytes[3];
    payloadBuffer[i++] = windKph.bytes[2];
    payloadBuffer[i++] = windKph.bytes[1];
    payloadBuffer[i++] = windKph.bytes[0];

    // The decoder is expecting the battery voltage * 100 to be encoded
    // in a uint8_t. This encoding only handles voltages up to about 3.6
    // before the uint8_t overflows.
    payloadBuffer[i++] = (uint8_t)(measuredvbat) - 127;
}

//#define SLEEP_1_MINUTE
//#define SLEEP_5_MINUTES
#define SLEEP_15_MINUTES
//#define SLEEP_1_HOUR

static int32_t delta_osticks = -1;
static int32_t delta_seconds = -1;

/*
 * This function is used to set the alarm to a relative time in the future, such as when
 * sleeping between LMIC tasks. It is not used for scheduling the sensor readings, which
 * are meant to happen at fixed times such as on the hour.
 */
void set_delta_alarm() {
    int32_t ss = (int32_t)rtc.getSeconds();
    int32_t mm = (int32_t)rtc.getMinutes();
    int32_t hh = (int32_t)rtc.getHours();

    // Sanity check.
    if (delta_seconds < 1) {
        delta_seconds = 1;
    }

    int32_t delta = delta_seconds;
    int32_t hh_delta = delta / 3600; delta -= (hh_delta * 3600);
    // Will always be less than 1 hour.
    int32_t mm_delta = delta / 60; delta -= (mm_delta * 60);
    // Will always be less than 1 minute.
    int32_t ss_delta = delta;

    ss += ss_delta;
    if (ss > 59) {
        ss = ss % 60;
        mm_delta++;
    }

    mm += mm_delta;
    if (mm > 59) {
        mm = mm % 60;
        hh_delta++;
    }

    hh = (hh + hh_delta) % 24;

    log_msg("Delta(s) = %d, wake at %02d:%02d:%02d", delta_seconds, hh, mm, ss);

    rtc.setAlarmTime((uint8_t)(hh & 0xff), (uint8_t)(mm & 0xff), (uint8_t)(ss & 0xff));
    rtc.enableAlarm(RTCZero::MATCH_HHMMSS);
}

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

#ifdef USE_SERIAL
    serial.begin(115200);
    log_msg("=== BOOT ===");

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

    log_msg("Kangaroo v%u.%u.%u", MAJOR, MINOR, REV);
    log_msg("DEVEUI: %02X %02X %02X %02X %02X %02X %02X %02X", DEVEUI[7], DEVEUI[6], DEVEUI[5], DEVEUI[4], DEVEUI[3], DEVEUI[2], DEVEUI[1], DEVEUI[0]);

#ifdef USE_CMDLINE
    log_msg("Command line enabled");
#else
    log_msg("Command line disabled");
#endif

#endif

    blink();

    // Get Arduino core interrupt code initialised. This sets clock gen 0 to feed the EIC. This clock gen is not fed from
    // an oscillator that runs during standby mode, so the interrupt won't be triggered in standby mode.
    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON), buttonISR, RISING);  // Attach interrupt to pin 6 with an ISR and when the pin state CHANGEs

    // Now set up XOSC32K to run during standby, and feed the EIC from clock gen 1 (which is configured to use XOSC32K).
    // This allows the pin interrupts to be generated to wake from standby mode.
    SYSCTRL->XOSC32K.reg |=  (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND); // set external 32k oscillator to run when idle or sleep mode is chosen
    REG_GCLK_CLKCTRL  |= GCLK_CLKCTRL_ID(GCM_EIC) |  // generic clock multiplexer id for the external interrupt controller
                         GCLK_CLKCTRL_GEN_GCLK1 |    // generic clock 1 which is xosc32k
                         GCLK_CLKCTRL_CLKEN;         // enable it
    while (GCLK->STATUS.bit.SYNCBUSY);               // write protected, wait for sync

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;   // Enable Standby or "deep sleep" mode

    pinMode(PUSH_BUTTON, INPUT_PULLDOWN);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Deactivate LoRa radio CS to avoid SPI bus contention.
    pinMode(LORA_CS_PIN, OUTPUT);
    digitalWrite(LORA_CS_PIN, HIGH);

    useSdCard = SD.begin(SDCARD_CS_PIN);
    log_msg("SD card initialised: %d", useSdCard);

    rtc.begin(false);

    // Disable NVM power reduction during standby, errata 1.5.8.
    NVMCTRL->CTRLB.bit.SLEEPPRM = 3;

    ad_step = AD_VOLTS / pow(2.0f, AD_BIT_SIZE);

    randomSeed(davis.getDirectionRaw());

    wdt_init();

    os_init();
    LMIC_reset();
    LMIC_startJoining();
}

static bool printLMICBusyMsg = true;
static bool printLMICFreeMsg = true;

void loop() {
    if (joinFailed) {
        log_msg("Join failed, resetting LMIC state.");
        LMIC_reset();
        joinFailed = false;

        // Wait another n sensor read cycles before trying to join and uplink again.
        joinCooldown = 4;
    }

    if (LMIC.opmode != lmicOpmode) {
        lmicOpmode = LMIC.opmode;
        log_opmode();
    }

    os_runloop_once();

    if (doSendAndMeasure) {
        doSendAndMeasure = false;
        log_msg("Joined, calling do_send to send initial uplink.");
        do_send();
        return;
    }

    if ( ! LMIC_queryTxReady()) {
        if (printLMICBusyMsg) {
            log_msg("LMIC busy");
            digitalWrite(LED_BUILTIN, HIGH);
            printLMICBusyMsg = false;
            printLMICFreeMsg = true;
        }

        // LMIC_queryTxReady() will return true if the OP_POLL bit is set. OP_POLL means
        // LMIC is waiting to send another uplink, either with a response to a MAC config
        // command or to receive another downlink the server has said to expect.
        //
        // LMIC schedules a job to run in the near future to do this uplink.
        //
        // So if OP_POLL is set and there is a near-future job scheduled the node can
        // sleep until just before the job is due.
        //
        // Because the node code is single-threaded and would have been in a busy loop
        // waiting for this future upload anyway, sleeping does not miss any sensor activity.
        //
        // This test checks OP_POLL | OP_RNDTX because LMIC sets the OP_RNDTX flag at this time,
        // although it only seems to be associated with class B devices.
        // As long as the joining and TX flags are clear, LMIC is basically in a waiting state.
        if (lmicOpmode == (OP_POLL + OP_RNDTX)) {
            log_msg("Checking for near future internal LMIC operations");

            bit_t valid = 0;
            ostime_t now = os_getTime();
            ostime_t deadline = os_getNextDeadline(&valid);
            if (valid) {
                delta_osticks = deadline - now;
                delta_seconds = osticks2ms(delta_osticks)/1000;
                if (delta_seconds > 3) {
                    log_msg("OP_POLL delta from now: %ld, %ld s", delta_osticks, delta_seconds);
                    delta_seconds = delta_seconds - 2;
                    set_delta_alarm();
                    rtc.standbyMode();
                    rtc.disableAlarm();
                }
            }
        }

        return;
    }

    if (printLMICFreeMsg) {
        log_msg("LMIC free");

        lmicOpmode = LMIC.opmode;
        log_opmode();

        digitalWrite(LED_BUILTIN, LOW);
        printLMICFreeMsg = false;
        printLMICBusyMsg = true;
    }

    set_next_absolute_alarm();

    log_msg("Sleeping");
#ifdef USE_SERIAL
    serial.flush();
#endif

    skipReadAndSend = false;
    buttonWake = false;
    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON), buttonISR, RISING);

    rtc.standbyMode();

    rtc.disableAlarm();

    log_msg("Awake");

    // Wrap the button processing with the WDT because I've seen it hang once.
    // The button processing will disable the WDT when going into command line
    // mode to handle long pauses there.
    //
    // blink() is called within the button processing if the button is pressed
    // a 2nd time, but given the button processing timeout is 5s and blink takes
    // 1.25s, the total max button processing time (excluding the command line) is
    // 6.25s, so less than the WDT 16s timeout.
    wdt_enable();
    if (buttonWake) {
        doButtonProcessing();
    }
    wdt_disable();

    detachInterrupt(digitalPinToInterrupt(PUSH_BUTTON));

    if ( ! skipReadAndSend) {
        do_send();
    }
}

void doButtonProcessing(void) {
    buttonWake = false;

    bool ledOn = true;
    digitalWrite(LED_BUILTIN, ledOn);

    int period = 500;
    unsigned long time_now = 0;
    uint16_t ticks = 0;
    while (ticks < 5000) {
        if (millis() - time_now > period) {
            time_now = millis();
            ticks += period;
            ledOn = !ledOn;
            digitalWrite(LED_BUILTIN, ledOn);
        }

        if (buttonWake) {
            log_msg("2nd button press, exiting button processing to measure, send, and sleep");
            skipReadAndSend = false;
            blink();
            break;
        }

#ifdef USE_SERIAL
#ifdef USE_CMDLINE
        if (serial.available()) {
            digitalWrite(LED_BUILTIN, HIGH);
            // Disable the WDT because the command line is reliant upon the user's timing and they
            // may have long periods of idleness.
            wdt_disable();
            commandLine();
            skipReadAndSend = true;
            break;
        }
#endif
#endif
    }

    if (ticks >= 5000) {
        log_msg("Button processing reached 5s timeout, going back to sleep");
        skipReadAndSend = true;
    }

    digitalWrite(LED_BUILTIN, LOW);
}

#ifdef USE_SERIAL
#ifdef USE_CMDLINE
inline void drainSerial(void) {
    while (serial.available()) {
        serial.read();
    }
}

inline int waitForChar(void) {
    while ( ! serial.available()) {
        if (joinFailed) {
            log_msg("Join failed, resetting LMIC state.");
            LMIC_reset();
            joinFailed = false;
        }

        if (LMIC.opmode != lmicOpmode) {
            lmicOpmode = LMIC.opmode;
            log_opmode();
        }

        os_runloop_once();
    }

    return serial.read();
}

#define MAX_CMD 32
char cmd[MAX_CMD];

int16_t readLine(void) {
    memset(cmd, 0, MAX_CMD);
    int16_t i = 0;
    int ch = 0;
    while (true) {
        if (i >= (MAX_CMD - 1)) {
            break;
        }

        ch = waitForChar();

        // backspace 127
        // cursor left 27 91 68
        // cursor right 27 91 67
        // home 27 91 49 126
        // end 27 91 52 126

        serial.write(ch);

        if (ch == 127 && i > 0) {
            // Backspace
            cmd[i - 1] = cmd[i];
            cmd[i] = 0;
            i = i - 1;

            serial.write(0x0D);
            serial.print("$ ");
            serial.print(cmd);

            continue;
        }

        if (ch == 0x0A || ch == 0x0D) {
            drainSerial();
            break;
        }

        if (ch >= ' ' && ch <= 'z') {
            cmd[i] = ch;
            i++;
        }
    }

    return i;
}

void commandLine(void) {
    serial.println("Kangaroo command line.");
    drainSerial();
    while (true) {
        serial.print("$ ");
        serial.flush();

        int16_t lineLen = readLine();
        serial.println();
        if (lineLen < 1) {
            continue;
        }

        log_msg("Received command [%s]", cmd);

        if (!strncmp("exit", cmd, MAX_CMD)) {
            break;
        } else if (!strncmp("join", cmd, MAX_CMD)) {
            LMIC_reset();
            LMIC_startJoining();
        } else if (!strncmp("uplink", cmd, MAX_CMD)) {
            LMIC_setTxData2(254, payloadBuffer, 0, 0);
        } else if (!strncmp("sensors", cmd, MAX_CMD)) {
            sensors();
        } else if (!strncmp("clearlog", cmd, MAX_CMD)) {
            if (useSdCard) {
                log_msg("Connfirm clearlog command - press Y to remove log file, any other key to cancel.");
                int ch = waitForChar();
                if (ch == 'Y') {
                    log_msg("Removing %s", datalogFilename);
                    int rc = SD.remove((char *) datalogFilename);
                    if (rc == 0) {
                        log_msg("Failed to delete %s", datalogFilename);
                    }
                } else {
                    log_msg("Cancelled clearing log file.");
                }
            } else {
                log_msg("SD Card not initialised.");
            }
        } else if (!strncmp("showlog", cmd, MAX_CMD)) {
            if (useSdCard) {
                log_msg("Opening %s", datalogFilename);
                File dataFile = SD.open(datalogFilename, FILE_READ);
                if (dataFile) {
                    while (true) {
                        memset(sdCardMsg, 0, SD_CARD_MAX);
                        int len = dataFile.read(sdCardMsg, SD_CARD_MAX);
                        if (len < 1) {
                            break;
                        }

                        serial.print(sdCardMsg);
                    }

                    dataFile.close();
                } else {
                    log_msg("Open failed.");
                }
            } else {
                log_msg("SD Card not initialised.");
            }
        } else if (!strncmp("help", cmd, MAX_CMD)) {
            log_msg("sensors  - do a read of the sensors; the values are recorded to the SD card logfile.");
            log_msg("showlog  - print the contents of the logfile on the SD card.");
            log_msg("clearlog - clear the logfile on the SD card.");
            log_msg("join     - attempt to join the LoRaWAN network.");
            log_msg("uplink   - send a test uplink; the payload is empty.");
            log_msg("exit     - exit the command processor, node goes to sleep.");
        }
    }
    drainSerial();
}
#endif // USE_CMDLINE
#endif // USE_SERIAL
