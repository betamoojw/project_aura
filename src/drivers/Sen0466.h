// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once
#include <Arduino.h>

class Sen0466 {
public:
    bool begin();
    bool start();
    void poll();
    bool isPresent() const { return present_; }
    bool isDataValid() const { return data_valid_; }
    bool isWarmupActive() const;
    float coPpm() const { return co_ppm_; }
    uint32_t lastDataMs() const { return last_data_ms_; }
    void invalidate();

private:
    bool pingAddress();
    bool setPassiveMode();
    bool readGasConcentration(float &co_ppm, uint8_t &gas_type);
    bool transact(const uint8_t *tx_frame, uint8_t *rx_frame);
    static uint8_t checksum7(const uint8_t *frame);
    static uint8_t checksum6(const uint8_t *frame);
    static void buildFrame(uint8_t command,
                           uint8_t arg0,
                           uint8_t arg1,
                           uint8_t arg2,
                           uint8_t arg3,
                           uint8_t arg4,
                           uint8_t *frame);

    bool present_ = false;
    bool data_valid_ = false;
    bool warned_type_mismatch_ = false;
    float co_ppm_ = 0.0f;
    uint8_t fail_count_ = 0;
    uint32_t warmup_started_ms_ = 0;
    uint32_t last_poll_ms_ = 0;
    uint32_t last_data_ms_ = 0;
    uint32_t last_retry_ms_ = 0;
    bool fail_cooldown_active_ = false;
    uint32_t fail_cooldown_started_ms_ = 0;
    uint8_t cooldown_recover_fail_count_ = 0;
    uint8_t start_attempts_ = 0;
    bool start_retry_exhausted_logged_ = false;
};
