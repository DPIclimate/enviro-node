#include "CAT_M1.h"
#include "Utils.h"
#include "DeviceConfig.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"

#define TAG "mqtt_stack"

SARA_R5 r5(0x83, 0x80);

#define MAX_RSP 64
char rsp[MAX_RSP + 1];

#define MAX_BUF 2048
char buf[MAX_BUF + 1];

bool c1SetSystemTimeFromModem(void) {
/*
    struct tm t;
    memset(&t, 0, sizeof(t));

    // TODO: Very sketchy, add some sanity checks.
    // +CCLK: "22/12/06,04:58:09"
    c++;
    *(c+2) = 0;
    // tm_year is years since 1900, and it is now 2022. So 22 should be 122 years. The modem
    // defaults to year = 80 (presumably meaning 1980) but as that is in the past it may just
    // as well be interpreted as 180 years past 1900, ie 2080.
    t.tm_year = atoi(c);
    t.tm_year += 100;

    // tm_mon is 0 - 11.
    c += 3;
    *(c+2) = 0;
    t.tm_mon = atoi(c) - 1;

    c += 3;
    *(c+2) = 0;
    t.tm_mday = atoi(c);

    c += 3;
    *(c+2) = 0;
    t.tm_hour = atoi(c);

    c += 3;
    *(c+2) = 0;
    t.tm_min = atoi(c);

    c += 3;
    *(c+2) = 0;
    t.tm_sec = atoi(c);

    ESP_LOGI(TAG, "Time read from modem: %s", asctime(&t));

    time_t time = mktime(&t);
    struct timeval tv;
    struct timezone tz;

    tv.tv_sec = time;
    tv.tv_usec = 0;

    // The modem reports, and the node runs on, UTC.
    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;

    settimeofday(&tv, &tz);
*/
    return true;
}

//void registrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}

//void epsRegistrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}


volatile bool mqttGotURC = false;
volatile bool mqttLoginOk = false;
volatile bool mqttLogoutOk = false;

void mqttCmdCallback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
    if (result == 0) {
        int e1, e2;
        r5.getMQTTprotocolError(&e1, &e2);
        ESP_LOGE(TAG, "MQTT op failed: %d, %d", e1, e2);
    }

    if (cmd == 0 && result == 1) {
        mqttLogoutOk = true;
    }
    if (cmd == 1 && result == 1) {
        mqttLoginOk = true;
    }

    mqttGotURC = true;
}


bool mqtt_login(void) {
    ESP_LOGI(TAG, "Starting modem and login");

    //r5.enableDebugging();
    //r5.enableAtDebugging();

    //r5.setRegistrationCallback(registrationCallback);
    //r5.setEpsRegistrationCallback(epsRegistrationCallback);
    r5.setMQTTCommandCallback(mqttCmdCallback);

    if ( ! r5.begin(LTE_Serial, 115200)) {
        ESP_LOGE(TAG, "SARA-R5 begin failed");
        return false;
    }

    // Only needs to be done one, but the Sparkfun library reads this value before setting so
    // it is quick enough to call this every time.
    if ( ! r5.setNetworkProfile(MNO_TELSTRA)) {
        ESP_LOGE(TAG, "Error setting network operator profile");
        return false;
    }
    delay(20);

    String op_str;
    if (r5.getOperator(&op_str) != SARA_R5_error_t::SARA_R5_ERROR_SUCCESS) {
        ESP_LOGE(TAG, "Error getting network operator");
        return false;
    }

    ESP_LOGI(TAG, "AT+COPS? = %s", op_str.c_str());
    if (op_str.isEmpty()) {
        // This issues the AT+COPS=0,0 command which starts the network registration process. This
        // takes over 10 seconds. We need to learn how to check if the radio is registered so we
        // can skip this step when possible.
        ESP_LOGI(TAG, "Calling auto operator selection");
        r5.automaticOperatorSelection();
        ESP_LOGI(TAG, "Back from auto operator selection");
        delay(20);
    }

    int regStatus = r5.registration();
    ESP_LOGI(TAG, "");
    if ( ! ((regStatus >= 0) && (regStatus <= 10))) {
        ESP_LOGE(TAG, "Not registered to network");
        return false;
    }
    delay(20);

    // These commands come from the SARA R4/R5 Internet applications development guide
    // ss 2.3, table Profile Activation: SARA R5.
    r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_PROTOCOL, 0);
    delay(20);
    r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_MAP_TO_CID, 1);
    delay(20);
    r5.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE);
    delay(20);

    r5.setMQTTserver("griffin-water.bnr.la", 2048);
    delay(20);
    r5.setMQTTcredentials("ublox", "test");
    delay(20);

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

void mqtt_logout(void) {
    ESP_LOGI(TAG, "logout");
    r5.disconnectMQTT();
    delay(20);

    while (!mqttGotURC) {
        r5.poll();
    }
    mqttGotURC = false;
}

void mqtt_publish(String& topic, String& msg) {
    ESP_LOGI(TAG, "Publish message: %s/%s", topic.c_str(), msg.c_str());
    r5.publishMQTT(topic, msg);
    delay(20);
    while (!mqttGotURC) {
        r5.poll();
    }
    mqttGotURC = false;
}
