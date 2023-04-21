#include <Arduino.h>
#include "version.h"

#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <TCA9534.h>
#include <driver/rtc_io.h>
#include <soc/rtc.h>
#include <esp_private/esp_clk.h>
#include <SD.h>
#include <esp_ota_ops.h>

#define ALLOCATE_GLOBALS
#include "globals.h"
#undef ALLOCATE_GLOBALS

#include "SensorTask.h"
#include "DeviceConfig.h"
#include "cli/CLI.h"
#include "peripherals.h"

#include "audio-feedback/tones.h"
#include "uplinks.h"
#include "ulp.h"

#include "soc/rtc.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "sd-card/interface.h"
#include "freertos/semphr.h"

#include "esp_partition.h"
#include "ftp_stack.h"

#define TAG "wombat"

TCA9534 io_expander;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;

static void select_rtc_slow_clk(void);

static File *log_file = nullptr;

/*
static vprintf_like_t old_log_fn;
static File *log_file;
constexpr size_t max_log_len = 4096;
static char log_msg[max_log_len+1];

// Maximum time to wait for the mutex in a logging statement.
//
// We don't expect this to happen in most cases, as contention is low. The most likely case is if a
// log function is called from an ISR (technically caller should use the ISR-friendly logging macros but
// possible they use the normal one instead and disable the log type by tag).
#define MAX_MUTEX_WAIT_MS 10
#define MAX_MUTEX_WAIT_TICKS ((MAX_MUTEX_WAIT_MS + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS)

static SemaphoreHandle_t s_log_mutex = NULL;

static void esp_log_impl_lock(void)
{
    if (unlikely(!s_log_mutex)) {
        s_log_mutex = xSemaphoreCreateMutex();
    }
    if (unlikely(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)) {
        return;
    }
    xSemaphoreTake(s_log_mutex, portMAX_DELAY);
}

static void esp_log_impl_unlock(void)
{
    if (unlikely(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)) {
        return;
    }
    xSemaphoreGive(s_log_mutex);
}

#ifdef __cplusplus
extern "C" {
#endif

int my_log_vprintf(const char *fmt, va_list args) {
    esp_log_impl_lock();

    log_msg[0] = '@';
    log_msg[1] = '|';
    int iresult = vsnprintf(&log_msg[2], max_log_len, fmt, args);
    if (iresult > 0) {
        printf(log_msg);
    }

    esp_log_impl_unlock();

    return iresult;
}

int my_log_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = my_log_vprintf(fmt, args);
    va_end(args);
    printf("\n");
    Serial.flush();
    return rc;
}

#ifdef __cplusplus
}
#endif
*/

void setup() {
    // Disable brown-out detection until the BT LE radio is running.
    // The radio startup triggers a brown out detection, but the
    // voltage seems ok and the node keeps running.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(PROG_BTN, INPUT);
    progBtnPressed = digitalRead(PROG_BTN);

    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    //ESP_LOGI(TAG, "Changing log function");
    //old_log_fn = esp_log_set_vprintf(&my_log_vprintf);

    setenv("TZ", "UTC", 1);
    tzset();

    // The device configuration singleton is created on entry to setup() due to C++ object creation rules.
    // The node id is available without loading the configuration because it is retrieved from the ESP32
    // modem during the config.reset() call, not the saved configuration.
    DeviceConfig& config = DeviceConfig::get();
    config.reset();

    //ESP_LOGI(TAG, "Old func: %p", old_log_fn);

    ESP_LOGI(TAG, "Wombat firmware: %s", commit_id);
    ESP_LOGI(TAG, "Wake up time: %s", iso8601());
    ESP_LOGI(TAG, "CPU MHz: %lu", getCpuFrequencyMhz());

    // Switch the RTC clock to the external crystal.
    rtc_slow_freq_t rtc_freq = rtc_clk_slow_freq_get();
    if (rtc_freq != RTC_SLOW_FREQ_32K_XTAL) {
        select_rtc_slow_clk();
    }

    // Try to avoid it getting optimized out.
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    // ==== CAT-M1 Setup START ====
    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    // Set all pins to output mode (reg 3, value 0).
    io_expander.config(TCA9534::Config::OUT);
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED

    // WARNING: The IO expander must be initialised before the SD card is enabled, because the SD card
    // enable line is one of the IO expander pins.
    if (SDCardInterface::begin()) {
        ESP_LOGI(TAG, "SD card initialised");
        //if (SDCardInterface::is_ready()) {
        //    *log_file = SD.open(sd_card_logfile_name, FILE_APPEND, true);
        //}
    } else {
        ESP_LOGW(TAG, "SD card initialisation failed");
    }

    enable12V();
    delay(250);

    cat_m1.begin(io_expander);

    LTE_Serial.begin(115200);
    while(!LTE_Serial) {
        delay(1);
    }

    // ==== CAT-M1 Setup END ====

    digitalWrite(SD_CARD_ENABLE, HIGH);
    if ( ! SD.begin()) {
        ESP_LOGI(TAG, "Failed to initialise SD card");
        digitalWrite(SD_CARD_ENABLE, LOW);
    } else {
        ESP_LOGI(TAG, "SD card initialised");
    }

    // This must be done before the config is loaded because the config file is
    // a list of commands.
    CLI::init();

    config.load();

    config.dumpConfig(Serial);

    // Enable the brown out detection now the node has stabilised its
    // current requirements.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

    delay(100);

    battery_monitor.begin();
    solar_monitor.begin();

    if (progBtnPressed) {
        // Turn on bluetooth if entering CLI
        init_sensors();
        //BluetoothServer::begin();
        progBtnPressed = false;
        ESP_LOGI(TAG, "Programmable button pressed while booting, dropping into REPL");
        CLI::repl(Serial);
        ESP_LOGI(TAG, "Continuing");
    }

    if (config.getBootCount() == 0) {
        initULP();
    }

    uint16_t measurement_interval_secs = config.getMeasureInterval();
    uint16_t uplink_interval_secs = config.getUplinkInterval();

    if (uplink_interval_secs < measurement_interval_secs) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be <= uplink interval "
                      "(%u). Resetting config to default values.", measurement_interval_secs, uplink_interval_secs);
        config.reset();
    }

    if (uplink_interval_secs % measurement_interval_secs != 0) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be a factor of uplink "
                      "interval (%u). Resetting config to default "
                      "values.", measurement_interval_secs, uplink_interval_secs);
        config.reset();
    }

    // Assuming the measure interval is a factor of the uplink interval,
    // boots_between_uplinks is the number of boots between uplinks.
    uint16_t boots_between_uplinks = uplink_interval_secs / measurement_interval_secs;

    // If the current boot cycle is a multiple of boots_between_uplinks, then this boot cycle
    // should do an uplink.
    bool is_uplink_cycle = config.getBootCount() % boots_between_uplinks == 0;

    ESP_LOGI(TAG, "Boot count: %lu, measurement interval: %u, "
                  "uplink interval: %u, uplink_interval_secs/measurement_interval_secs: %u, uplink this cycle: %d",
             config.getBootCount(), measurement_interval_secs, uplink_interval_secs, boots_between_uplinks, is_uplink_cycle);

    // Not sure how useful this is, or how it will work. We don't want to
    // interrupt sensor reading or uplink processing and loop() will likely
    // never run if we do the usual ESP32 setup going to deep sleep mode.
    // It is useful while developing because the node isn't going to sleep.
//    attachInterrupt(PROG_BTN, progBtnISR, RISING);

    // Ensure this is 0 for when we are not in an uplink cycle, or the connection to the mobile network fails.
    sleep_drift_adjustment = 0;
    if (is_uplink_cycle) {
        if ( ! connect_to_internet()) {
            ESP_LOGW(TAG, "Could not connect to the internet on an uplink cycle. This is now a measurement-only cycle with no RTC update.");
            is_uplink_cycle = false;
        }
    }

    init_sensors();
    sensor_task();

    if (is_uplink_cycle) {
        send_messages();
    }

    ESP_LOGI(TAG, "Shutting down");

    if (r5_ok) {
        r5.modulePowerOff();
    }

    cat_m1.power_supply(false);
    delay(20);
    battery_monitor.sleep();
    solar_monitor.sleep();

    disable12V();
    delay(20);

    // Disable the SD card.
    if (log_file) {
        log_file->flush();
        log_file->close();
    }
    SD.end();
    digitalWrite(SD_CARD_ENABLE, LOW);

    unsigned long setup_end_ms = millis();
    unsigned long setup_in_secs = setup_end_ms / 1000;
    uint64_t mi_in_secs = (uint64_t)measurement_interval_secs;
    uint64_t sleep_time = 0;
    if (setup_in_secs > measurement_interval_secs) {
        ESP_LOGW(TAG, "Processing took longer than the measurement interval, skip some intervals");
        uint64_t remainder = setup_in_secs - mi_in_secs;
        while (remainder > mi_in_secs) {
            remainder = setup_in_secs - mi_in_secs;
            setup_in_secs = setup_in_secs - mi_in_secs;
        }

        ESP_LOGW(TAG, "mi_in_secs = %lu, setup_in_secs = %lu, remainder = %lu", mi_in_secs, setup_in_secs, remainder);
        sleep_time = (mi_in_secs - remainder) * 1000000;
    } else {
        // sleep_drift_adjustment is measured in seconds and calculated from the difference between the ESP32 RTC
        // and the time from the mobile network.
        sleep_time = ((measurement_interval_secs + sleep_drift_adjustment) * 1000000) - (setup_end_ms * 1000);
        ESP_LOGI(TAG, "Unadjusted sleep_time = %lu", sleep_time);
        sleep_time = (uint64_t)((float)sleep_time * config.getSleepAdjustment());
        ESP_LOGI(TAG, "Adjusted sleep_time = %lu", sleep_time);
    }

    float f_s_time = (float)sleep_time / 1000000.0f;
    ESP_LOGI(TAG, "Run took %lu ms, going to sleep at: %s, for %.2f s", setup_end_ms, iso8601(), f_s_time);
    Serial.flush();

    esp_sleep_enable_timer_wakeup(sleep_time);
    esp_deep_sleep_start();
}

void loop() {
}

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Allow digitalWrite to be used for pins on the IO expander.
 *
 * This function overrides the default digitalWrite provided by the Arduino core so
 * we can use it with pins on the IO expander IC. If the pin number > 0x80 then the
 * bits > 0x80 are masked off and the request is sent to the IO expander library.
 *
 * For example, passing in pin 0x82 will ask the IO expander library to write to its
 * pin 2.
 *
 * @param pin the GPIO to write to; values > 0x80 will be handled by the IO expander library
 * after masking off the bits > 0x80.
 * @val HIGH or LOW
 */
extern void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin & 0x80) {
        io_expander.output(ARDUINO_TO_IO(pin), val == HIGH ? TCA9534::H : TCA9534::L);
    } else {
        gpio_set_level((gpio_num_t) pin, val);
    }
}
#ifdef __cplusplus
}
#endif

#define SLOW_CLK_CAL_CYCLES 2048

/**
 * \brief Switch the RTC clock source to the external crystal.
 *
 * This code is copied from the ESP32 Ardiuno core, and slightly modified so it only tries
 * to use the external crystal before falling back to the 150 kHz internal oscillator.
 */
void select_rtc_slow_clk(void)
{
    rtc_slow_freq_t rtc_slow_freq = RTC_SLOW_FREQ_32K_XTAL;
    uint32_t cal_val = 0;
    /* number of times to repeat 32k XTAL calibration
     * before giving up and switching to the internal RC
     */
    int retry_32k_xtal = 5;

    do {
        /* 32k XTAL oscillator needs to be enabled and running before it can
         * be used. Hardware doesn't have a direct way of checking if the
         * oscillator is running. Here we use rtc_clk_cal function to count
         * the number of main XTAL cycles in the given number of 32k XTAL
         * oscillator cycles. If the 32k XTAL has not started up, calibration
         * will time out, returning 0.
         */
        ESP_LOGI(TAG, "waiting for 32k oscillator to start up");
        rtc_clk_32k_enable(true);

        // When SLOW_CLK_CAL_CYCLES is set to 0, clock calibration will not be performed at startup.
        if (SLOW_CLK_CAL_CYCLES > 0) {
            cal_val = rtc_clk_cal(RTC_CAL_32K_XTAL, SLOW_CLK_CAL_CYCLES);
            if (cal_val == 0) {
                if (retry_32k_xtal-- > 0) {
                    continue;
                }
                ESP_LOGW(TAG, "32 kHz XTAL not found, switching to internal 150 kHz oscillator");
                rtc_slow_freq = RTC_SLOW_FREQ_RTC;
            }
        }

        rtc_clk_slow_freq_set(rtc_slow_freq);

        if (SLOW_CLK_CAL_CYCLES > 0) {
            /* TODO: 32k XTAL oscillator has some frequency drift at startup.
             * Improve calibration routine to wait until the frequency is stable.
             */
            cal_val = rtc_clk_cal(RTC_CAL_RTC_MUX, SLOW_CLK_CAL_CYCLES);
        } else {
            const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;
            cal_val = (uint32_t) (cal_dividend / rtc_clk_slow_freq_get_hz());
        }
    } while (cal_val == 0);

    ESP_LOGI(TAG, "RTC_SLOW_CLK calibration value: %d", cal_val);
    esp_clk_slowclk_cal_set(cal_val);
}
