#include "node_tests.h"


Node_UnitTests::Node_UnitTests(){

    // Flying probe, 12 V and 5 V power
    //test_12V_and_5V();

    //// Power monitoring
    //test_solar_monitoring();
    //test_battery_monitoring();

    // SD-Card connect and list directory
    //test_sdcard();

    // Startup and send LoRaWAN
    test_lorawan();

}

Node_UnitTests::~Node_UnitTests() = default;

__attribute__((unused)) void Node_UnitTests::test_12V_and_5V(){
    log_msg("[TEST]: Starting 12V test");

    digitalWrite(POWER_12V_EN, HIGH);

    log_msg("[TEST]: 12V and 5V output: ENABLED.");

    for(uint8_t i = 15; i > 0; i--){
        log_msg("\tProbe output: %d sec remaining...", i);
        delay(1000);
    }

    digitalWrite(POWER_12V_EN, LOW);

    log_msg("[TEST]: 12V and 5V output: DISABLED.");
}


__attribute__((unused)) void Node_UnitTests::test_sdi12(){
    log_msg("[TEST]: Starting SDI-12 test");

    digitalWrite(POWER_12V_EN, HIGH);

}

__attribute__((unused)) void Node_UnitTests::test_solar_monitoring() {
    log_msg("[TEST]: Starting solar monitoring on node.");

    Node_PowerMonitoring pwr;
    pwr.getSolarCurrent();
    pwr.getSolarVoltage();

    log_msg("[TEST]: Solar monitoring on node complete.");

}

__attribute__((unused)) void Node_UnitTests::test_battery_monitoring() {
    log_msg("[TEST]: Battery monitoring on node.");

    Node_PowerMonitoring pwr;
    pwr.getBatteryVoltage();
    pwr.getBatteryCurrent();

    log_msg("[TEST]: Battery monitoring on node complete.");

}

__attribute__((unused)) void Node_UnitTests::test_sdcard() {

    const char* filepath = "test.txt";

    // Add a file
    log_msg("[TEST]: SD-Card initialise and add file.");
    Node_SDCard sdcard;
    sdcard.add_file(filepath, "This is a test file.\n"); /* NOLINT */
    log_msg("[TEST]: SD-Card add file test complete.");

    // List the directory structure
    log_msg("[TEST]: Reading directory structure from SD-Card.");
    File root = SD.open("/");
    sdcard.list_directory(root, 0);
    root.close();
    log_msg("[TEST]: SD-Card read directory test complete.");

    // Read a file
    log_msg("[TEST]: Reading file '%s' from SD-Card.", filepath);
    sdcard.read_file(filepath); /* NOLINT */
    log_msg("[TEST]: Reading file '%s' from SD-Card test complete.", filepath);
}

__attribute__((unused)) void Node_UnitTests::test_lorawan() {

    log_msg("[TEST]: Initialising LoRaWAN test.");
    Node_LoRaWAN lora;
    log_msg("[TEST]: Initialising LoRaWAN test complete.");

    log_msg("[TEST]: Testing LoRaWAN send.");
    int8_t payload[] = {1, 2, 3, 4};
    Node_LoRaWAN::do_send(payload, sizeof(payload), 0);

    while(!Node_LoRaWAN::state()){
        os_runloop_once();
    }

    log_msg("[TEST]: Testing LoRaWAN send complete.");

}
