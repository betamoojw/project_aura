// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "modules/FanControl.h"

#include "config/AppConfig.h"
#include "core/Logger.h"

namespace {

bool timeReached(uint32_t now_ms, uint32_t deadline_ms) {
    return static_cast<int32_t>(now_ms - deadline_ms) >= 0;
}

} // namespace

void FanControl::begin(bool auto_mode_preference) {
    mode_ = auto_mode_preference ? Mode::Auto : Mode::Manual;
    manual_step_ = 1;
    selected_timer_s_ = 0;
    start_requested_ = false;
    stop_requested_ = false;
    available_ = false;
    faulted_ = false;
    applyStopState(true);
    manual_step_update_pending_ = false;
    timer_update_pending_ = false;
    last_recover_attempt_ms_ = 0;
    last_health_check_ms_ = 0;
    health_probe_fail_count_ = 0;
    boot_missing_lockout_ = false;

    if (!Config::DAC_FEATURE_ENABLED) {
        LOGI("FanControl", "DAC feature disabled");
        return;
    }

    const uint32_t now_ms = millis();
    if (tryInitialize(now_ms)) {
        LOGI("FanControl", "DAC ready at 0x%02X", Config::DAC_I2C_ADDR_DEFAULT);
    } else {
        LOGW("FanControl", "DAC not detected at boot, retry only after reboot");
        boot_missing_lockout_ = true;
        output_known_ = false;
    }
}

void FanControl::poll(uint32_t now_ms) {
    if (!Config::DAC_FEATURE_ENABLED) {
        available_ = false;
        faulted_ = false;
        applyStopState(true);
        return;
    }

    if (!available_) {
        if (!boot_missing_lockout_ &&
            now_ms - last_recover_attempt_ms_ >= Config::DAC_RECOVER_COOLDOWN_MS) {
            last_recover_attempt_ms_ = now_ms;
            if (tryInitialize(now_ms)) {
                LOGI("FanControl", "DAC recovered");
            }
        }
    } else if (!running_ &&
               now_ms - last_health_check_ms_ >= Config::DAC_HEALTH_CHECK_MS) {
        last_health_check_ms_ = now_ms;
        if (!dac_.probe()) {
            if (health_probe_fail_count_ < 0xFFu) {
                ++health_probe_fail_count_;
            }
            if (health_probe_fail_count_ >= Config::DAC_HEALTH_FAIL_THRESHOLD) {
                handleDacFault("probe failed");
            } else {
                LOGW("FanControl",
                     "DAC probe failed (%u/%u)",
                     static_cast<unsigned>(health_probe_fail_count_),
                     static_cast<unsigned>(Config::DAC_HEALTH_FAIL_THRESHOLD));
            }
        } else {
            health_probe_fail_count_ = 0;
        }
    }

    if (stop_requested_) {
        stop_requested_ = false;
        if (available_ && !applyOutputMillivolts(Config::DAC_SAFE_ERROR_MV)) {
            handleDacFault("safe stop write failed");
            return;
        }
        applyStopState(available_);
    }

    if (start_requested_) {
        start_requested_ = false;
        if (mode_ != Mode::Manual || !available_) {
            return;
        }

        const uint16_t target_mv = stepToMillivolts(manual_step_);
        if (!applyOutputMillivolts(target_mv)) {
            handleDacFault("start write failed");
            return;
        }

        running_ = true;
        output_mv_ = target_mv;
        manual_step_update_pending_ = false;
        if (selected_timer_s_ > 0) {
            stop_at_ms_ = now_ms + selected_timer_s_ * 1000UL;
        } else {
            stop_at_ms_ = 0;
        }
        timer_update_pending_ = false;
    }

    if (manual_step_update_pending_) {
        manual_step_update_pending_ = false;
        if (running_ && mode_ == Mode::Manual && available_) {
            const uint16_t target_mv = stepToMillivolts(manual_step_);
            if (!applyOutputMillivolts(target_mv)) {
                handleDacFault("manual level update failed");
                return;
            }
            output_mv_ = target_mv;
        }
    }

    if (timer_update_pending_) {
        timer_update_pending_ = false;
        if (running_ && mode_ == Mode::Manual) {
            if (selected_timer_s_ > 0) {
                stop_at_ms_ = now_ms + selected_timer_s_ * 1000UL;
            } else {
                stop_at_ms_ = 0;
            }
        }
    }

    if (running_ && stop_at_ms_ != 0 && timeReached(now_ms, stop_at_ms_)) {
        if (available_ && !applyOutputMillivolts(Config::DAC_SAFE_ERROR_MV)) {
            handleDacFault("timer stop write failed");
            return;
        }
        applyStopState(available_);
    }
}

void FanControl::setMode(Mode mode) {
    mode_ = mode;
    if (mode_ == Mode::Auto && running_) {
        requestStop();
    }
}

void FanControl::setManualStep(uint8_t step) {
    if (step < 1) {
        step = 1;
    } else if (step > 10) {
        step = 10;
    }
    if (manual_step_ != step) {
        manual_step_ = step;
        manual_step_update_pending_ = true;
    }
}

void FanControl::setTimerSeconds(uint32_t seconds) {
    if (selected_timer_s_ != seconds) {
        selected_timer_s_ = seconds;
        timer_update_pending_ = true;
    }
}

void FanControl::requestStart() {
    stop_requested_ = false;
    start_requested_ = true;
}

void FanControl::requestStop() {
    start_requested_ = false;
    stop_requested_ = true;
}

uint8_t FanControl::outputPercent() const {
    if (Config::DAC_VOUT_FULL_SCALE_MV == 0) {
        return 0;
    }
    uint32_t percent = static_cast<uint32_t>(output_mv_) * 100u;
    percent = (percent + (Config::DAC_VOUT_FULL_SCALE_MV / 2u)) / Config::DAC_VOUT_FULL_SCALE_MV;
    if (percent > 100u) {
        percent = 100u;
    }
    return static_cast<uint8_t>(percent);
}

uint32_t FanControl::remainingSeconds(uint32_t now_ms) const {
    if (!running_ || stop_at_ms_ == 0 || timeReached(now_ms, stop_at_ms_)) {
        return 0;
    }
    return (stop_at_ms_ - now_ms + 999UL) / 1000UL;
}

bool FanControl::tryInitialize(uint32_t now_ms) {
    if (!dac_.begin(Config::DAC_I2C_ADDR_DEFAULT)) {
        available_ = false;
        return false;
    }
    if (!dac_.setOutputRange10V()) {
        available_ = false;
        return false;
    }
    if (!dac_.writeChannelMillivolts(Config::DAC_CHANNEL_VOUT0, Config::DAC_SAFE_DEFAULT_MV)) {
        available_ = false;
        return false;
    }

    available_ = true;
    faulted_ = false;
    running_ = false;
    output_known_ = true;
    output_mv_ = Config::DAC_SAFE_DEFAULT_MV;
    stop_at_ms_ = 0;
    manual_step_update_pending_ = false;
    timer_update_pending_ = false;
    last_health_check_ms_ = now_ms;
    health_probe_fail_count_ = 0;
    return true;
}

bool FanControl::applyOutputMillivolts(uint16_t millivolts) {
    return dac_.writeChannelMillivolts(Config::DAC_CHANNEL_VOUT0, millivolts);
}

void FanControl::handleDacFault(const char *reason) {
    LOGW("FanControl", "DAC error: %s", reason ? reason : "unknown");
    available_ = false;
    faulted_ = true;
    applyStopState(false);
    health_probe_fail_count_ = 0;
    last_recover_attempt_ms_ = millis();
}

void FanControl::applyStopState(bool output_known) {
    running_ = false;
    output_known_ = output_known;
    if (output_known_) {
        output_mv_ = Config::DAC_SAFE_ERROR_MV;
    }
    stop_at_ms_ = 0;
    manual_step_update_pending_ = false;
    timer_update_pending_ = false;
}

uint16_t FanControl::stepToMillivolts(uint8_t step) const {
    if (step < 1) {
        step = 1;
    } else if (step > 10) {
        step = 10;
    }
    const uint16_t millivolts = static_cast<uint16_t>(step) * 1000u;
    if (millivolts > Config::DAC_VOUT_FULL_SCALE_MV) {
        return Config::DAC_VOUT_FULL_SCALE_MV;
    }
    return millivolts;
}
