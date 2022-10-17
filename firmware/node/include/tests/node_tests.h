#ifndef NODE_TESTS_H
#define NODE_TESTS_H

#include "power_monitoring/node_power_monitoring.h"
#include "sdcard/node_sdcard.h"
#include "node_config.h"
#include "lorawan/node_lorawan.h"
#include "bluetooth/node_bluetooth.h"

class Node_UnitTests {
private:

    __attribute__((unused)) static void test_12V_and_5V();

    __attribute__((unused)) static void test_sdi12();

    __attribute__((unused)) static void test_solar_monitoring();

    __attribute__((unused)) static void test_battery_monitoring();

    __attribute__((unused)) static void test_sdcard();

    __attribute__((unused)) static void test_lorawan();

    __attribute__((unused)) static void test_bluetooth();

    //void test_onewire();
    //void test_sdcard();


public:
    Node_UnitTests();
    ~Node_UnitTests();

};

#endif //NODE_TESTS_H
