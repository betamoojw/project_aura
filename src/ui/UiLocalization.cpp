// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiLocalization.h"

#include "ui/UiController.h"
#include "ui/ui.h"

void UiLocalization::refreshTextsForScreen(UiController &owner, int screen_id) {
    switch (screen_id) {
        case SCREEN_ID_PAGE_MAIN_PRO:
            owner.update_main_texts();
            break;
        case SCREEN_ID_PAGE_SETTINGS:
            owner.update_settings_texts();
            owner.update_confirm_texts();
            break;
        case SCREEN_ID_PAGE_WIFI:
            owner.update_wifi_texts();
            break;
        case SCREEN_ID_PAGE_THEME:
            owner.update_theme_texts();
            break;
        case SCREEN_ID_PAGE_CLOCK:
            owner.update_datetime_texts();
            break;
        case SCREEN_ID_PAGE_CO2_CALIB:
            owner.update_co2_calib_texts();
            break;
        case SCREEN_ID_PAGE_AUTO_NIGHT_MODE:
            owner.update_auto_night_texts();
            break;
        case SCREEN_ID_PAGE_BACKLIGHT:
            owner.update_backlight_texts();
            break;
        case SCREEN_ID_PAGE_MQTT:
            owner.update_mqtt_texts();
            break;
        case SCREEN_ID_PAGE_SENSORS_INFO:
            owner.update_sensor_info_texts();
            break;
        case SCREEN_ID_PAGE_BOOT_DIAG:
            owner.update_boot_diag_texts();
            break;
        case SCREEN_ID_PAGE_BOOT_LOGO:
        default:
            break;
    }

    if (owner.ui_language == Config::Language::ZH) {
        owner.update_language_fonts();
    }
}

void UiLocalization::refreshAllTexts(UiController &owner) {
    owner.update_language_label();
    owner.update_settings_texts();
    owner.update_main_texts();
    owner.update_sensor_info_texts();
    owner.update_confirm_texts();
    owner.update_wifi_texts();
    owner.update_mqtt_texts();
    owner.update_datetime_texts();
    owner.update_theme_texts();
    owner.update_auto_night_texts();
    owner.update_backlight_texts();
    owner.update_co2_calib_texts();
    owner.update_boot_diag_texts();
    owner.update_language_fonts();
}
