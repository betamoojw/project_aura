#include <unity.h>

#include "ArduinoMock.h"
#include "I2cMock.h"
#include "config/AppConfig.h"
#include "core/I2CHelper.h"
#include "core/Logger.h"
#include "drivers/Sfa40.h"
#include "esp_system.h"

esp_reset_reason_t boot_reset_reason = ESP_RST_POWERON;

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

void test_real_sfa40_start_keeps_absent_when_device_does_not_ack() {
    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_FALSE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::Status::Absent),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa40_start_marks_fault_when_present_but_start_fails() {
    uint8_t serial_data[9];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_START, true);

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::Status::Fault),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa40_warm_restart_stop_failure_marks_fault_when_device_acks() {
    uint8_t serial_data[9];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_STOP, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    boot_reset_reason = ESP_RST_SW;

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_FALSE(sfa.isWarmupActive());
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::Status::Fault),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa40_can_restart_after_runtime_stop_failure_once_bus_recovers() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(123, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0000, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.isOk());

    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_STOP, true);
    sfa.stop();

    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_STOP, false);
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());

    const uint32_t ready_ms =
        getMillis() + Config::SFA40_FIRST_READ_DELAY_MS - Config::SFA3X_START_DELAY_MS;
    setMillis(ready_ms);
    sfa.poll();

    float hcho_ppb = 0.0f;
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.3f, hcho_ppb);
}

void test_real_sfa40_detects_by_valid_id_and_reads_hcho() {
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

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_EQUAL_STRING("SFA40", sfa.label());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    setMillis(Config::SFA40_FIRST_READ_DELAY_MS);
    sfa.poll();

    float hcho_ppb = 0.0f;
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 43.2f, hcho_ppb);
    TEST_ASSERT_FALSE(sfa.isWarmupActive());
}

void test_real_sfa40_waits_minimum_startup_delay_before_first_read() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(123, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0000, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    setMillis(100000);
    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    sfa.poll();
    float hcho_ppb = 0.0f;
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));

    setMillis(100000 + Config::SFA40_FIRST_READ_DELAY_MS - 1);
    sfa.poll();
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));

    setMillis(100000 + Config::SFA40_FIRST_READ_DELAY_MS);
    sfa.poll();
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.3f, hcho_ppb);
}

void test_real_sfa40_waits_startup_delay_across_millis_wraparound() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(123, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0000, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    const uint32_t start_ms = 0xFFFFFF00UL;
    const uint32_t before_ready_ms = start_ms + Config::SFA40_FIRST_READ_DELAY_MS - 1U;
    const uint32_t ready_ms = start_ms + Config::SFA40_FIRST_READ_DELAY_MS;

    setMillis(start_ms);
    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    float hcho_ppb = 0.0f;
    sfa.poll();
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));

    setMillis(before_ready_ms);
    sfa.poll();
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));

    setMillis(ready_ms);
    sfa.poll();
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.3f, hcho_ppb);
}

void test_real_sfa40_marks_fault_when_id_read_fails() {
    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, true);

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_FALSE(sfa.isWarmupActive());
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::Status::Fault),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa40_marks_fault_when_id_returns_zero_serial() {
    uint8_t serial_data[9];

    encodeWordWithCrc(0x0000, &serial_data[0]);
    encodeWordWithCrc(0x0000, &serial_data[3]);
    encodeWordWithCrc(0x0000, &serial_data[6]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_ID,
                            serial_data,
                            sizeof(serial_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_FALSE(sfa.isWarmupActive());
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::Status::Fault),
                      static_cast<int>(sfa.status()));
}

void test_real_sfa40_keeps_starting_state_while_status_not_ready() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(0, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0300, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    setMillis(Config::SFA40_FIRST_READ_DELAY_MS);
    sfa.poll();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    float hcho_ppb = 0.0f;
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));
}

void test_real_sfa40_returns_data_but_keeps_warmup_until_within_spec() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(321, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0200, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    setMillis(Config::SFA40_FIRST_READ_DELAY_MS);
    sfa.poll();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    float hcho_ppb = 0.0f;
    TEST_ASSERT_TRUE(sfa.takeNewData(hcho_ppb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 32.1f, hcho_ppb);
}

void test_real_sfa40_marks_fault_for_undocumented_status_10_combination() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(0, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0100, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    setMillis(Config::SFA40_FIRST_READ_DELAY_MS);
    sfa.poll();

    TEST_ASSERT_TRUE(sfa.isPresent());
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_FALSE(sfa.isWarmupActive());

    float hcho_ppb = 0.0f;
    TEST_ASSERT_FALSE(sfa.takeNewData(hcho_ppb));
}

void test_real_sfa40_read_failure_does_not_clear_known_warmup_state() {
    uint8_t serial_data[9];
    uint8_t read_data[12];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    encodeWordWithCrc(321, &read_data[0]);
    encodeWordWithCrc(0x8000, &read_data[3]);
    encodeWordWithCrc(0x6666, &read_data[6]);
    encodeWordWithCrc(0x0200, &read_data[9]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_VALUES,
                            read_data,
                            sizeof(read_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();

    setMillis(Config::SFA40_FIRST_READ_DELAY_MS);
    sfa.poll();
    TEST_ASSERT_TRUE(sfa.isWarmupActive());

    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_READ_VALUES, true);
    setMillis(Config::SFA40_FIRST_READ_DELAY_MS + Config::SFA40_POLL_MS);
    sfa.poll();

    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.isWarmupActive());
}

void test_real_sfa40_selftest_reports_running_status() {
    uint8_t serial_data[9];
    uint8_t selftest_data[3];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);
    encodeWordWithCrc(Config::SFA40_SELFTEST_RUNNING_RAW, &selftest_data[0]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_SELFTEST,
                            selftest_data,
                            sizeof(selftest_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.startSelfTest());

    uint16_t raw_result = 0;
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::SelfTestStatus::Running),
                      static_cast<int>(sfa.readSelfTestStatus(raw_result)));
    TEST_ASSERT_EQUAL_HEX16(Config::SFA40_SELFTEST_RUNNING_RAW, raw_result);
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
}

void test_real_sfa40_selftest_reports_passed_status() {
    uint8_t serial_data[9];
    uint8_t selftest_data[3];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);
    encodeWordWithCrc(0x0000, &selftest_data[0]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_SELFTEST,
                            selftest_data,
                            sizeof(selftest_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.startSelfTest());

    uint16_t raw_result = 0xFFFF;
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::SelfTestStatus::Passed),
                      static_cast<int>(sfa.readSelfTestStatus(raw_result)));
    TEST_ASSERT_EQUAL_HEX16(0x0000, raw_result);
    TEST_ASSERT_TRUE(sfa.isOk());
    TEST_ASSERT_FALSE(sfa.hasFault());
}

void test_real_sfa40_selftest_reports_failed_status_and_marks_fault() {
    uint8_t serial_data[9];
    uint8_t selftest_data[3];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);
    encodeWordWithCrc(0x0042, &selftest_data[0]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandRead(Config::SFA3X_ADDR,
                            Config::SFA40_CMD_READ_SELFTEST,
                            selftest_data,
                            sizeof(selftest_data));

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.startSelfTest());

    uint16_t raw_result = 0;
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::SelfTestStatus::Failed),
                      static_cast<int>(sfa.readSelfTestStatus(raw_result)));
    TEST_ASSERT_EQUAL_HEX16(0x0042, raw_result);
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
}

void test_real_sfa40_selftest_read_error_marks_fault_and_clears_active_state() {
    uint8_t serial_data[9];

    encodeWordWithCrc(0x1234, &serial_data[0]);
    encodeWordWithCrc(0x5678, &serial_data[3]);
    encodeWordWithCrc(0x9ABC, &serial_data[6]);

    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandRead(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, serial_data, sizeof(serial_data));
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_READ_SELFTEST, true);

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.startSelfTest());

    uint16_t raw_result = 0xABCD;
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::SelfTestStatus::ReadError),
                      static_cast<int>(sfa.readSelfTestStatus(raw_result)));
    TEST_ASSERT_EQUAL_HEX16(0x0000, raw_result);
    TEST_ASSERT_FALSE(sfa.isOk());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::Status::Fault),
                      static_cast<int>(sfa.status()));

    TEST_ASSERT_EQUAL(static_cast<int>(Sfa40::SelfTestStatus::Idle),
                      static_cast<int>(sfa.readSelfTestStatus(raw_result)));
}

void test_real_sfa40_selftest_is_rejected_when_driver_is_in_fault_state() {
    I2cMock::setDevicePresent(Config::SFA3X_ADDR, true);
    I2cMock::setCommandFailure(Config::SFA3X_ADDR, Config::SFA40_CMD_ID, true);

    Sfa40 sfa;

    TEST_ASSERT_TRUE(sfa.begin());
    sfa.start();
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_FALSE(sfa.isOk());

    TEST_ASSERT_FALSE(sfa.startSelfTest());
    TEST_ASSERT_TRUE(sfa.hasFault());
    TEST_ASSERT_FALSE(sfa.isOk());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_real_sfa40_start_keeps_absent_when_device_does_not_ack);
    RUN_TEST(test_real_sfa40_start_marks_fault_when_present_but_start_fails);
    RUN_TEST(test_real_sfa40_warm_restart_stop_failure_marks_fault_when_device_acks);
    RUN_TEST(test_real_sfa40_can_restart_after_runtime_stop_failure_once_bus_recovers);
    RUN_TEST(test_real_sfa40_detects_by_valid_id_and_reads_hcho);
    RUN_TEST(test_real_sfa40_waits_minimum_startup_delay_before_first_read);
    RUN_TEST(test_real_sfa40_waits_startup_delay_across_millis_wraparound);
    RUN_TEST(test_real_sfa40_marks_fault_when_id_read_fails);
    RUN_TEST(test_real_sfa40_marks_fault_when_id_returns_zero_serial);
    RUN_TEST(test_real_sfa40_keeps_starting_state_while_status_not_ready);
    RUN_TEST(test_real_sfa40_returns_data_but_keeps_warmup_until_within_spec);
    RUN_TEST(test_real_sfa40_marks_fault_for_undocumented_status_10_combination);
    RUN_TEST(test_real_sfa40_read_failure_does_not_clear_known_warmup_state);
    RUN_TEST(test_real_sfa40_selftest_reports_running_status);
    RUN_TEST(test_real_sfa40_selftest_reports_passed_status);
    RUN_TEST(test_real_sfa40_selftest_reports_failed_status_and_marks_fault);
    RUN_TEST(test_real_sfa40_selftest_read_error_marks_fault_and_clears_active_state);
    RUN_TEST(test_real_sfa40_selftest_is_rejected_when_driver_is_in_fault_state);
    return UNITY_END();
}
