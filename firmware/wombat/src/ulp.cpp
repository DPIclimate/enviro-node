#include <driver/rtc_io.h>
#include "driver/gpio.h"
#include "esp32/ulp.h"
#include "../ulp/ulp_main.h"

#include "ulp.h"
#include "DeviceConfig.h"

#define TAG "ulp"

#define ULPSLEEP_uS          2000         // amount in microseconds the ULP co-processor sleeps
#define RTC_ULP_PIN         GPIO_NUM_27 // Has a pull-up on the board

// To get this the ULP code compiled when using platformio see:
// https://github.com/likeablob/ulptool-pio/tree/feat-pio-integration
//
// For the first build in a clean repo you must comment out this entire file and run the build. That generates the
// ULP related source files. Then uncomment this file and subsequent builds will work.
//
// Connect the rain gauge between the 'Digital' and 'GND' pins on the Wombat.

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

void initULP(void) {
    // Only initialise the ULP on the first boot. Subsequent boots are deep sleep wakeups.
    if (DeviceConfig::get().getBootCount() > 0) {
        return;
    }

    ESP_LOGI(TAG, "Initialising ULP");

    esp_err_t err_load = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err_load);

    // GPIO used for pulse counting.
    assert(rtc_gpio_is_valid_gpio(RTC_ULP_PIN) && "GPIO used for pulse counting must be an RTC IO");

    // Initialize some variables used by ULP program.
    // Each 'ulp_xyz' variable corresponds to 'xyz' variable in the ULP program.
    // These variables are declared in an auto generated header file,
    // 'ulp_main.h', name of this file is defined in component.mk as ULP_APP_NAME.
    // These variables are located in RTC_SLOW_MEM and can be accessed both by the
    // ULP and the main CPUs.
    //
    // Note that the ULP reads only the lower 16 bits of these variables.
    ulp_debounce_counter = 1;
    ulp_debounce_max_count = 1;
    ulp_pulse_edge = 1;
    ulp_next_edge = 1;
    ulp_io_number = rtc_io_number_get(RTC_ULP_PIN); // map from GPIO# to RTC_IO#

    // Initialize selected GPIO as RTC IO, enable input, sets pullup and pulldown.
    rtc_gpio_init(RTC_ULP_PIN);
    rtc_gpio_set_direction(RTC_ULP_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    // The wind speed pin has a pull-up on the board.
    rtc_gpio_pulldown_dis(RTC_ULP_PIN);
    gpio_pullup_en(RTC_ULP_PIN);
    rtc_gpio_hold_en(RTC_ULP_PIN);

    // Set ULP wake up period to T = 2ms.
    ulp_set_wakeup_period(0, ULPSLEEP_uS);

    // Start the program
    esp_err_t err_run = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err_run);
}

uint32_t get_pulse_count(void) {
    // ULP program counts signal edges, convert that to the number of pulses.
    uint32_t pulse_count_from_ulp = (ulp_edge_count & UINT16_MAX) / 2;

    // In case of an odd number of edges, keep one until next time.
    ulp_edge_count = ulp_edge_count % 2;

    return pulse_count_from_ulp;
}

uint32_t get_shortest_pulse(void) {
    // ULP program saves shortest pulse.
    uint32_t pulse_time_min = (ulp_pulse_min & UINT16_MAX) * ULPSLEEP_uS;

    // Reset shortest edge.
    ulp_pulse_min = 0;

    return pulse_time_min;
}
