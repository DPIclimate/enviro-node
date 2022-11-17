#include <Utils.h>

#include <gtest/gtest.h>

char str[10];

TEST(Utils, stripLeadingWS) {
    EXPECT_EQ(stripLeadingWS(nullptr), 0);

    str[0] = 0;
    EXPECT_EQ(stripLeadingWS(str), 0);
    EXPECT_STREQ(str, "");

    strncpy(str, "a", sizeof(str));
    EXPECT_EQ(stripLeadingWS(str), 1);
    EXPECT_STREQ(str, "a");

    strncpy(str, "a ", sizeof(str));
    EXPECT_EQ(stripLeadingWS(str), 2);
    EXPECT_STREQ(str, "a ");

    strncpy(str, " a ", sizeof(str));
    EXPECT_EQ(stripLeadingWS(str), 2);
    EXPECT_STREQ(str, "a ");

    strncpy(str, " \ta", sizeof(str));
    EXPECT_EQ(stripLeadingWS(str), 1);
    EXPECT_STREQ(str, "a");
}

TEST(Utils, stripTrailingWS) {
    EXPECT_EQ(stripTrailingWS(nullptr), 0);

    str[0] = 0;
    EXPECT_EQ(stripTrailingWS(str), 0);
    EXPECT_STREQ(str, "");

    strncpy(str, "a", sizeof(str));
    EXPECT_EQ(stripTrailingWS(str), 1);
    EXPECT_STREQ(str, "a");

    strncpy(str, "a ", sizeof(str));
    EXPECT_EQ(stripTrailingWS(str), 1);
    EXPECT_STREQ(str, "a");

    strncpy(str, " a ", sizeof(str));
    EXPECT_EQ(stripTrailingWS(str), 2);
    EXPECT_STREQ(str, " a");

    strncpy(str, "a\t ", sizeof(str));
    EXPECT_EQ(stripTrailingWS(str), 1);
    EXPECT_STREQ(str, "a");
}

TEST(Utils, stripWS) {
    EXPECT_EQ(stripWS(nullptr), 0);

    str[0] = 0;
    EXPECT_EQ(stripWS(str), 0);
    EXPECT_STREQ(str, "");

    strncpy(str, "a", sizeof(str));
    EXPECT_EQ(stripWS(str), 1);
    EXPECT_STREQ(str, "a");

    strncpy(str, "a ", sizeof(str));
    EXPECT_EQ(stripWS(str), 1);
    EXPECT_STREQ(str, "a");

    strncpy(str, " a ", sizeof(str));
    EXPECT_EQ(stripWS(str), 1);
    EXPECT_STREQ(str, "a");

    strncpy(str, "\ta\t ", sizeof(str));
    EXPECT_EQ(stripWS(str), 1);
    EXPECT_STREQ(str, "a");
}

// TEST_F(...)

#undef ARDUINO
#if defined(ARDUINO)
#include <Arduino.h>

void setup()
{
    // should be the same value as for the `test_speed` option in "platformio.ini"
    // default value is test_speed=115200
    Serial.begin(115200);

    ::testing::InitGoogleTest();
    // if you plan to use GMock, replace the line above with
    // ::testing::InitGoogleMock();
}

void loop()
{
    // Run tests
    if (RUN_ALL_TESTS())
        ;

    // sleep for 1 sec
    delay(1000);
}

#else
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    // if you plan to use GMock, replace the line above with
    // ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS())
    ;

    // Always return zero-code and allow PlatformIO to parse results
    return 0;
}
#endif
