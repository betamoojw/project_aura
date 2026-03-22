// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <stdint.h>

#include "modules/DacAutoConfig.h"

enum class FanMode : uint8_t {
    Manual = 0,
    Auto = 1,
};

struct FanStateSnapshot {
    bool present = false;
    bool available = false;
    bool running = false;
    bool faulted = false;
    bool output_known = true;
    bool manual_override_active = false;
    bool auto_resume_blocked = false;
    FanMode mode = FanMode::Manual;
    uint8_t manual_step = 1;
    uint32_t selected_timer_s = 0;
    uint16_t output_mv = 0;
    uint32_t stop_at_ms = 0;
    DacAutoConfig auto_config{};
};
