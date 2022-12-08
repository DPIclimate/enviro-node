#include "cli/CLI.h"

Stream* cliStream = nullptr;
static char cmd[CLI::MAX_CLI_CMD_LEN+1];
static char msg[CLI::MAX_CLI_MSG_LEN+1];

static const CLI_Command_Definition_t configCmd = {
        CLIConfig::cmd.c_str(),
        "config:\r\n Load, save, or list the node configuration\r\n",
        CLIConfig::enter_cli,
        -1
};

static const CLI_Command_Definition_t intervalCmd = {
        CLIConfigIntervals::cmd.c_str(),
        "interval:\r\n Configure acquisition interval settings\r\n",
        CLIConfigIntervals::enter_cli,
        -1
};

static const CLI_Command_Definition_t sdi12Cmd = {
        CLISdi12::cmd.c_str(),
        "sdi12:\r\n Configure and interface with SDI-12 sensors\r\n",
        CLISdi12::enter_cli,
        -1
};

static const CLI_Command_Definition_t catM1Cmd = {
        CLICatM1::cmd.c_str(),
        "c1:\r\n Configure and interface with Cat M1 modem\r\n",
        CLICatM1::enter_cli,
        -1
};

static const CLI_Command_Definition_t mqttCmd = {
        CLIMQTT::cmd.c_str(),
        "mqtt:\r\n Configure MQTT connection parameters\r\n",
        CLIMQTT::enter_cli,
        -1
};

//static const CLI_Command_Definition_t btCmd = {
//        "bt",
//        "bt:\r\n Connect and communicate over Bluetooth UART\r\n",
//        CLIBluetooth::enter_cli,
//        -1
//};

void CLI::init() {
    ESP_LOGI(CLI_TAG, "Registering CLI commands");
    FreeRTOS_CLIRegisterCommand(&configCmd);
    FreeRTOS_CLIRegisterCommand(&intervalCmd);
    FreeRTOS_CLIRegisterCommand(&sdi12Cmd);
    FreeRTOS_CLIRegisterCommand(&catM1Cmd);
    FreeRTOS_CLIRegisterCommand(&mqttCmd);
    //FreeRTOS_CLIRegisterCommand(&btCmd);
}

void CLI::repl(Stream& io) {
    // cliStream is used by the SDI-12 & Cat M1 passthrough modes, so must be set from here before either
    // mode is used.

    cliStream = &io;
    io.println("Entering repl");

    while (true) {
        io.print("$ ");
        while (!io.available()) {
            yield();
        }

        size_t len = readFromStreamUntil(io, '\n', cmd, MAX_CLI_CMD_LEN);
        if (len > 1) {
            cmd[len] = 0;
            stripWS(cmd);
            if (!strcmp("exit", cmd)) {
                io.println("Exiting repl");
                return;
            }

            BaseType_t rc = pdTRUE;
            while (rc != pdFALSE) {
                memset(msg, 0, sizeof(msg));
                rc = FreeRTOS_CLIProcessCommand(cmd, msg, MAX_CLI_MSG_LEN);
                if (msg[0] != 0) {
                    io.print(msg);
                    if (BluetoothServer::is_device_connected()) {
                        BluetoothServer::notify_device(msg);
                    }
                }
            }
        }
    }
}
