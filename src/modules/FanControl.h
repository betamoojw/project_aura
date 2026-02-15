// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <Arduino.h>

#include "drivers/Gp8403.h"

class FanControl {
public:
    enum class Mode : uint8_t {
        Manual = 0,
        Auto = 1,
    };

    void begin(bool auto_mode_preference);
    void poll(uint32_t now_ms);

    void setMode(Mode mode);
    void setManualStep(uint8_t step);
    void setTimerSeconds(uint32_t seconds);
    void requestStart();
    void requestStop();

    bool isAvailable() const { return available_; }
    bool isRunning() const { return running_; }
    Mode mode() const { return mode_; }
    uint8_t manualStep() const { return manual_step_; }
    uint32_t selectedTimerSeconds() const { return selected_timer_s_; }
    uint16_t outputMillivolts() const { return output_mv_; }
    uint8_t outputPercent() const;
    uint32_t remainingSeconds(uint32_t now_ms) const;

private:
    bool tryInitialize(uint32_t now_ms);
    bool applyOutputMillivolts(uint16_t millivolts);
    void handleDacFault(const char *reason);
    void applyStopState();
    uint16_t stepToMillivolts(uint8_t step) const;

    Gp8403 dac_;
    Mode mode_ = Mode::Manual;
    uint8_t manual_step_ = 1;
    uint32_t selected_timer_s_ = 0;
    bool start_requested_ = false;
    bool stop_requested_ = false;
    bool available_ = false;
    bool running_ = false;
    uint16_t output_mv_ = 0;
    uint32_t stop_at_ms_ = 0;
    uint32_t last_recover_attempt_ms_ = 0;
    uint32_t last_health_check_ms_ = 0;
};

