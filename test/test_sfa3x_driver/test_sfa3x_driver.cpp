#include <unity.h>

#include "ArduinoMock.h"
#include "I2cMock.h"
#include "config/AppConfig.h"
#include "core/Logger.h"
#include "esp_system.h"

esp_reset_reason_t boot_reset_reason = ESP_RST_POWERON;

#define Sfa3x RealSfa3x
#include "../../src/core/I2CHelper.cpp"
#include "../../src/drivers/Sfa3x.cpp"
#undef Sfa3x

void setUp() {
    setMillis(0);
    I2cMock::reset();
    Logger::begin(Serial, Logger::Debug);
    Logger::setSerialOutputEnabled(false);
    Logger::setSensorsSerialOutputEnabled(false);
    boot_reset_reason = ESP_RST_POWERON;
}

void tearDown() {}

void test_real_sfa3x_start_keeps_absent_when_device_does_not_ack() {
    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_FALSE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
    TEST_ASSERT_EQUAL(static_cast<int>(RealSfa3x::Status::Absent),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa3x_start_marks_fault_when_present_but_start_fails() {
    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA3X_CMD_START, true);

    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_EQUAL(static_cast<int>(RealSfa3x::Status::Fault),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa3x_warm_restart_stop_failure_marks_fault_when_device_acks() {
    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA3X_CMD_STOP, true);
    boot_reset_reason = ESP_RST_SW;

    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_EQUAL(static_cast<int>(RealSfa3x::Status::Fault),
                      static_cast<int>(sfa.status()));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_real_sfa3x_start_keeps_absent_when_device_does_not_ack);
    RUN_TEST(test_real_sfa3x_start_marks_fault_when_present_but_start_fails);
    RUN_TEST(test_real_sfa3x_warm_restart_stop_failure_marks_fault_when_device_acks);
    return UNITY_END();
}
