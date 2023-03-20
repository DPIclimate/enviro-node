#include <Arduino.h>
#include "version.h"

#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <TCA9534.h>
#include <driver/rtc_io.h>
#include <soc/rtc.h>
#include <esp_private/esp_clk.h>

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

#define TAG "wombat"

TCA9534 io_expander;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;

static void select_rtc_slow_clk(void);

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

    setenv("TZ", "UTC", 1);
    tzset();

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
    io_expander.output(4, TCA9534::Level::L); // Turn off LED

    disable12V();

    cat_m1.begin(io_expander);

    delay(1000);

    LTE_Serial.begin(115200);
    while(!LTE_Serial) {
        delay(1);
    }

    // ==== CAT-M1 Setup END ====

    // This must be done before the config is loaded because the config file is
    // a list of commands.
    CLI::init();

    DeviceConfig& config = DeviceConfig::get();
    config.load();
    config.dumpConfig(Serial);

    // Enable the brown out detection now the node has stabilised its
    // current requirements.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

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
        uint8_t p = pin & 0x7F;
        io_expander.output(p, val == HIGH ? TCA9534::H : TCA9534::L);
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
