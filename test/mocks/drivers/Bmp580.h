#pragma once

#ifdef USE_REAL_BMP580_HEADER
#include "../../../src/drivers/Bmp580.h"
#else

#include "Arduino.h"

struct Bmp580TestState {
    bool ok = true;
    bool start_ok = true;
    bool pressure_valid = false;
    bool has_new_data = false;
    bool invalidate_called = false;
    float pressure = 0.0f;
    float temperature = 0.0f;
    uint32_t last_data_ms = 0;
};

class Bmp580 {
public:
    enum class Variant : uint8_t {
        Unknown = 0,
        BMP580_581,
        BMP585
    };

    static Bmp580TestState &state() {
        static Bmp580TestState instance;
        return instance;
    }

    bool begin() { return true; }
    bool start() { return state().start_ok; }
    void poll() {}
    bool takeNewData(float &pressure_hpa, float &temperature_c) {
        if (!state().has_new_data) {
            return false;
        }
        pressure_hpa = state().pressure;
        temperature_c = state().temperature;
        state().has_new_data = false;
        state().pressure_valid = true;
        state().last_data_ms = millis();
        return true;
    }
    bool isOk() const { return state().ok; }
    bool isPressureValid() const { return state().pressure_valid; }
    uint32_t lastDataMs() const { return state().last_data_ms; }
    Variant variant() const { return variant_state(); }
    const char *variantLabel() const {
        switch (variant_state()) {
            case Variant::BMP580_581:
                return "BMP580/581";
            case Variant::BMP585:
                return "BMP585";
            default:
                return "BMP58x";
        }
    }
    void invalidate() {
        state().pressure_valid = false;
        state().has_new_data = false;
        state().invalidate_called = true;
    }

    static Variant &variant_state() {
        static Variant instance = Variant::BMP580_581;
        return instance;
    }
};

#endif
