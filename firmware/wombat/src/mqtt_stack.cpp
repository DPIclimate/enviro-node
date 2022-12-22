#include "CAT_M1.h"
#include "Utils.h"
#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"

#define TAG "mqtt_stack"

#define MAX_RSP 64
char rsp[MAX_RSP + 1];

#define MAX_BUF 2048
char buf[MAX_BUF + 1];

//void registrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}

//void epsRegistrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}


volatile bool mqttGotURC = false;
volatile bool mqttLoginOk = false;
volatile bool mqttLogoutOk = false;
volatile bool mqttPublishOk = false;

void mqttCmdCallback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
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

    r5.setMQTTCommandCallback(mqttCmdCallback);

    if (r5.connectMQTT() != SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        return false;
    }
    delay(20);

    // Wait for UMQTTC URC to show connection success/failure.
    while ( ! mqttGotURC) {
        r5.poll();
    }
    mqttGotURC = false;

    if ( ! mqttLoginOk) {
        ESP_LOGE(TAG, "Connection or mqtt_login to MQTT broker failed");
        return false;
    }

    ESP_LOGI(TAG, "Connected to MQTT");

    return true;
}

bool mqtt_logout(void) {
    ESP_LOGI(TAG, "logout");
    r5.disconnectMQTT();
    delay(20);

    while (!mqttGotURC) {
        r5.poll();
    }
    mqttGotURC = false;
    return mqttLogoutOk;
}

bool mqtt_publish(String& topic, const char * const msg) {
    ESP_LOGI(TAG, "Publish message: %s/%s", topic.c_str(), msg);
    SARA_R5_error_t err = r5.publishMQTT(topic, msg);
    if (err != SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Publish failed");
        return false;
    }

    delay(20);
    while (!mqttGotURC) {
        r5.poll();
    }
    mqttGotURC = false;
    return mqttPublishOk;
}
