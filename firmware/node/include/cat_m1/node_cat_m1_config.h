#ifndef NODE_CAT_M1_CONFIG_H
#define NODE_CAT_M1_CONFIG_H

#include <Arduino.h>

/* SARA-R410-02B AT-commands */

/**
 * Basis of all AT commands
 *
 * @note
 * Append "AT" or CAT_M1_AT to the start of each command
 *
 * @example
 * To identify the module concat CAT_M1_AT ("AT") and
 * CAT_M1_IDENTIFY ("I") to give "ATI"
 *
 */
static const char CAT_M1_AT_COMMAND[] = "AT";

/**
 * Model identification
 * @example
 * SARA-R410M-02B
 */
static const char CAT_M1_MODEL_ID_COMMAND[] = "+GMM";

/**
 * Firmware version number
 * @example
 * L0.0.00.00.05.08 [Apr 17 2019 19:34:02]
 * @note
 * Also contains firmware date of deployment
 */
static const char CAT_M1_FIRMWARE_VERSION_COMMAND[] = "+CGMR";

/**
 * Identify the connected device
 * @example
 * Manufacturer: u-blox
 * Model: SARA-R410M-02B
 * Revision: L0.0.00.00.05.08 [Apr 17 2019 19:34:02]
 * SVN: 03
 * IMEI: 355523762489807
 */
static const char CAT_M1_IDENTIFY_COMMAND[] = "I";

/**
 * Sim-Card identification number
 * @example
 * +CCID: 89610135002619526296
 */
static const char CAT_M1_SIM_IDENTIFY_COMMAND[] = "+CCID";

/**
 * Cellular activity status
 * @example
 * +CPAS: 0
 * @note
 * List of states:
 * - 0: ready (MT allows commands from DTE)
 * - 1: unavailable (MT does not allow commands from DTE)
 * - 2: unknown (MT is not guaranteed to respond to instructions)
 * - 3: ringing (MT is ready for commands from DTE, but the ringer is active)
 * - 4: call in progress (MT is ready for commands from DTE, but a call is in
 * progress, e.g. call active, hold, disconnecting)
 * - 5: asleep(ME is unable to process commands from DTE because it is in a
 * low functionality state)
 * .
 */
static const char CAT_M1_PHONE_STATUS_COMMAND[] = "+CPAS";

/**
 * Power off module
 * @example
 * OK
 */
static const char CAT_M1_POWER_DOWN_COMMAND[] = "+CPWROFF";

/**
 * Set module functionality state
 * @example
 * AT+CFUN=?
 * +CFUN: (0,1,4,15),(0-1)
 */
static const char CAT_M1_MODULE_STATE_COMMAND[] = "+CFUN";

/**
 * Mode functionality types
 *
 * Used in conjunction with CAT_M1_MODULE_STATE to set the modules state.
 * @example
 * AT+CFUN=1
 */
typedef enum {
    // Sets the MT to minimum functionality (disable both transmit and receive RF
    // circuits by deactivating both CS and PS services)
    MINIMUM_FUNCTION = 0,
    // DEFAULT. Sets the MT to full functionality,e.g.from airplane
    // mode or minimum functionality
    FULL_FUNCTION = 1,
    // Disables both transmit and receive RF circuits by deactivating both
    // CS and PS services and sets the MT into airplane mode
    AIRPLANE_MODE = 4,
    // Fast and safe power-off,the command triggers a fast shutdown, without
    // sending a detach request to the network, with storage of current
    // settings in module's non-volatile memory.
    FAST_SHUTDOWN = 10,
    // MT silent reset without resetting the SIM card
    SILENT_RESET_NO_RESET_SIM = 15,
    // MT silent reset with a reset of the SIM card
    SILENT_RESET_RESET_SIM = 16,
    // Sets the MT to minimum functionality by deactivating CS and PS services
    // and the SIM card. Re-enable the SIM card by means of AT+CFUN=0, 1, 4.
    DEACTIVATE = 19
} CAT_M1_MODULE_STATE;

/**
 * Module indicator states command
 *
 * @example
 * AT+CIND?
 * +CIND: 5,0,0,0,0,0,0,0,0,0,0,2
 */
static const char CAT_M1_INDICATOR_STATE_COMMAND[] = "+CIND";

/**
 * Set and get current time on the module
 *
 * @example
 * Set: AT+CCLK="27/10/21,11:50:00+11"
 * OK
 * Get: AT+CCLK?
 * +CCLK: "27/10/21,11:50:26+11"
 */
static const char CAT_M1_CLOCK_COMMAND[] = "+CCLK";

/**
 * Set greeting text. Useful for recognising when module starts up.
 *
 * @example
 * AT+CGST=1,"Greeting text here"
 * OK
 */
static const char CAT_M1_GREETING_TEXT[] = "+CGST";
static const char CAT_M1_DEFAULT_GREETING_TEXT[] = "[CAT-M1]: Module started.";

/**
 * Forces an attempt to select and register with the GSM/LTE network operator,
 * that can be chosen in the list of network operators returned by the test
 * command, that triggers a PLMN scan on all supported bands.
 */
 static const char CAT_M1_OPERATOR_SELECTION[] = "+COPS";



#endif // NODE_CAT_M1_CONFIG_H
