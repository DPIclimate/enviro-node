// This is so vscode can find definitions.
#include <Arduino.h>
#include "WDT.h"

static bool _initialized = false;

void wdt_init() {
    // RTCZero has already set GCLK(2) up with the external 32 KHz oscillator with the
    // same division factor, so all that needs to be done is to use GCLK(2) as the source
    // for the WDT.
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2;

    // Disable during configuration.
    wdt_disable();

    WDT->INTENCLR.bit.EW = 1;   // Disable early warning interrupt
    WDT->CONFIG.bit.PER = 0x0B; // Set period for chip reset - 16384 input clock cycles, ~16s
    WDT->CTRL.bit.WEN = 0;      // Disable window mode
    while (WDT->STATUS.bit.SYNCBUSY);

    NVIC_DisableIRQ(WDT_IRQn);
    NVIC_ClearPendingIRQ(WDT_IRQn);

    _initialized = true;
}

void wdt_enable() {
    // Enable the watchdog with a period up to the specified max period in
    // milliseconds.

    // Review the watchdog section from the SAMD21 datasheet section 17:
    // http://www.atmel.com/images/atmel-42181-sam-d21_datasheet.pdf

    if (!_initialized) {
        wdt_init();
    }

    wdt_reset();              // Clear watchdog interval
    WDT->CTRL.bit.ENABLE = 1; // Start watchdog now!
    while (WDT->STATUS.bit.SYNCBUSY);

    log_msg("WDT on");
}

void wdt_disable() {
    WDT->CTRL.bit.ENABLE = 0;
    while (WDT->STATUS.bit.SYNCBUSY);

    log_msg("WDT off");
}

void wdt_reset() {
    // Write the watchdog clear key value (0xA5) to the watchdog
    // clear register to clear the watchdog timer and reset it.
    while (WDT->STATUS.bit.SYNCBUSY);
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
}
