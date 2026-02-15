// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <Arduino.h>

#include "modules/DacAutoConfig.h"
#include "drivers/Gp8403.h"

struct SensorData;

class FanControl {
public:
    enum class Mode : uint8_t {
        Manual = 0,
        Auto = 1,
    };

    void begin(bool auto_mode_preference);
    void poll(uint32_t now_ms, const SensorData *sensor_data, bool gas_warmup);

    void setMode(Mode mode);
    void setManualStep(uint8_t step);
    void setTimerSeconds(uint32_t seconds);
    void requestStart();
    void requestStop();
    void requestAutoStart();
    void setAutoConfig(const DacAutoConfig &config);
    const DacAutoConfig &autoConfig() const { return auto_config_; }

    bool isAvailable() const { return available_; }
    bool isRunning() const { return running_; }
    bool isFaulted() const { return faulted_; }
    bool isOutputKnown() const { return output_known_; }
    bool isManualOverrideActive() const { return manual_override_active_; }
    bool isAutoResumeBlocked() const { return auto_resume_blocked_; }
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
    void applyStopState(bool output_known);
    uint16_t stepToMillivolts(uint8_t step) const;
    uint16_t percentToMillivolts(uint8_t percent) const;
    uint8_t evaluateAutoDemandPercent(const SensorData &data, bool gas_warmup) const;
    static uint8_t maxPercent(uint8_t a, uint8_t b) { return (a > b) ? a : b; }

    Gp8403 dac_;
    DacAutoConfig auto_config_{};
    Mode mode_ = Mode::Manual;
    uint8_t manual_step_ = 1;
    uint32_t selected_timer_s_ = 0;
    bool start_requested_ = false;
    bool stop_requested_ = false;
    bool available_ = false;
    bool running_ = false;
    bool faulted_ = false;
    bool output_known_ = true;
    bool manual_override_active_ = false;
    uint16_t output_mv_ = 0;
    uint32_t stop_at_ms_ = 0;
    bool manual_step_update_pending_ = false;
    bool timer_update_pending_ = false;
    uint32_t last_recover_attempt_ms_ = 0;
    uint32_t last_health_check_ms_ = 0;
    uint8_t health_probe_fail_count_ = 0;
    bool boot_missing_lockout_ = false;
    bool auto_resume_blocked_ = false;
};
