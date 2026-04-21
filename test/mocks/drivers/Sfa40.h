#pragma once

#include "Arduino.h"

struct Sfa40TestState {
    enum class Status : uint8_t {
        Absent = 0,
        Ok,
        Fault,
    };

    Status status = Status::Ok;
    bool data_valid = false;
    bool has_new_data = false;
    bool invalidate_called = false;
    bool warmup_active = false;
    bool fallback_to_sfa30 = false;
    bool start_called = false;
    float hcho_ppb = 0.0f;
    uint32_t last_data_ms = 0;
};

class Sfa40 {
public:
    enum class SelfTestStatus : uint8_t {
        Idle = 0,
        Running,
        Passed,
        Failed,
        ReadError,
    };

    using Status = Sfa40TestState::Status;

    static Sfa40TestState &state() {
        static Sfa40TestState instance;
        return instance;
    }

    bool begin() { return true; }
    void start() { state().start_called = true; }
    void stop() {}
    bool readData(float &) { return false; }
    bool startSelfTest() { return false; }
    SelfTestStatus readSelfTestStatus(uint16_t &raw_result) {
        raw_result = 0;
        return SelfTestStatus::Idle;
    }
    void poll() {}
    bool isDataValid() const { return state().data_valid; }
    bool isOk() const { return state().status == Status::Ok; }
    bool isPresent() const { return state().status != Status::Absent; }
    bool hasFault() const { return state().status == Status::Fault; }
    bool isWarmupActive() const { return state().warmup_active; }
    Status status() const { return state().status; }
    bool shouldFallbackToSfa30() const { return state().fallback_to_sfa30; }
    const char *label() const { return "SFA40"; }
    uint32_t lastDataMs() const { return state().last_data_ms; }
    bool takeNewData(float &hcho_ppb) {
        if (!state().has_new_data) {
            return false;
        }
        hcho_ppb = state().hcho_ppb;
        state().has_new_data = false;
        state().data_valid = true;
        state().last_data_ms = millis();
        return true;
    }
    void invalidate() {
        state().data_valid = false;
        state().invalidate_called = true;
    }
};
