#pragma once

#include "Arduino.h"

struct Sfa30TestState {
    enum class Status : uint8_t {
        Absent = 0,
        Ok,
        Fault,
    };

    Status status = Status::Absent;
    bool data_valid = false;
    bool has_new_data = false;
    bool invalidate_called = false;
    bool probe_ok = false;
    bool probe_called = false;
    bool start_called = false;
    bool warmup_active = false;
    float hcho_ppb = 0.0f;
    uint32_t last_data_ms = 0;
};

class Sfa30 {
public:
    using Status = Sfa30TestState::Status;

    static Sfa30TestState &state() {
        static Sfa30TestState instance;
        return instance;
    }

    bool begin() { return true; }
    bool probe() {
        state().probe_called = true;
        return state().probe_ok;
    }
    void start() { state().start_called = true; }
    void stop() {}
    bool readData(float &) { return false; }
    void poll() {}
    bool isDataValid() const { return state().data_valid; }
    bool isOk() const { return state().status == Status::Ok; }
    bool isPresent() const { return state().status != Status::Absent; }
    bool hasFault() const { return state().status == Status::Fault; }
    Status status() const { return state().status; }
    bool isWarmupActive() const { return state().warmup_active; }
    const char *label() const { return "SFA30"; }
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
