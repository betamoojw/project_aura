// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiLocalization.h"

#include <lvgl.h>

#include "modules/StorageManager.h"
#include "ui/UiController.h"
#include "ui/UiStrings.h"
#include "ui/fonts.h"
#include "ui/ui.h"

namespace {

const char *language_label(Config::Language lang) {
    switch (lang) {
        case Config::Language::DE: return "DEUTSCH";
        case Config::Language::ES: return "ESPA\xC3\x91OL";
        case Config::Language::FR: return "FRAN\xC3\x87" "AIS";
        case Config::Language::IT: return "ITALIANO";
        case Config::Language::PT: return "PORTUGU\xC3\x8AS BR";
        case Config::Language::NL: return "NEDERLANDS";
        case Config::Language::ZH: return "\xE7\xAE\x80\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87";
        case Config::Language::EN:
        default:
            return "ENGLISH";
    }
}

void replace_font_recursive(lv_obj_t *obj, const lv_font_t *from, const lv_font_t *to) {
    if (!obj || !from || !to || from == to) {
        return;
    }

    const lv_font_t *current = lv_obj_get_style_text_font(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (current == from) {
        lv_obj_set_style_text_font(obj, to, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const uint32_t child_count = lv_obj_get_child_cnt(obj);
    for (uint32_t i = 0; i < child_count; ++i) {
        replace_font_recursive(lv_obj_get_child(obj, i), from, to);
    }
}

} // namespace

Config::Language UiLocalization::nextLanguage(Config::Language current) {
    switch (current) {
        case Config::Language::EN: return Config::Language::DE;
        case Config::Language::DE: return Config::Language::ES;
        case Config::Language::ES: return Config::Language::FR;
        case Config::Language::FR: return Config::Language::IT;
        case Config::Language::IT: return Config::Language::PT;
        case Config::Language::PT: return Config::Language::NL;
        case Config::Language::NL: return Config::Language::ZH;
        case Config::Language::ZH: return Config::Language::EN;
        default:
            return Config::Language::EN;
    }
}

void UiLocalization::applyCurrentLanguage(UiController &owner) {
    UiStrings::setLanguage(owner.ui_language);
    refreshAllTexts(owner);
}

void UiLocalization::cycleLanguage(UiController &owner) {
    owner.ui_language = nextLanguage(owner.ui_language);
    owner.language_dirty = (owner.ui_language != owner.storage.config().language);
    applyCurrentLanguage(owner);
}

void UiLocalization::updateLanguageLabel(UiController &owner) {
    if (objects.label_language_value) {
        owner.safe_label_set_text(objects.label_language_value, language_label(owner.ui_language));
    }
}

void UiLocalization::updateLanguageFonts(UiController &owner) {
    const bool is_zh = (owner.ui_language == Config::Language::ZH);
    const lv_font_t *from_14 = is_zh ? &ui_font_jet_reg_14 : &ui_font_noto_sans_sc_reg_14;
    const lv_font_t *to_14 = is_zh ? &ui_font_noto_sans_sc_reg_14 : &ui_font_jet_reg_14;
    const lv_font_t *from_18 = is_zh ? &ui_font_jet_reg_18 : &ui_font_noto_sans_sc_reg_18;
    const lv_font_t *to_18 = is_zh ? &ui_font_noto_sans_sc_reg_18 : &ui_font_jet_reg_18;

    lv_obj_t *roots[] = {
        objects.page_boot_logo,
        objects.page_boot_diag,
        objects.page_main_pro,
        objects.page_settings,
        objects.page_wifi,
        objects.page_theme,
        objects.page_clock,
        objects.page_co2_calib,
        objects.page_auto_night_mode,
        objects.page_backlight,
        objects.page_mqtt,
        objects.page_sensors_info,
        objects.page_dac_settings,
    };

    for (lv_obj_t *root : roots) {
        replace_font_recursive(root, from_14, to_14);
        replace_font_recursive(root, from_18, to_18);
    }
}

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
        case SCREEN_ID_PAGE_DAC_SETTINGS:
            break;
        case SCREEN_ID_PAGE_BOOT_DIAG:
            owner.update_boot_diag_texts();
            break;
        case SCREEN_ID_PAGE_BOOT_LOGO:
        default:
            break;
    }

    if (owner.ui_language == Config::Language::ZH) {
        updateLanguageFonts(owner);
    }
}

void UiLocalization::refreshAllTexts(UiController &owner) {
    updateLanguageLabel(owner);
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
    updateLanguageFonts(owner);
}
