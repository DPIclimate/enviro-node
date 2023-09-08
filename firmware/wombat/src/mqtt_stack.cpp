
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

static CommandURCVector<SARA_R5_mqtt_command_opcode_t> urcs;

void mqttCmdCallback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    log_to_sdcardf("mqtt cb cmd: %d, result: %d", cmd, result);

    urcs.addPair(static_cast<SARA_R5_mqtt_command_opcode_t>(cmd), result);

    if (result == 0) {
        int e1, e2;
        r5.getMQTTprotocolError(&e1, &e2);
        urcs.back().err1 = e1;
        urcs.back().err2 = e2;

        ESP_LOGE(TAG, "MQTT op failed: %d, %d", e1, e2);
    }
}

bool mqtt_login(void) {
    ESP_LOGI(TAG, "Starting modem and login");
    log_to_sdcard("mqtt_login");

    DeviceConfig &config = DeviceConfig::get();
    std::string &host = config.getMqttHost();
    uint16_t port = config.getMqttPort();
    std::string &user = config.getMqttUser();
    std::string &password = config.getMqttPassword();

    if (!connect_to_internet()) {
        ESP_LOGE(TAG, "Could not connect to internet");
        log_to_sdcard("[E] cti failed");
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

    SARA_R5_error_t err = r5.connectMQTT();
    if (err != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed: %d", err);
        log_to_sdcardf("[E] r5.connectMQTT AT cmd failed: %d", err);
        return false;
    }
    delay(20);

    // Wait for UMQTTC URC to show connection success/failure.
    log_to_sdcard("Waiting for mqtt login URC");
    int result = -1;
    bool found_urc = urcs.waitForURC(SARA_R5_MQTT_COMMAND_LOGIN, &result, 60, 500);

    if (result != 1) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        log_to_sdcard("[E] mqtt login URC not received or failed");
        return false;
    }

    log_to_sdcard("mqtt login ok");
    ESP_LOGI(TAG, "Connected to MQTT");

    // If script is null, it is ok to check for a config download. If script already has a value it means
    // a script has been downloaded and may be executing, so don't check for or download another one.
    if (script == nullptr) {
        String cmd_topic("wombat/");
        cmd_topic += config.node_id;
        if (r5.subscribeMQTTtopic(1, cmd_topic) != SARA_R5_ERROR_SUCCESS) {
            ESP_LOGW(TAG, "Sub at cmd failed");
            log_to_sdcard("[E] mqtt sub at cmd subscribe failed");
            // Returning true because the login succeeded.
            return true;
        }

        // So, more than one call to the URC callback can happen in a single call to bufferedPoll. If the URCs
        // arrive in quick succession such as an MQTT sub URC followed immediately by a read URC, the code
        // below fails because the line that checks lastCmd after issuing the sub call sees lastCmd is
        // 6 and not 4!

        ESP_LOGI(TAG, "Checking for a config script, waiting for subscribe URC");

        result = -1;
        urcs.waitForURC(SARA_R5_MQTT_COMMAND_SUBSCRIBE, &result, 30, 100);
        ESP_LOGI(TAG, "out of sub urc loop, checking values: result = %d", result);
        log_to_sdcardf("out of sub urc loop, checking values: result = %d", result);

        if (result == 1) {
            // After a subscribe, a read URC will be sent by the modem if there is a message
            // waiting in the topic.
            result = -1;
            urcs.waitForURC(SARA_R5_MQTT_COMMAND_READ, &result, 20, 100);

            ESP_LOGI(TAG, "out of read urc loop, result = %d", result);
            log_to_sdcardf("out of read urc loop, result = %d", result);

            if (result > 0) {
                ESP_LOGI(TAG, "Config download waiting.");
                int qos;
                String topic;
                int bytes_read = 0;
                memset(g_buffer, 0, MAX_G_BUFFER + 1);
                err = r5.readMQTT(&qos, &topic, (uint8_t *) g_buffer, 2048, &bytes_read);
                if (err == SARA_R5_ERROR_SUCCESS && bytes_read > 0) {
                    // Publish a zero length message to clear the retained message.
                    // This will result in another read URC, because there is a new message
                    // in the topic, but we're not interested in that and don't look for it.
                    if (r5.mqttPublishTextMsg(cmd_topic, "", 0, true) == SARA_R5_ERROR_SUCCESS) {
                        result = -1;
                        urcs.waitForURC(SARA_R5_MQTT_COMMAND_PUBLISH, &result, 30, 100);

                        if (result != 1) {
                            ESP_LOGW(TAG, "URC not received after clearing config msg");
                            log_to_sdcard("[W] URC not received after clearing config msg");
                        }
                    } else {
                        ESP_LOGW(TAG, "publish empty msg failed");
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
            log_to_sdcard("[E] no mqtt sub urc");
        }
    } else {
        ESP_LOGI(TAG, "Already have a config script, not checking for a new one");
    }

    log_to_sdcard("mqtt login ok");
    return true;
}

bool mqtt_logout(void) {
    ESP_LOGI(TAG, "logout");
    log_to_sdcard("mqtt logging out");
    r5.disconnectMQTT();
    delay(20);

    int result = -1;
    urcs.waitForURC(SARA_R5_MQTT_COMMAND_LOGOUT, &result, 45, 500);

    // Remove any leftover URCs. There might be some subscribe, read, or write
    // URCs. Eg if a config script is published between when the Wombat checks
    // for it after login and when it logs out, there will be a read URC.
    urcs.clear();

    if (result != 1) {
        log_to_sdcard("[E] mqtt logout failed");
    }

    return result == 1;
}

bool mqtt_publish(String &topic, const char *const msg, size_t msg_len) {
    log_to_sdcard("mqtt_publish");
    SARA_R5_mqtt_command_opcode_t expected_cmd = SARA_R5_MQTT_COMMAND_INVALID;
    SARA_R5_error_t err = SARA_R5_ERROR_INVALID;
    if (msg_len < MAX_MQTT_DIRECT_MSG_LEN) {
        ESP_LOGD(TAG, "Direct publish message: %s/%s", topic.c_str(), msg);
        log_to_sdcard("using direct publish");
        err = r5.mqttPublishBinaryMsg(topic, msg, msg_len, 1);
        expected_cmd = SARA_R5_MQTT_COMMAND_PUBLISHBINARY;
    } else {
        ESP_LOGD(TAG, "Publish from file: %s/%s", topic.c_str(), msg);
        log_to_sdcard("using publish from file");
        static const String mqtt_msg_filename("mqtt.json");
        r5.deleteFile(mqtt_msg_filename);
        r5.appendFileContents(mqtt_msg_filename, msg, static_cast<int>(msg_len));
        memset(g_buffer, 0, sizeof(g_buffer));
        err = r5.mqttPublishFromFile(topic, mqtt_msg_filename);
        expected_cmd = SARA_R5_MQTT_COMMAND_PUBLISHFILE;

//        err = r5.getFileContents(mqtt_msg_filename, g_buffer);
//        if (err == SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
//            ESP_LOGD(TAG, "Msg content from modem:\n%s", g_buffer);
//        } else {
//            ESP_LOGE(TAG, "Failed to read file content from R5");
//        }
    }

    delay(20);

    if (err != SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Publish failed");
        log_to_sdcardf("[E] pub failed, err: %d", err);
        return false;
    }

    log_to_sdcard("waiting for pub urc");
    delay(20);

    int result = -1;
    urcs.waitForURC(expected_cmd, &result, 60, 500);
    return result == 1;
}
