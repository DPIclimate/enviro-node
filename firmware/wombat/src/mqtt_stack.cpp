
#include "Utils.h"
#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"
#include "cli/CLI.h"

#define TAG "mqtt_stack"

#define MAX_RSP 64
static char rsp[MAX_RSP + 1];

// This buffer is allocated in the mqtt_login method but the script should be
// run later, so it must be freed elsewhere after the run.
char *script = nullptr;

//void registrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}

//void epsRegistrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}


static volatile int lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
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
        case SARA_R5_MQTT_COMMAND_PUBLISHFILE:
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

    if (!connect_to_internet()) {
        ESP_LOGE(TAG, "Could not connect to internet");
        return false;
    }

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
    lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
    mqttLoginOk = false;
    while (!mqttGotURC && lastCmd != SARA_R5_MQTT_COMMAND_LOGIN) {
        r5.bufferedPoll();
    }

    if (!mqttLoginOk) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        return false;
    }

    ESP_LOGI(TAG, "Connected to MQTT");

    // If script is null, it is ok to check for a config download. If script already has a value it means
    // a script has been downloaded and may be executing, so don't check for or download another one.
    if (script == nullptr) {
        String cmd_topic("wombat/");
        cmd_topic += config.node_id;
        r5.subscribeMQTTtopic(1, cmd_topic);

        ESP_LOGI(TAG, "Checking for a config script, waiting for subscribe URC");
        lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
        lastResult = 0;
        mqttGotURC = false;
        int attempts = 30;
        while (!mqttGotURC && attempts > 0) {
            delay(100);
            r5.bufferedPoll();
            attempts--;
        }

        if (lastCmd == SARA_R5_MQTT_COMMAND_SUBSCRIBE && lastResult != 0) {
            lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
            lastResult = 0;
            for (int i = 0; i < 50; i++) {
                delay(20);
                mqttGotURC = false;
                r5.bufferedPoll();
                if (mqttGotURC && lastCmd == SARA_R5_MQTT_COMMAND_READ) {
                    break;
                }
            }

            if (lastCmd == SARA_R5_MQTT_COMMAND_READ && lastResult > 0) {
                ESP_LOGI(TAG, "Config download waiting.");
                int qos;
                String topic;
                int bytes_read = 0;
                memset(g_buffer, 0, MAX_G_BUFFER + 1);
                SARA_R5_error_t err = r5.readMQTT(&qos, &topic, (uint8_t *) g_buffer, MAX_G_BUFFER, &bytes_read);
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

                    script = static_cast<char *>(malloc(bytes_read + 1));
                    if (script != nullptr) {
                        memcpy(script, g_buffer, bytes_read);
                        script[bytes_read] = 0;
                    } else {
                        ESP_LOGE(TAG, "Could not allocate memory for script");
                    }
                } else {
                    ESP_LOGW(TAG, "Did not read any config data.");
                }
            } else {
                ESP_LOGI(TAG, "No config script");
            }
        } else {
            ESP_LOGE(TAG, "MQTT subscribe failed");
        }
    } else {
        ESP_LOGI(TAG, "Already have a config script, not checking for a new one");
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

bool mqtt_publish(String &topic, const char *const msg, size_t msg_len) {
    SARA_R5_error_t err = SARA_R5_ERROR_INVALID;
    if (msg_len < MAX_MQTT_DIRECT_MSG_LEN) {
        ESP_LOGD(TAG, "Direct publish message: %s/%s", topic.c_str(), msg);
        err = r5.mqttPublishBinaryMsg(topic, msg, msg_len, 1);
    } else {
        ESP_LOGD(TAG, "Publish from file: %s/%s", topic.c_str(), msg);
        static const String mqtt_msg_filename("mqtt.json");
        r5.deleteFile(mqtt_msg_filename);
        r5.appendFileContents(mqtt_msg_filename, msg, static_cast<int>(msg_len));
        memset(g_buffer, 0, sizeof(g_buffer));
        err = r5.mqttPublishFromFile(topic, mqtt_msg_filename);

//        err = r5.getFileContents(mqtt_msg_filename, g_buffer);
//        if (err == SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
//            ESP_LOGD(TAG, "Msg content from modem:\n%s", g_buffer);
//        } else {
//            ESP_LOGE(TAG, "Failed to read file content from R5");
//        }
    }

    if (err != SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Publish failed");
        return false;
    }

    delay(20);
    mqttGotURC = false;
    mqttPublishOk = false;
    while (!mqttGotURC) {
        r5.bufferedPoll();
        delay(20);
    }
    return mqttPublishOk;
}
