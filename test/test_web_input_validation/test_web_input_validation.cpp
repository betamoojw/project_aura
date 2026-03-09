#include <unity.h>

#include "web/WebInputValidation.h"

void setUp() {}
void tearDown() {}

void test_wifi_ssid_validation_accepts_spaces_and_utf8() {
    String ssid = String("  Net ") + String("\xC3\xA4") + "  ";
    TEST_ASSERT_TRUE(WebInputValidation::isWifiSsidValid(ssid));
}

void test_wifi_ssid_validation_rejects_empty_and_too_long() {
    TEST_ASSERT_FALSE(WebInputValidation::isWifiSsidValid(String()));

    String long_ssid;
    for (size_t i = 0; i < WebInputValidation::kWifiSsidMaxBytes + 1; ++i) {
        long_ssid += 'a';
    }
    TEST_ASSERT_FALSE(WebInputValidation::isWifiSsidValid(long_ssid));
}

void test_port_parse_uses_default_for_empty_input() {
    uint16_t port = 0;
    TEST_ASSERT_TRUE(WebInputValidation::parsePortOrDefault(String("   "), 1883, port));
    TEST_ASSERT_EQUAL_UINT16(1883, port);
}

void test_port_parse_accepts_valid_numeric_value() {
    uint16_t port = 0;
    TEST_ASSERT_TRUE(WebInputValidation::parsePortOrDefault(String("1884"), 1883, port));
    TEST_ASSERT_EQUAL_UINT16(1884, port);
}

void test_port_parse_rejects_invalid_values() {
    uint16_t port = 0;
    TEST_ASSERT_FALSE(WebInputValidation::parsePortOrDefault(String("70000"), 1883, port));
    TEST_ASSERT_FALSE(WebInputValidation::parsePortOrDefault(String("12abc"), 1883, port));
    TEST_ASSERT_FALSE(WebInputValidation::parsePortOrDefault(String("0"), 1883, port));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_wifi_ssid_validation_accepts_spaces_and_utf8);
    RUN_TEST(test_wifi_ssid_validation_rejects_empty_and_too_long);
    RUN_TEST(test_port_parse_uses_default_for_empty_input);
    RUN_TEST(test_port_parse_accepts_valid_numeric_value);
    RUN_TEST(test_port_parse_rejects_invalid_values);
    return UNITY_END();
}
