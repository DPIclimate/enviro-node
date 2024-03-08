#include "str_utils.h"

#include <gtest/gtest.h>

using namespace wombat;

char str[10];

TEST(str_utils, stripLeadingWS) {
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

TEST(str_utils, stripTrailingWS) {
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

TEST(str_utils, stripWS) {
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

    strncpy(str, "  a b c     ", sizeof(str));
    EXPECT_EQ(stripWS(str), 5);
    EXPECT_STREQ(str, "a b c");

    strncpy(str, "\ta\t ", sizeof(str));
    EXPECT_EQ(stripWS(str), 1);
    EXPECT_STREQ(str, "a");
}

TEST(str_utils, get_cp_destination_no_dest) {
    int dest;
    const char *out_filename;
    size_t filename_len;

    EXPECT_EQ(get_cp_destination((const char *)nullptr, 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("a", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination(" ", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("a:b", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination(":b", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);
}

TEST(str_utils, get_cp_destination_r5) {
    int dest;
    const char *out_filename;
    size_t filename_len;

    EXPECT_EQ(get_cp_destination("r5:", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("r5:x", 4, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 1);
    EXPECT_EQ(*out_filename, 'x');
    EXPECT_EQ(filename_len, 1);

    EXPECT_EQ(get_cp_destination("r5:x y", 4, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 1);
    EXPECT_EQ(*out_filename, 'x');
    EXPECT_EQ(filename_len, 1);

    EXPECT_EQ(get_cp_destination("r5:x y", 6, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("r5:/x y", 7, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("r5:/ x y", 8, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("r5://x y", 8, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("r5:/msg.json    ", 12, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 1);
    EXPECT_EQ(strncmp(out_filename, "/msg.json", filename_len), 0);
    EXPECT_EQ(filename_len, 9);

    EXPECT_EQ(get_cp_destination("r5:/abcdef ", 10, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 1);
    EXPECT_EQ(strncmp(out_filename, "/abcdef", filename_len), 0);
    EXPECT_EQ(filename_len, 7);

    EXPECT_EQ(get_cp_destination("r5:/a-b_c.def", 13, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 1);
    EXPECT_EQ(strncmp(out_filename, "/a-b_c.def", filename_len), 0);
    EXPECT_EQ(filename_len, 10);
}

TEST(str_utils, get_cp_destination_sd) {
    int dest;
    const char *out_filename;
    size_t filename_len;

    EXPECT_EQ(get_cp_destination("sd:", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("sd:x", 4, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 2);
    EXPECT_EQ(*out_filename, 'x');
    EXPECT_EQ(filename_len, 1);

    EXPECT_EQ(get_cp_destination("sd:x y", 4, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 2);
    EXPECT_EQ(*out_filename, 'x');
    EXPECT_EQ(filename_len, 1);

    EXPECT_EQ(get_cp_destination("sd:x y", 6, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("sd:/x y", 7, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("sd:/ x y", 8, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("sd://x y", 8, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("sd:/abcdef ", 10, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 2);
    EXPECT_EQ(strncmp(out_filename, "/abcdef", filename_len), 0);
    EXPECT_EQ(filename_len, 7);
}

TEST(str_utils, get_cp_destination_spiffs) {
    int dest;
    const char *out_filename;
    size_t filename_len;

    EXPECT_EQ(get_cp_destination("spiffs:", 10, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("spiffs:x", 4, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("spiffs:x y", 4, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("spiffs:x", 8, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 3);
    EXPECT_EQ(*out_filename, 'x');
    EXPECT_EQ(filename_len, 1);

    EXPECT_EQ(get_cp_destination("spiffs:x y", 8, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 3);
    EXPECT_EQ(*out_filename, 'x');
    EXPECT_EQ(filename_len, 1);

    EXPECT_EQ(get_cp_destination("spiffs:x y", 6, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("spiffs:/x y", 9, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 3);
    EXPECT_EQ(strncmp(out_filename, "/x", filename_len), 0);
    EXPECT_EQ(filename_len, 2);

    EXPECT_EQ(get_cp_destination("spiffs:/ x y", 8, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("spiffs://x y", 8, dest, &out_filename, filename_len), false);
    EXPECT_EQ(dest, 0);

    EXPECT_EQ(get_cp_destination("spiffs:/abcdef ", 14, dest, &out_filename, filename_len), true);
    EXPECT_EQ(dest, 3);
    EXPECT_EQ(strncmp(out_filename, "/abcdef", filename_len), 0);
    EXPECT_EQ(filename_len, 7);
}

// TEST_F(...)

//#undef ARDUINO
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
