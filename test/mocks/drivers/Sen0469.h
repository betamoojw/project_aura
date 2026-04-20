#pragma once

#include "Arduino.h"

struct Sen0469TestState {
    bool present = false;
    bool start_ok = false;
    bool start_called = false;
    bool data_valid = false;
    bool warmup = false;
    bool invalidate_called = false;
    float nh3_ppm = 0.0f;
    uint32_t last_data_ms = 0;
};

class Sen0469 {
public:
    static Sen0469TestState &state() {
        static Sen0469TestState instance;
        return instance;
    }

    bool begin() { return true; }
    bool start() {
        state().start_called = true;
        state().present = state().start_ok;
        if (!state().present) {
            state().data_valid = false;
            state().nh3_ppm = 0.0f;
            state().warmup = false;
        }
        return state().start_ok;
    }
    void poll() {}
    bool isPresent() const { return state().present; }
    bool isDataValid() const { return state().data_valid; }
    bool isWarmupActive() const { return state().warmup; }
    float nh3Ppm() const { return state().nh3_ppm; }
    const char *label() const { return "SEN0469 NH3"; }
    uint8_t address() const { return 0x75; }
    uint32_t lastDataMs() const { return state().last_data_ms; }
    void invalidate() {
        state().invalidate_called = true;
        state().data_valid = false;
    }
};
