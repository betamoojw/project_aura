// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <Arduino.h>

class Sfa40 {
public:
    enum class Status : uint8_t {
        Absent = 0,
        Ok,
        Fault,
    };

    enum class SelfTestStatus : uint8_t {
        Idle = 0,
        Running,
        Passed,
        Failed,
        ReadError,
    };

    bool begin();
    void start();
    void stop();
    bool readData(float &hcho_ppb);
    bool startSelfTest();
    SelfTestStatus readSelfTestStatus(uint16_t &raw_result);
    void poll();
    bool isDataValid() const { return data_valid_; }
    bool isOk() const { return status_ == Status::Ok; }
    bool isPresent() const { return status_ != Status::Absent; }
    bool hasFault() const { return status_ == Status::Fault; }
    Status status() const { return status_; }
    bool isWarmupActive() const { return warmup_active_; }
    bool shouldFallbackToSfa30() const;
    const char *label() const { return "SFA40"; }
    uint32_t lastDataMs() const { return last_data_ms_; }
    bool takeNewData(float &hcho_ppb);
    void invalidate();

private:
    enum class ErrorCause : uint8_t {
        None = 0,
        DetectSensor,
        WarmRestartStop,
        StartCommand,
        ReadCommand,
        ReadBytes,
        ReadCrc,
        ReadStatus,
    };

    struct MeasurementReadResult {
        bool status_valid = false;
        bool warmup_active = false;
        float hcho_ppb = 0.0f;
    };

    bool readMeasurement(MeasurementReadResult &result);
    bool detectSensor();
    bool readWords(uint16_t cmd, uint16_t *out, size_t words, uint32_t delay_ms);
    bool ensureIdleBeforeDetect();
    bool ensureIdleBeforeStart();
    bool pingAddress();
    bool writeCmd(uint16_t cmd);
    bool readBytes(uint8_t *buf, size_t len);
    const char *errorCauseLabel() const;

    bool ok_ = false;
    bool measuring_ = false;
    bool measurement_state_unknown_ = false;
    bool data_valid_ = false;
    bool has_new_data_ = false;
    float last_hcho_ppb_ = 0.0f;
    uint32_t last_poll_ms_ = 0;
    uint32_t next_measurement_read_ms_ = 0;
    uint32_t last_data_ms_ = 0;
    uint8_t fail_count_ = 0;
    Status status_ = Status::Absent;
    ErrorCause last_error_cause_ = ErrorCause::None;
    bool warmup_active_ = false;
    bool selftest_active_ = false;
};
