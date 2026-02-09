// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiDeferredUnload.h"

#include <string.h>

#include "ui/ui.h"

const int UiDeferredUnload::kScreenIds[UiDeferredUnload::kCount] = {
    SCREEN_ID_PAGE_WIFI,
    SCREEN_ID_PAGE_MQTT,
    SCREEN_ID_PAGE_CLOCK,
    SCREEN_ID_PAGE_CO2_CALIB,
    SCREEN_ID_PAGE_AUTO_NIGHT_MODE,
    SCREEN_ID_PAGE_BACKLIGHT,
};

void UiDeferredUnload::reset() {
    memset(unload_at_ms_, 0, sizeof(unload_at_ms_));
}

void UiDeferredUnload::scheduleOnSwitch(int previous_screen_id, int current_screen_id, uint32_t now_ms) {
    for (size_t i = 0; i < kCount; ++i) {
        const int unload_screen_id = kScreenIds[i];
        if (previous_screen_id == unload_screen_id && current_screen_id != unload_screen_id) {
            unload_at_ms_[i] = now_ms + kDelayMs;
        } else if (current_screen_id == unload_screen_id) {
            unload_at_ms_[i] = 0;
        }
    }
}

bool UiDeferredUnload::ready(size_t index, uint32_t now_ms, int pending_screen_id, int current_screen_id) const {
    if (index >= kCount) {
        return false;
    }
    const uint32_t unload_at_ms = unload_at_ms_[index];
    if (unload_at_ms == 0 || pending_screen_id != 0) {
        return false;
    }
    const int unload_screen_id = kScreenIds[index];
    return current_screen_id != unload_screen_id && now_ms >= unload_at_ms;
}

int UiDeferredUnload::screenId(size_t index) const {
    if (index >= kCount) {
        return 0;
    }
    return kScreenIds[index];
}

void UiDeferredUnload::clear(size_t index) {
    if (index < kCount) {
        unload_at_ms_[index] = 0;
    }
}

void UiDeferredUnload::retry(size_t index, uint32_t now_ms) {
    if (index < kCount) {
        unload_at_ms_[index] = now_ms + kRetryMs;
    }
}

