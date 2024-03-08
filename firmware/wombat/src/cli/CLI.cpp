/**
 * @file CLI.cpp
 *
 * @brief FreeRTOS CLI handler.
 *
 * @date January 2023
 */
#include "cli/CLI.h"
#include "cli/peripherals/sdi12.h"

#include "cli/peripherals/cli_power.h"
#include "cli/peripherals/sd_card.h"
#include "cli/peripherals/cli_spiffs.h"
#include "cli/device_config/acquisition_intervals.h"
#include "cli/device_config/mqtt_cli.h"
#include "cli/device_config/ftp_cli.h"
#include "cli/device_config/config_cli.h"

//! Command line stream
Stream *CLI::cliInput = nullptr;
Stream *CLI::cliOutput = nullptr;

//! Command buffer
static char cmd[CLI::MAX_CLI_CMD_LEN+1];
//! Message buffer
static char msg[CLI::MAX_CLI_MSG_LEN+1];

//! Node configuration commands
static const CLI_Command_Definition_t configCmd = {
        CLIConfig::cmd.c_str(),
        "config:\r\n Load, save, or list the node configuration or reboot\r\n",
        CLIConfig::enter_cli,
        -1
};

//! Node measurement interval commands
static const CLI_Command_Definition_t intervalCmd = {
        CLIConfigIntervals::cmd.c_str(),
        "interval:\r\n Configure acquisition interval settings\r\n",
        CLIConfigIntervals::enter_cli,
        -1
};

//! SDI-12 commands
static const CLI_Command_Definition_t sdi12Cmd = {
        CLISdi12::cmd.c_str(),
        "sdi12:\r\n Configure and interface with SDI-12 sensors\r\n",
        CLISdi12::enter_cli,
        -1
};

//! CAT-M1 modem commands
static const CLI_Command_Definition_t catM1Cmd = {
        CLICatM1::cmd.c_str(),
        "c1:\r\n Configure and interface with Cat M1 modem\r\n",
        CLICatM1::enter_cli,
        -1
};

//! MQTT setup and connection commands
static const CLI_Command_Definition_t mqttCmd = {
        CLIMQTT::cmd.c_str(),
        "mqtt:\r\n Configure MQTT connection parameters\r\n",
        CLIMQTT::enter_cli,
        -1
};

//! FTP setup and connection commands
static const CLI_Command_Definition_t ftpCmd = {
        CLIFTP::cmd.c_str(),
        "ftp:\r\n Configure FTP connection parameters\r\n",
        CLIFTP::enter_cli,
        -1
};

//! Power commands
static const CLI_Command_Definition_t powerCmd = {
        CLIPower::cmd.c_str(),
        "pwr:\r\n Show battery and solar information\r\n",
        CLIPower::enter_cli,
        -1
};

//! SD card commands
static const CLI_Command_Definition_t sdCmd = {
        CLISDCard::cmd.c_str(),
        "sd:\r\n Access the SD card\r\n",
        CLISDCard::enter_cli,
        -1
};

//! SPIFFS commands
static const CLI_Command_Definition_t spiffsCmd = {
    CLISPIFFS::cmd.c_str(),
    "sd:\r\n Access the SPIFFS filesystem\r\n",
    CLISPIFFS::enter_cli,
    -1
};

/**
 * @brief Initialise the FreeRTOS command line interface.
 */
void CLI::init() {
    ESP_LOGI(CLI_TAG, "Registering CLI commands");
    FreeRTOS_CLIRegisterCommand(&configCmd);
    FreeRTOS_CLIRegisterCommand(&intervalCmd);
    FreeRTOS_CLIRegisterCommand(&sdi12Cmd);
    FreeRTOS_CLIRegisterCommand(&catM1Cmd);
    FreeRTOS_CLIRegisterCommand(&mqttCmd);
    FreeRTOS_CLIRegisterCommand(&ftpCmd);
    FreeRTOS_CLIRegisterCommand(&powerCmd);
    FreeRTOS_CLIRegisterCommand(&sdCmd);
    FreeRTOS_CLIRegisterCommand(&spiffsCmd);
}

/**
 * @brief Read-eval-print loop (REPL) for the command line interface (CLI).
 *
 * This function implements a REPL for the CLI. It reads a command from the
 * user, processes the command, and prints the result of the command. The REPL
 * loop is exited by typing the "exit" command.
 *
 * @param io A stream object used for reading input from the user and writing
 *           output to the user.
 */
void CLI::repl(Stream& input, Stream& output) {
    // cliStream is used by the SDI-12 & Cat M1 passthrough modes, so must be
    // set from here before either mode is used.
    cliInput = &input;
    cliOutput = &output;
    output.println("Entering repl");

    while (true) {
        output.print("$ ");
        while (!input.available()) {
            taskYIELD();
        }

        size_t len = readFromStreamUntil(input, '\n', cmd, MAX_CLI_CMD_LEN);
        if (len > 1) {
            if (len >= (MAX_CLI_CMD_LEN - 1)) {
                ESP_LOGE("CLI", "\r\nCommand too long");
                continue;
            }

            cmd[len] = 0;
            wombat::stripWS(cmd);
            if (!strcmp("exit", cmd)) {
                break;
            }

            BaseType_t rc = pdTRUE;
            while (rc != pdFALSE) {
                memset(msg, 0, sizeof(msg));
                rc = FreeRTOS_CLIProcessCommand(cmd, msg, MAX_CLI_MSG_LEN);
                if (msg[0] != 0) {
                    output.print(msg);
                }
            }
        }
    }

    output.println("Exiting repl");
}
