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

namespace {

void encodeWordWithCrc(uint16_t word, uint8_t *dst) {
    dst[0] = static_cast<uint8_t>(word >> 8);
    dst[1] = static_cast<uint8_t>(word & 0xFF);
    dst[2] = I2C::crc8(dst, 2);
}

} // namespace

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
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_STOP, true);
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

void test_real_sfa3x_detects_sfa40_by_valid_id_and_reads_hcho() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(432, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0000, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_EQUAL(static_cast<int>(RealSfa3x::Variant::Sfa40),
                      static_cast<int>(sfa.variant()));
    TEST_ASSERT_EQUAL_STRING("SFA40", sfa.label());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    setMillis(Config::SFA3X_POLL_MS);
    sfa.poll();

    float hcho_ppb = 0.0f;
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 43.2f, hcho_ppb);
    TEST_ASSERT_FALSE(sfa.isWarmupActive());
}

void test_real_sfa3x_falls_back_to_sfa30_when_sfa40_id_read_fails() {
    uint8_t read_data[9];

    encodeWordWithCrc(500, &read_data[0]);
    encodeWordWithCrc(0x0000, &read_data[3]);
    encodeWordWithCrc(0x0000, &read_data[6]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA3X_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_EQUAL(static_cast<int>(RealSfa3x::Variant::Sfa30),
                      static_cast<int>(sfa.variant()));
    TEST_ASSERT_EQUAL_STRING("SFA30", sfa.label());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.isWarmupActive());

    setMillis(Config::SFA3X_POLL_MS);
    sfa.poll();

    float hcho_ppb = 0.0f;
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, hcho_ppb);
}

void test_real_sfa3x_keeps_sfa40_in_starting_state_while_status_not_ready() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(0, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0001, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    setMillis(Config::SFA3X_POLL_MS);
    sfa.poll();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    float hcho_ppb = 0.0f;
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));
}

void test_real_sfa3x_clears_sfa40_warmup_after_timeout() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(0, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0001, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    RealSfa3x sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    setMillis(Config::SFA40_WARMUP_MS + Config::SFA3X_POLL_MS);
    sfa.poll();

    TEST_ASSERT_FALSE(sfa.isWarmupActive());
    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_real_sfa3x_start_keeps_absent_when_device_does_not_ack);
    RUN_TEST(test_real_sfa3x_start_marks_fault_when_present_but_start_fails);
    RUN_TEST(test_real_sfa3x_warm_restart_stop_failure_marks_fault_when_device_acks);
    RUN_TEST(test_real_sfa3x_detects_sfa40_by_valid_id_and_reads_hcho);
    RUN_TEST(test_real_sfa3x_falls_back_to_sfa30_when_sfa40_id_read_fails);
    RUN_TEST(test_real_sfa3x_keeps_sfa40_in_starting_state_while_status_not_ready);
    RUN_TEST(test_real_sfa3x_clears_sfa40_warmup_after_timeout);
    return UNITY_END();
}
