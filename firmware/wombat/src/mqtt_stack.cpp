
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

std::vector<std::pair<SARA_R5_mqtt_command_opcode_t, int>> urcs;

static volatile int lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
static volatile int lastResult = -1;
static volatile bool mqttGotURC = false;
static volatile bool mqttLoginOk = false;
static volatile bool mqttLogoutOk = false;
static volatile bool mqttPublishOk = false;
//static volatile bool mqtt_subscribe_ok = false;
//static volatile bool mqtt_read_ok = false;

void mqttCmdCallback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    log_to_sdcardf("mqtt cb cmd: %d, result: %d", cmd, result);

    std::pair<SARA_R5_mqtt_command_opcode_t, int> cmd_result(static_cast<SARA_R5_mqtt_command_opcode_t>(cmd), result);
    urcs.push_back(cmd_result);

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
//        case SARA_R5_MQTT_COMMAND_SUBSCRIBE:
//            mqttGotSubscribeURC = true;
//            break;
//        case SARA_R5_MQTT_COMMAND_READ:
//            mqttGotSubscribeURC = true;
//            break;
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
    log_to_sdcard("mqtt_login");

    DeviceConfig &config = DeviceConfig::get();
    std::string &host = config.getMqttHost();
    uint16_t port = config.getMqttPort();
    std::string &user = config.getMqttUser();
    std::string &password = config.getMqttPassword();

    if (!connect_to_internet()) {
        ESP_LOGE(TAG, "Could not connect to internet");
        log_to_sdcard("cti failed");
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
        log_to_sdcard("r5.connectMQTT AT cmd failed");
        return false;
    }
    delay(20);

    // Wait for UMQTTC URC to show connection success/failure.
    log_to_sdcard("Waiting for mqtt login URC");
    mqttGotURC = false;
    lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
    mqttLoginOk = false;
    int retries = 30;
    while (!mqttGotURC && lastCmd != SARA_R5_MQTT_COMMAND_LOGIN && retries > 0) {
        delay(1000);
        r5.bufferedPoll();
        retries--;
    }

    if (!mqttLoginOk) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        log_to_sdcard("mqtt login URC not received");
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
            log_to_sdcard("mqtt sub at cmd subscribe failed");
            // Returning true because the login succeeded.
            return true;
        }

        // So, more than one call to the URC callback can happen in a single call to bufferedPoll. If the URCs
        // arrive in quick succession such as an MQTT sub URC followed immediately by a read URC, the code
        // below fails because the line that checks lastCmd after issuing the sub call sees lastCmd is
        // 6 and not 4!

        ESP_LOGI(TAG, "Checking for a config script, waiting for subscribe URC");
        lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
        lastResult = 0;
        mqttGotURC = false;
        int attempts = 30; // Wait 3s
        while (!mqttGotURC && attempts > 0) {
            delay(100);
            r5.bufferedPoll();
            attempts--;
        }

        bool sub_ok = false;
        bool read_ok = false;

        auto urc_iter = urcs.begin();
        while (urc_iter != urcs.end()) {
            if (urc_iter->first == SARA_R5_MQTT_COMMAND_SUBSCRIBE) {
                sub_ok = urc_iter->second == 1;
                ESP_LOGI(TAG, "found mqtt sub urc, result = %d", urc_iter->second);
                log_to_sdcardf("found mqtt sub urc, result = %d", urc_iter->second);
                urc_iter = urcs.erase(urc_iter);
                continue;
            }

            if (urc_iter->first == SARA_R5_MQTT_COMMAND_READ) {
                read_ok = urc_iter->second == 1;
                ESP_LOGI(TAG, "found mqtt read urc, result = %d", urc_iter->second);
                log_to_sdcardf("found mqtt read urc, result = %d", urc_iter->second);
                urc_iter = urcs.erase(urc_iter);
                continue;
            }

            urc_iter++;
        }

        ESP_LOGI(TAG, "out of sub urc loop, checking values: lastCmd = %d, lastResult = %d, sub_ok = %d", lastCmd, lastResult, sub_ok);
        log_to_sdcardf("out of sub urc loop, checking values: lastCmd = %d, lastResult = %d, sub_ok = %d", lastCmd, lastResult, sub_ok);

        if (sub_ok) {
            if ( ! read_ok) {
                ESP_LOGI(TAG, "waiting for read urc");
                lastCmd = SARA_R5_MQTT_COMMAND_INVALID;
                lastResult = 0;
                // Wait 1s
                for (int i = 0; i < 50; i++) {
                    delay(20);
                    mqttGotURC = false;
                    r5.bufferedPoll();
                    if (mqttGotURC && lastCmd == SARA_R5_MQTT_COMMAND_READ) {
                        break;
                    }
                }

                while (urc_iter != urcs.end()) {
                    if (urc_iter->first == SARA_R5_MQTT_COMMAND_READ) {
                        read_ok = urc_iter->second == 1;
                        ESP_LOGI(TAG, "found mqtt read urc, result = %d", urc_iter->second);
                        log_to_sdcardf("found mqtt read urc, result = %d", urc_iter->second);
                        urc_iter = urcs.erase(urc_iter);
                        continue;
                    }

                    urc_iter++;
                }
            }

            ESP_LOGI(TAG, "out of read urc loop, checking values: lastCmd = %d, lastResult = %d, read_ok = %d", lastCmd, lastResult, read_ok);
            log_to_sdcardf("out of read urc loop, checking values: lastCmd = %d, lastResult = %d, read_ok = %d", lastCmd, lastResult, read_ok);

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
            log_to_sdcard("no mqtt sub urc");
        }
    } else {
        ESP_LOGI(TAG, "Already have a config script, not checking for a new one");
    }

    log_to_sdcard("mqtt login returning true");

    return true;
}

bool mqtt_logout(void) {
    ESP_LOGI(TAG, "logout");
    log_to_sdcard("mqtt logging out");
    r5.disconnectMQTT();
    delay(20);

    mqttGotURC = false;
    int retries = 30;
    while (!mqttGotURC && retries > 0) {
        delay(1000);
        r5.bufferedPoll();
        retries--;
    }

    log_to_sdcardf("mqtt log out result got URC = %d, lastCmd = %d, lastResult = %d, mqttLogoutOk = %d", mqttGotURC, lastCmd, lastResult, mqttLogoutOk);
    return mqttLogoutOk;
}

bool mqtt_publish(String &topic, const char *const msg, size_t msg_len) {
    log_to_sdcard("mqtt_publish");
    SARA_R5_error_t err = SARA_R5_ERROR_INVALID;
    if (msg_len < MAX_MQTT_DIRECT_MSG_LEN) {
        ESP_LOGD(TAG, "Direct publish message: %s/%s", topic.c_str(), msg);
        log_to_sdcard("using direct publish");
        err = r5.mqttPublishBinaryMsg(topic, msg, msg_len, 1);
    } else {
        ESP_LOGD(TAG, "Publish from file: %s/%s", topic.c_str(), msg);
        log_to_sdcard("using publish from file");
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

    delay(20);

    if (err != SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Publish failed");
        log_to_sdcardf("pub failed, err: %d", err);
        return false;
    }

    log_to_sdcard("waiting for pub urc");
    delay(20);
    mqttGotURC = false;
    mqttPublishOk = false;
    int retries = 30;
    while (!mqttGotURC && retries > 0) {
        delay(1000);
        r5.bufferedPoll();
        retries--;
    }

    log_to_sdcardf("mqtt pub result got URC = %d, lastCmd = %d, lastResult = %d, mqttLogoutOk = %d", mqttGotURC, lastCmd, lastResult, mqttLogoutOk);
    return mqttPublishOk;
}
