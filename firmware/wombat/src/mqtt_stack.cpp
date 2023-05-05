
#include "CAT_M1.h"
#include "Utils.h"
#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"
#include "cli/CLI.h"

#define TAG "mqtt_stack"

#define MAX_RSP 64
static char rsp[MAX_RSP + 1];

#define MAX_BUF 2048
static char buf[MAX_BUF + 1];

//void registrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}

//void epsRegistrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}


static volatile int lastCmd = -1;
static volatile int lastResult = -1;
static volatile bool mqttGotURC = false;
static volatile bool mqttLoginOk = false;
static volatile bool mqttLogoutOk = false;
static volatile bool mqttPublishOk = false;
static volatile bool mqttGotSubscribeURC = false;
static volatile bool mqttGotReadURC = false;

void mqttCmdCallback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    lastCmd = cmd;
    lastResult = result;
    if (result == 0) {
        int e1, e2;
        r5.getMQTTprotocolError(&e1, &e2);
        ESP_LOGE(TAG, "MQTT op failed: %d, %d", e1, e2);
    }

    switch (cmd) {
        case SARA_R5_MQTT_COMMAND_LOGOUT:
            mqttLogoutOk = (result == 1);
            break;
        case SARA_R5_MQTT_COMMAND_LOGIN:
            mqttLoginOk = (result == 1);
            break;
        case SARA_R5_MQTT_COMMAND_SUBSCRIBE:
            mqttGotSubscribeURC = true;
            break;
        case SARA_R5_MQTT_COMMAND_READ:
            mqttGotSubscribeURC = true;
            break;
        case SARA_R5_MQTT_COMMAND_PUBLISH:
        case SARA_R5_MQTT_COMMAND_PUBLISHBINARY:
            mqttPublishOk = (result == 1);
            break;
    }

    mqttGotURC = true;
}

bool mqtt_login(void) {
    ESP_LOGI(TAG, "Starting modem and login");

    DeviceConfig &config = DeviceConfig::get();
    std::string &host = config.getMqttHost();
    uint16_t port = config.getMqttPort();
    std::string &user = config.getMqttUser();
    std::string &password = config.getMqttPassword();

    r5.setMQTTserver(host.c_str(), port);
    delay(20);

    r5.setMQTTcredentials(user.c_str(), password.c_str());
    delay(20);

    String client_id("w");
    client_id += config.node_id;
    ESP_LOGI(TAG, "MQTT client id: %s", client_id.c_str());
    r5.setMQTTclientId(client_id);
    delay(20);

    r5.setMQTTCommandCallback(mqttCmdCallback);
    delay(20);

    if (r5.connectMQTT() != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        return false;
    }
    delay(20);

    // Wait for UMQTTC URC to show connection success/failure.
    mqttGotURC = false;
    while ( ! mqttGotURC) {
        r5.bufferedPoll();
    }

    if ( ! mqttLoginOk) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        return false;
    }

    ESP_LOGI(TAG, "Connected to MQTT");

    String cmd_topic("wombat/");
    cmd_topic += config.node_id;
    r5.subscribeMQTTtopic(1, cmd_topic);
    mqttGotURC = false;

    ESP_LOGI(TAG, "Waiting for subscribe URC");
    while ( ! mqttGotURC) {
        r5.bufferedPoll();
    }

    ESP_LOGI(TAG, "Checking if read URC arrived after subscribe");
    r5.bufferedPoll();
    if (lastCmd == SARA_R5_MQTT_COMMAND_READ && lastResult > 0) {
        ESP_LOGI(TAG, "Config download waiting.");
        int qos;
        String topic;
        int bytes_read = 0;
        memset(g_buffer, 0, MAX_G_BUFFER+1);
        SARA_R5_error_t err = r5.readMQTT(&qos, &topic, (uint8_t *)g_buffer, MAX_G_BUFFER, &bytes_read);
        if (bytes_read > 0) {
            // Publish a zero length message to clear the retained message.
            r5.mqttPublishTextMsg(cmd_topic, "", 0, true);
            uint8_t counter = 0;
            mqttGotURC = false;
            while (mqttGotURC != true && counter < 50) {
                r5.bufferedPoll();
                delay(20);
                counter++;
            }

            char* script = static_cast<char*>(malloc(bytes_read + 1));
            if (script != nullptr) {
                memcpy(script, g_buffer, bytes_read);
                script[bytes_read] = 0;

                // Apply config. This is done after publishing the empty message because
                // if the config script contains a "config reboot" command and the retained
                // config message has not been cleared then the node gets stuck in a reboot
                // loop.
                char *strtok_ctx;
                char *cmd = strtok_r(script, "\n", &strtok_ctx);
                while (cmd != nullptr) {
                    stripWS(cmd);
                    ESP_LOGI(TAG, "Command: [%s]", cmd);
                    BaseType_t rc = pdTRUE;
                    while (rc != pdFALSE) {
                        rc = FreeRTOS_CLIProcessCommand(cmd, rsp, MAX_RSP);
                        ESP_LOGI(TAG, "%s", rsp);
                    }

                    cmd = strtok_r(nullptr, "\n", &strtok_ctx);
                }

                free(script);
            } else {
                ESP_LOGE(TAG, "Could not allocate memory for script");
            }
        } else {
            ESP_LOGW(TAG, "Did not read any config data.");
        }
    }

    return true;
}

bool mqtt_logout(void) {
    ESP_LOGI(TAG, "logout");
    r5.disconnectMQTT();
    delay(20);

    mqttGotURC = false;
    while (!mqttGotURC) {
        r5.bufferedPoll();
    }
    return mqttLogoutOk;
}

bool mqtt_publish(String& topic, const char * const msg) {
    ESP_LOGI(TAG, "Publish message: %s/%s", topic.c_str(), msg);
    SARA_R5_error_t err = r5.mqttPublishBinaryMsg(topic, msg);
    if (err != SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Publish failed");
        return false;
    }

    delay(20);
    mqttGotURC = false;
    while (!mqttGotURC) {
        r5.bufferedPoll();
    }
    mqttGotURC = false;
    return mqttPublishOk;
}
