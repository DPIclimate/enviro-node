#include "CAT_M1.h"
#include "Utils.h"
#include "DeviceConfig.h"

#define TAG "mqtt_stack"

#define MAX_RSP 64
char rsp[MAX_RSP + 1];

#define MAX_BUF 2048
char buf[MAX_BUF + 1];

void waitForResponse(void) {
    int c = -1;
    while (c < 0) {
        c = waitForChar(LTE_Serial, 1000);
    }
}

size_t getResponse(void) {
    size_t len = 0;
    rsp[0] = 0;

    waitForResponse();
    if (LTE_Serial.available()) {
        len = LTE_Serial.read(rsp, MAX_RSP);
        rsp[len] = 0;
//        for (size_t i = 0; i < len; i++) {
//            Serial.printf("%02X ", rsp[i]);
//        }
//        Serial.println();

        len = stripWS(rsp);
    }

    return len;
}

bool c1Ready(void) {
    ESP_LOGI(TAG, "c1Ready");

    LTE_Serial.print("AT\r");
    delay(10);
    size_t len = getResponse();
    if (len > 0) {
        ESP_LOGI(TAG, "[%s]", rsp);
    }

    return len > 0;
}

/*
AT+UMQTT=2,"griffin-water.bnr.la",2048
+UMQTT: 2,1

OK
AT+UMQTT=4,"ublox","test"
+UMQTT: 4,1

OK

 */
bool mqInit(void) {
    LTE_Serial.print("ATE0\r");
    size_t len = getResponse();
    ESP_LOGI(TAG, "[%s] [%u]", rsp, len);
//    if (len != 2 || strncmp("OK", rsp, 2)) {
//        return false;
//    }

    DeviceConfig& config = DeviceConfig::get();

    snprintf(rsp, MAX_RSP, "AT+UMQTT=2,\"%s\",%u\r", config.getMqttHost().c_str(), config.getMqttPort());
    ESP_LOGI(TAG, "> [%s]", rsp);
    LTE_Serial.print(rsp);
    len = getResponse();
    ESP_LOGI(TAG, "< [%s] [%u]", rsp, len);
//    if (len != 2 || strncmp("OK", rsp, 2)) {
//        return false;
//    }

    snprintf(rsp, MAX_RSP, "AT+UMQTT=4,\"%s\",\"%s\"\r", config.getMqttUser().c_str(), config.getMqttPassword().c_str());
    ESP_LOGI(TAG, "> [%s]", rsp);
    LTE_Serial.print(rsp);
    len = getResponse();
    ESP_LOGI(TAG, "< [%s] [%u]", rsp, len);

    return true;
}

bool mqConnect(void) {
    LTE_Serial.print("AT+UMQTTC=1\r");
    size_t len = getResponse();
    ESP_LOGI(TAG, "< [%s] [%u]", rsp, len);
    return true;
}

void mqDisconnect(void) {
    LTE_Serial.print("AT+UMQTTC=0\r");
    size_t len = getResponse();
    ESP_LOGI(TAG, "< [%s] [%u]", rsp, len);
}

int mqPublish(const std::string topic, const std::string msg) {
    size_t len = snprintf(buf, MAX_BUF, "AT+UMQTTC=2,0,0,0,\"%s\",\"%s\"\r", topic.c_str(), msg.c_str());
    buf[len] = 0;
    ESP_LOGI(TAG, "> [%s]", buf);
    LTE_Serial.print(buf);
    len = getResponse();
    ESP_LOGI(TAG, "< [%s] [%u]", rsp, len);

    return 0;
}

bool c1SetSystemTimeFromModem(void) {
    size_t len = snprintf(buf, MAX_BUF, "AT+CCLK?\r");
    buf[len] = 0;
    ESP_LOGI(TAG, "> [%s]", buf);
    LTE_Serial.print(buf);
    len = getResponse();
    ESP_LOGI(TAG, "< [%s] [%u]", rsp, len);
    char *c = rsp;
    while (*c != '"' && c < rsp+len) {
        c++;
    }

    if (*c != '"') {
        return false;
    }

    // TODO: Very sketchy, add some sanity checks.

    // +CCLK: "22/12/06,04:58:09"
    c++;
    *(c+2) = 0;
    int yy = atoi(c) * 100;

    c += 3;
    *(c+2) = 0;
    int mm = atoi(c);

    c += 3;
    *(c+2) = 0;
    int dd = atoi(c);

    c += 3;
    *(c+2) = 0;
    int hh = atoi(c);

    c += 3;
    *(c+2) = 0;
    int mins = atoi(c);

    c += 3;
    *(c+2) = 0;
    int ss = atoi(c);

    ESP_LOGI(TAG, "Time is %d/%02d/%02dT%02d:%02d:%02dZ", yy, mm, dd, hh, mins, ss);
    return true;
}
