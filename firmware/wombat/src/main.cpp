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
#include <SPIFFS.h>

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
#include "ota_update.h"
#include "ftp_stack.h"
#include "power_monitoring/battery.h"
#include "power_monitoring/solar.h"

#include "Utils.h"

#define TAG "wombat"

TCA9534 io_expander;

bool spiffs_ok = false;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;

static void select_rtc_slow_clk(void);

//static File *log_file = nullptr;

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

/**
 * A simple timeout task. Runs on core 0 so the app code does not interfere with it.
 *
 * If 14 minutes passes and timeout_restart has not been set to false, power to the
 * modem is shut off and the ESP32 is rebooted.
 *
 * @param pvParameters not used.
 */
[[noreturn]] void timeout_task(void * pvParameters) {
    const TickType_t delay = 60 * 60 * 1000 / portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "Starting");
    while(true) {
        // Set to true here, the app code on core 1 must set it to false before the
        // timeout task wakes up to avoid a reboot.
        timeout_restart = true;
        vTaskDelay(delay);

        if (timeout_active && timeout_restart) {
            log_to_sdcard("[E] timeout_task forced reboot");
            ESP_LOGE(TAG, "Removing power from R5");
            cat_m1.power_supply(false);
            vTaskDelay(5000 / portTICK_PERIOD_MS); // 5s

            ESP_LOGE(TAG, "Rebooting due to app code timeout");
            esp_restart();
        }
    }
}

static TaskHandle_t xHandle = nullptr;

void setup(void) {
    // Disable brown-out detection until the BT LE radio is running.
    // The radio startup triggers a brown out detection, but the
    // voltage seems ok and the node keeps running.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(PROG_BTN, INPUT);
    progBtnPressed = digitalRead(PROG_BTN);

    // The timeout task is pinned to core 0, which usually runs the wireless stacks. We're not using
    // them so the core is free and this should mean the task is not blocked by anything the app does.
    timeout_active = true;
    static uint8_t ucParameterToPass = 0;
    xTaskCreatePinnedToCore(timeout_task, "Timeout", 4096, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle, 0);
    configASSERT(xHandle);

    // SARA R5 library logging does not work without this.
    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    //ESP_LOGI(TAG, "Changing log function");
    //old_log_fn = esp_log_set_vprintf(&my_log_vprintf);

    setenv("TZ", "UTC", 1);
    tzset();

    spiffs_ok = SPIFFS.begin();

    // The device configuration singleton is created on entry to setup() due to C++ object creation rules.
    // The node id is available without loading the configuration because it is retrieved from the ESP32
    // modem during the config.reset() call, not the saved configuration.
    DeviceConfig& config = DeviceConfig::get();
    config.reset();

    //ESP_LOGI(TAG, "Old func: %p", old_log_fn);

    ESP_LOGI(TAG, "Wake up time: %s", iso8601());
    ESP_LOGI(TAG, "CPU MHz: %lu", getCpuFrequencyMhz());

    ESP_LOGI(TAG, "Boot partition");
    const esp_partition_t *p_type = esp_ota_get_boot_partition();
    ESP_LOGI(TAG, "%d/%d %lx %lx %s", p_type->type, p_type->subtype, p_type->address, p_type->size, p_type->label);

    ESP_LOGI(TAG, "App ver: %u.%u.%u, commit: %s, repo status: %s", ver_major, ver_minor, ver_update, commit_id, repo_status);

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

    log_to_sdcard("--------------------");
    log_to_sdcard("Woke up");

    // This must be done before the config is loaded because the config file is
    // a list of commands.
    CLI::init();
    config.load();
    config.dumpConfig(Serial);

    // Enable the brown out detection now the node has stabilised its
    // current requirements.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

    delay(100);

    BatteryMonitor::begin();
    SolarMonitor::begin();

//    back_to_factory();
//    ESP_LOGI(TAG, "Boot partition");
//    p_type = esp_ota_get_boot_partition();
//    ESP_LOGI(TAG, "%d/%d %lx %lx %s", p_type->type, p_type->subtype, p_type->address, p_type->size, p_type->label);

    if (progBtnPressed) {
        init_sensors();
        progBtnPressed = false;
        ESP_LOGI(TAG, "Programmable button pressed while booting, dropping into REPL");
        CLI::repl(Serial, Serial);
        ESP_LOGI(TAG, "Continuing");
    }

    if (config.getBootCount() == 0) {
        ESP_LOGI(TAG, "Starting ULP processing");
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

    if (is_uplink_cycle) {
        if ( ! connect_to_internet()) {
            ESP_LOGW(TAG, "Could not connect to the internet on an uplink cycle. This is now a measurement-only cycle");
            log_to_sdcard("[E] cti failed, only measuring");
            is_uplink_cycle = false;
        }
    }

    log_to_sdcard("init_sensors");
    init_sensors();
    log_to_sdcard("sensor_task");
    sensor_task();
    log_to_sdcard("back from sensor_task");

    if (is_uplink_cycle) {
        log_to_sdcard("send_messages");
        send_messages();
        log_to_sdcard("back from send_messages");
        // If a config script turned up, run it now.
        if (script != nullptr) {
            log_to_sdcard("Running config script");
            log_to_sdcard(script);

            ESP_LOGI(TAG, "Running config script\n%s", script);
            StreamString scriptStream;
            scriptStream.print(script);
            // Ensure the script ends with an exit command.
            scriptStream.print("\nexit\n");
            free(script);
            CLI::repl(scriptStream, Serial);

            log_to_sdcard("Finished script");
        }
    }

    shutdown();

    // NOTE: Because the ESP32 effectively reboots after waking from deep sleep the ESP32 internal tick counter
    // always starts at 0. This means simply getting the value of millis() here is sufficient to get an idea of
    // how long it took to get here. There is no need to save the millis() at the top of the function and do a
    // subtraction here.
    uint64_t measurement_interval_ms = measurement_interval_secs * 1000;
    uint64_t delta_ms = 0;
    uint64_t sleep_time_us = 0;
    uint64_t setup_duration_ms = millis();

    if (setup_duration_ms > measurement_interval_ms) {
        ESP_LOGW(TAG, "run time > measurement interval");
        delta_ms = setup_duration_ms - measurement_interval_ms;
        while (delta_ms > measurement_interval_ms) {
            delta_ms = setup_duration_ms - measurement_interval_ms;
            setup_duration_ms = setup_duration_ms - measurement_interval_ms;
            ESP_LOGW(TAG, "skipping an interval, setup_duration_ms now = %llu, delta_ms = %llu", setup_duration_ms, delta_ms);
        }

        sleep_time_us = (measurement_interval_ms - delta_ms) * 1000;
    } else {
        delta_ms = measurement_interval_ms - setup_duration_ms;
        // Catch the case of the run time being exactly the measurement interval. With the current
        // code structure there is no simple way of just running the whole thing again, so sleep for 1ms
        // and reboot.
        if (delta_ms < 1) {
            delta_ms = 1;
        }

        sleep_time_us = delta_ms * 1000;
    }

    // A final sanity check.
    if (sleep_time_us > (measurement_interval_ms * 1000)) {
        ESP_LOGE(TAG, "sleep time us %llu > measurement interval us %llu", sleep_time_us, (measurement_interval_ms * 1000));
        sleep_time_us = measurement_interval_ms * 1000;
    }

    ESP_LOGI(TAG, "Unadjusted sleep_time_us = %llu", sleep_time_us);
    sleep_time_us = (uint64_t)((float)sleep_time_us * config.getSleepAdjustment());
    ESP_LOGI(TAG, "Adjusted sleep_time_us = %llu", sleep_time_us);

    float f_s_time = (float)sleep_time_us / 1000000.0f;
    ESP_LOGI(TAG, "Run took %llu ms, going to sleep at: %s, for %.2f s", setup_duration_ms, iso8601(), f_s_time);
    Serial.flush();

    esp_sleep_enable_timer_wakeup(sleep_time_us);
    esp_deep_sleep_start();
}

void shutdown(void) {
    ESP_LOGI(TAG, "Shutting down");

    SPIFFS.end();

    if (r5_ok) {
        log_to_sdcard("r5.modulePowerOff");
        r5.modulePowerOff();
    } else {
        log_to_sdcard("[E] r5_ok was false");
    }

    cat_m1.power_supply(false);
    delay(20);
    BatteryMonitor::sleep();
    SolarMonitor::sleep();

    disable12V();
    delay(20);

    log_to_sdcard("power down SD card");
    SD.end();
    digitalWrite(SD_CARD_ENABLE, LOW);

    // Use the handle to delete the task.
    if (xHandle != nullptr) {
        vTaskDelete(xHandle);
    }
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
