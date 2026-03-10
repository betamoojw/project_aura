// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/WifiPowerSaveGuard.h"

#include <WiFi.h>
#include <esp_wifi.h>

namespace {

uint32_t g_suspend_depth = 0;
bool g_prev_valid = false;
wifi_ps_type_t g_prev_mode = WIFI_PS_NONE;

} // namespace

WifiPowerSaveGuard::~WifiPowerSaveGuard() {
    restore();
}

bool WifiPowerSaveGuard::suspend() {
    if (active_) {
        return true;
    }

    const wifi_mode_t mode = WiFi.getMode();
    if ((mode & WIFI_MODE_STA) == 0) {
        return false;
    }

    if (g_suspend_depth == 0) {
        wifi_ps_type_t current_mode = WIFI_PS_NONE;
        if (esp_wifi_get_ps(&current_mode) != ESP_OK) {
            return false;
        }
        g_prev_mode = current_mode;
        g_prev_valid = true;
        if (current_mode != WIFI_PS_NONE) {
            if (esp_wifi_set_ps(WIFI_PS_NONE) != ESP_OK) {
                g_prev_valid = false;
                g_prev_mode = WIFI_PS_NONE;
                return false;
            }
        }
    }

    g_suspend_depth++;
    active_ = true;
    return true;
}

void WifiPowerSaveGuard::restore() {
    if (!active_) {
        return;
    }

    if (g_suspend_depth > 0) {
        g_suspend_depth--;
    }

    if (g_suspend_depth == 0) {
        if (g_prev_valid && g_prev_mode != WIFI_PS_NONE) {
            esp_wifi_set_ps(g_prev_mode);
        }
        g_prev_valid = false;
        g_prev_mode = WIFI_PS_NONE;
    }

    active_ = false;
}
