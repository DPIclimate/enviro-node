#include <Arduino.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <TCA9534.h>
#include "SensorTask.h"
//#include <CAT_M1.h>
#include "DeviceConfig.h"
#include "cli/CLI.h"
#include "peripherals.h"
#include "mqtt_stack.h"

#include <SparkFun_u-blox_SARA-R5_Arduino_Library.h>

#include "audio-feedback/tones.h"

#define TAG "wombat"

TCA9534 io_expander;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;

SARA_R5 r5(0x83, 0x80);

//void registrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}

volatile bool mqttGotURC = false;
volatile bool mqttLoginOk = false;
volatile bool mqttLogoutOk = false;

void mqttCmdCallback(int cmd, int result) {
    ESP_LOGI(TAG, "cmd: %d, result: %d", cmd, result);
//    if (result == 0) {
//        int e1, e2;
//        r5.getMQTTprotocolError(&e1, &e2);
//        ESP_LOGE(TAG, "MQTT op failed: %d, %d", e1, e2);
//    }

    if (cmd == 0 && result == 1) {
        mqttLogoutOk = true;
    }
    if (cmd == 1 && result == 1) {
        mqttLoginOk = true;
    }

    mqttGotURC = true;
}

//void epsRegistrationCallback(SARA_R5_registration_status_t status, unsigned int lac, unsigned int ci, int Act) {
//    ESP_LOGI(TAG, "%d, %u, %u, %d", status, lac, ci, Act);
//}

void setup() {
    pinMode(PROG_BTN, INPUT);

    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    // Not working.
    esp_log_level_set("sensors", ESP_LOG_WARN);
    esp_log_level_set("DPI12", ESP_LOG_WARN);

    // Try to avoid it getting optimized out.
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    // ==== CAT-M1 Setup START ====
    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    // Set all pins to output mode (reg 3, value 0).
    io_expander.config(TCA9534::Config::OUT);

    cat_m1.begin(io_expander);
    LTE_Serial.begin(115200);
    while(!LTE_Serial) {
        delay(1);
    }
    // ==== CAT-M1 Setup END ====

    // This must be done before the config is loaded because the config file is
    // a list of commands.
    CLI::init();

    DeviceConfig& config = DeviceConfig::get();
    config.load();
    config.dumpConfig(Serial);

    //BluetoothServer::begin();

    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Programmable button pressed while booting, dropping into REPL");
        CLI::repl(Serial);
        ESP_LOGI(TAG, "Continuing");
    }

    uint16_t mi = config.getMeasureInterval();
    uint16_t ui = config.getUplinkInterval();

    if (ui < mi) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be <= uplink interval "
                      "(%u). Resetting config to default values.", mi, ui);
        config.reset();
    }

    if (ui % mi != 0) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be a factor of uplink "
                      "interval (%u). Resetting config to default "
                      "values.", mi, ui);
        config.reset();
    }

    // Assuming the measure interval is a factor of the uplink interval,
    // uimi is the number of boots between uplinks.
    uint16_t uimi = ui / mi;

    // If the current boot cycle is a multiple of uimi, then this boot cycle
    // should do an uplink.
    bool uplinkCycle = config.getBootCount() % uimi == 0;

    ESP_LOGI(TAG, "Boot count: %lu, measurement interval: %u, "
                  "uplink interval: %u, ui/mi: %u, uplink this cycle: %d",
                  config.getBootCount(), mi, ui, uimi, uplinkCycle);

    // Not sure how useful this is, or how it will work. We don't want to
    // interrupt sensor reading or uplink processing and loop() will likely
    // never run if we do the usual ESP32 setup going to deep sleep mode.
    // It is useful while developing because the node isn't going to sleep.
    attachInterrupt(PROG_BTN, progBtnISR, RISING);

    //initSensors();


    //while ( ! c1Ready());
    //c1SetSystemTimeFromModem();


//
//    if (mqInit()) {
//        if (mqConnect()) {
//            ESP_LOGI(TAG, "Connected");
//            delay(2);
//            std::string s("wombat/");
//            s += config.node_id;
//            mqPublish(s, "ABCDEF");
//            mqDisconnect();
//        }
//    }
    //sensorTask();

    // Flush log messages before sleeping;

    r5.enableDebugging();
    r5.enableAtDebugging();

    //r5.setRegistrationCallback(registrationCallback);
    //r5.setEpsRegistrationCallback(epsRegistrationCallback);
    r5.setMQTTCommandCallback(mqttCmdCallback);

    if ( ! r5.begin(LTE_Serial, 115200)) {
        ESP_LOGE(TAG, "r5 begin failed");
        Serial.flush();
        while (true) {
            CLI::repl(Serial);
        }
    }

    // Only needs to be done one, but the Sparkfun library reads this value before setting so
    // it is quick enough to call this every time.
    if (!r5.setNetworkProfile(MNO_TELSTRA)) {
        Serial.println(F("Error setting network. Try cycling the power."));
        while (1) ;
    }

    // This issues the AT+COPS=0,0 command which starts the network registration process. This
    // takes over 10 seconds. We need to learn how to check if the radio is registered so we
    // can skip this step when possible.
    ESP_LOGI(TAG, "Calling auto op selection");
    r5.automaticOperatorSelection();
    ESP_LOGI(TAG, "Back from auto op selection");

    int regStatus = r5.registration();
    if ((regStatus >= 0) && (regStatus <= 10)) {
        // These commands come from the SARA R4/R5 Internet applications development guide
        // ss 2.3, table Profile Activation: SARA R5.
        r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_PROTOCOL, 0);
        r5.setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_MAP_TO_CID, 1);
        r5.performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE);

        r5.setMQTTserver("griffin-water.bnr.la", 2048);
        r5.setMQTTcredentials("ublox", "test");

        if (r5.connectMQTT() == SARA_R5_ERROR_SUCCESS) {
            while ( ! mqttGotURC) {
                r5.poll();
            }
            mqttGotURC = false;

            if (mqttLoginOk) {
                ESP_LOGI(TAG, "Connected to MQTT");
                delay(500);

                String topic("wombat");
                String msg("abcdefghijklmnopqrstuvwxyz");
                r5.publishMQTT(topic, msg);
                delay(500);
                while (!mqttGotURC) {
                    r5.poll();
                }
                mqttGotURC = false;

                r5.disconnectMQTT();
                while (!mqttGotURC) {
                    r5.poll();
                }
                mqttGotURC = false;
            }
        } else {
            int e1, e2;
            r5.getMQTTprotocolError(&e1, &e2);
            ESP_LOGE(TAG, "MQTT connection failed: %d, %d", e1, e2);
        }
    }

/*
AT+UMQTT=2,"griffin-water.bnr.la",2048
OK
sendCommandWithResponse: Command: +UMQTT=4,"ublox","test"
AT+UMQTT=4,"ublox","test"

OK
sendCommandWithResponse: Command: +UMQTTC=1
AT+UMQTTC=1

OK
sendCommandWithResponse: Command: +UMQTTER
AT+UMQTTER
*/
    String theTime = r5.clock();
    Serial.println(theTime);

    ESP_LOGI(TAG, "Time is now %s", iso8601());
    Serial.flush();
    CLI::repl(Serial);

//    unsigned long setupEnd = millis();
//    uint64_t sleepTime = (mi * 1000000) - (setupEnd * 1000);
//    esp_sleep_enable_timer_wakeup(sleepTime);
//    esp_deep_sleep_start();
}

void loop() {
    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Button");
        CLI::repl(Serial);
    }
}

#ifdef __cplusplus
extern "C" {
#endif
extern void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin & 0x80) {
        uint8_t p = pin & 0x7F;
        io_expander.output(p, val == HIGH ? TCA9534::H : TCA9534::L);
    } else {
        gpio_set_level((gpio_num_t) pin, val);
    }
}
#ifdef __cplusplus
}
#endif
