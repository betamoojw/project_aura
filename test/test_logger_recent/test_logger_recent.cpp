#include <unity.h>

#include "ArduinoMock.h"
#include "core/Logger.h"

void setUp() {
    setMillis(0);
    Logger::begin(Serial, Logger::Debug);
    Logger::setSerialOutputEnabled(false);
    Logger::setSensorsSerialOutputEnabled(true);
    Logger::resetRecentForTest();
}

void tearDown() {
    Logger::resetRecentForTest();
}

void test_alert_buffer_keeps_only_warn_and_error() {
    Logger::log(Logger::Info, "WiFi", "connected");
    advanceMillis(1);
    Logger::log(Logger::Warn, "MQTT", "publish delayed");
    advanceMillis(1);
    Logger::log(Logger::Error, "OTA", "write failed");

    Logger::RecentEntry recent[4];
    Logger::RecentEntry alerts[4];

    const size_t recent_count = Logger::copyRecent(recent, 4);
    const size_t alert_count = Logger::copyRecentAlerts(alerts, 4);

    TEST_ASSERT_EQUAL_UINT32(3, recent_count);
    TEST_ASSERT_EQUAL_UINT32(2, alert_count);

    TEST_ASSERT_EQUAL(Logger::Warn, alerts[0].level);
    TEST_ASSERT_EQUAL_STRING("MQTT", alerts[0].tag);
    TEST_ASSERT_EQUAL_STRING("publish delayed", alerts[0].message);

    TEST_ASSERT_EQUAL(Logger::Error, alerts[1].level);
    TEST_ASSERT_EQUAL_STRING("OTA", alerts[1].tag);
    TEST_ASSERT_EQUAL_STRING("write failed", alerts[1].message);
}

void test_alert_buffer_survives_info_churn() {
    Logger::log(Logger::Warn, "WiFi", "link unstable");
    for (unsigned i = 0; i < 80; ++i) {
        advanceMillis(1);
        Logger::log(Logger::Info, "Sensors", "sample %u", i);
    }

    Logger::RecentEntry alerts[4];
    const size_t alert_count = Logger::copyRecentAlerts(alerts, 4);

    TEST_ASSERT_EQUAL_UINT32(1, alert_count);
    TEST_ASSERT_EQUAL(Logger::Warn, alerts[0].level);
    TEST_ASSERT_EQUAL_STRING("WiFi", alerts[0].tag);
    TEST_ASSERT_EQUAL_STRING("link unstable", alerts[0].message);
}

void test_alert_buffer_excludes_soft_sensor_warnings() {
    Logger::log(Logger::Warn, "Sensors", "CO2 high: 1155 ppm");
    advanceMillis(1);
    Logger::log(Logger::Warn, "Sensors", "PM2.5 elevated: 19.7");
    advanceMillis(1);
    Logger::log(Logger::Error, "WiFi", "sta reconnect failed");

    Logger::RecentEntry alerts[4];
    const size_t alert_count = Logger::copyRecentAlerts(alerts, 4);

    TEST_ASSERT_EQUAL_UINT32(1, alert_count);
    TEST_ASSERT_EQUAL(Logger::Error, alerts[0].level);
    TEST_ASSERT_EQUAL_STRING("WiFi", alerts[0].tag);
    TEST_ASSERT_EQUAL_STRING("sta reconnect failed", alerts[0].message);
}

void test_alert_buffer_preserves_hard_errors_during_soft_sensor_warn_churn() {
    Logger::log(Logger::Error, "MQTT", "connect timeout");
    for (unsigned i = 0; i < 80; ++i) {
        advanceMillis(1);
        Logger::log(Logger::Warn, "Sensors", "CO2 high: 1203 ppm");
    }

    Logger::RecentEntry alerts[4];
    const size_t alert_count = Logger::copyRecentAlerts(alerts, 4);

    TEST_ASSERT_EQUAL_UINT32(1, alert_count);
    TEST_ASSERT_EQUAL(Logger::Error, alerts[0].level);
    TEST_ASSERT_EQUAL_STRING("MQTT", alerts[0].tag);
    TEST_ASSERT_EQUAL_STRING("connect timeout", alerts[0].message);
}

void test_alert_buffer_keeps_sen66_internal_faults() {
    Logger::log(Logger::Warn, "SEN66", "fan speed warning");
    advanceMillis(1);
    Logger::log(Logger::Error, "SEN66", "PM sensor error");

    Logger::RecentEntry alerts[4];
    const size_t alert_count = Logger::copyRecentAlerts(alerts, 4);

    TEST_ASSERT_EQUAL_UINT32(2, alert_count);
    TEST_ASSERT_EQUAL(Logger::Warn, alerts[0].level);
    TEST_ASSERT_EQUAL_STRING("SEN66", alerts[0].tag);
    TEST_ASSERT_EQUAL_STRING("fan speed warning", alerts[0].message);
    TEST_ASSERT_EQUAL(Logger::Error, alerts[1].level);
    TEST_ASSERT_EQUAL_STRING("SEN66", alerts[1].tag);
    TEST_ASSERT_EQUAL_STRING("PM sensor error", alerts[1].message);
}

void test_alert_buffer_excludes_optional_absence_but_keeps_faults() {
    Logger::log(Logger::Info, "Sensors", "SEN0466 CO not installed");
    advanceMillis(1);
    Logger::log(Logger::Info, "SEN0466", "not installed after 3 attempts, stop probing until reboot");
    advanceMillis(1);
    Logger::log(Logger::Info, "FanControl", "DAC not installed");
    advanceMillis(1);
    Logger::log(Logger::Warn, "FanControl", "DAC init failed: range write failed");

    Logger::RecentEntry alerts[4];
    const size_t alert_count = Logger::copyRecentAlerts(alerts, 4);

    TEST_ASSERT_EQUAL_UINT32(1, alert_count);
    TEST_ASSERT_EQUAL(Logger::Warn, alerts[0].level);
    TEST_ASSERT_EQUAL_STRING("FanControl", alerts[0].tag);
    TEST_ASSERT_EQUAL_STRING("DAC init failed: range write failed", alerts[0].message);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_alert_buffer_keeps_only_warn_and_error);
    RUN_TEST(test_alert_buffer_survives_info_churn);
    RUN_TEST(test_alert_buffer_excludes_soft_sensor_warnings);
    RUN_TEST(test_alert_buffer_preserves_hard_errors_during_soft_sensor_warn_churn);
    RUN_TEST(test_alert_buffer_keeps_sen66_internal_faults);
    RUN_TEST(test_alert_buffer_excludes_optional_absence_but_keeps_faults);
    return UNITY_END();
}
