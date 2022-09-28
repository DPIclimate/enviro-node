#include <mbed.h>
#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void simple_test()
{
    TEST_ASSERT_EQUAL(33, 33);
}

void setup()
{
    delay(2000);
    printf("Hello");

    UNITY_BEGIN();
    RUN_TEST(simple_test);
    UNITY_END();
}

void loop()
{
    printf("Hello");
    delay(1000);
}