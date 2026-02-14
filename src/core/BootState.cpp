// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/BootState.h"
#include <esp_attr.h>

namespace {

constexpr uint32_t BOOT_UI_AUTO_RECOVERY_MAGIC = 0xA11A0F5Au;
RTC_DATA_ATTR uint32_t boot_ui_auto_recovery_magic = 0;

} // namespace

RTC_DATA_ATTR uint32_t boot_count = 0;
RTC_DATA_ATTR uint32_t safe_boot_stage = 0;
esp_reset_reason_t boot_reset_reason = ESP_RST_UNKNOWN;
bool boot_i2c_recovered = false;
bool boot_touch_detected = false;
bool boot_ui_auto_recovery_reboot = false;

void boot_mark_ui_auto_recovery_reboot() {
    boot_ui_auto_recovery_magic = BOOT_UI_AUTO_RECOVERY_MAGIC;
}

bool boot_consume_ui_auto_recovery_reboot() {
    bool flagged = (boot_ui_auto_recovery_magic == BOOT_UI_AUTO_RECOVERY_MAGIC);
    boot_ui_auto_recovery_magic = 0;
    return flagged;
}
