#include <unity.h>

#include "ArduinoMock.h"
#include "I2cMock.h"
#include "core/Logger.h"
#include "esp_system.h"

esp_reset_reason_t boot_reset_reason = ESP_RST_POWERON;

#define private public
#define Sen66 RealSen66
#include "../../src/drivers/Sen66.h"
#undef private

#include "../../src/core/I2CHelper.cpp"
#include "../../src/drivers/Sen66.cpp"
#undef Sen66

void setUp() {
    setMillis(0);
    I2cMock::reset();
    Logger::begin(Serial, Logger::Debug);
    Logger::setSerialOutputEnabled(false);
    Logger::setSensorsSerialOutputEnabled(false);
    boot_reset_reason = ESP_RST_POWERON;
}

void tearDown() {}

void test_real_sen66_device_reset_clears_co2_smoother_ring() {
    I2cMock::setDevicePresent(Config::SEN66_ADDR, true);

    RealSen66 sen66;
    TEST_ASSERT_TRUE(sen66.begin());

    sen66.co2_first_ = false;
    sen66.co2_idx_ = 3;
    sen66.co2_readings_[0] = 1111;
    sen66.co2_readings_[1] = 1222;
    sen66.co2_readings_[2] = 1333;
    sen66.co2_readings_[3] = 1444;
    sen66.co2_readings_[4] = 1555;

    TEST_ASSERT_TRUE(sen66.deviceReset());
    TEST_ASSERT_TRUE(sen66.co2_first_);
    TEST_ASSERT_EQUAL(0, sen66.co2_idx_);
    for (int reading : sen66.co2_readings_) {
        TEST_ASSERT_EQUAL(400, reading);
    }
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_real_sen66_device_reset_clears_co2_smoother_ring);
    return UNITY_END();
}
