// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiEventBinder.h"

#include <stddef.h>

#include "ui/UiController.h"
#include "ui/ThemeManager.h"
#include "ui/ui.h"

lv_obj_t *UiEventBinder::screenRootById(int screen_id) {
    switch (screen_id) {
        case SCREEN_ID_PAGE_BOOT_LOGO:
            return objects.page_boot_logo;
        case SCREEN_ID_PAGE_BOOT_DIAG:
            return objects.page_boot_diag;
        case SCREEN_ID_PAGE_MAIN_PRO:
            return objects.page_main_pro;
        case SCREEN_ID_PAGE_SETTINGS:
            return objects.page_settings;
        case SCREEN_ID_PAGE_WIFI:
            return objects.page_wifi;
        case SCREEN_ID_PAGE_THEME:
            return objects.page_theme;
        case SCREEN_ID_PAGE_CLOCK:
            return objects.page_clock;
        case SCREEN_ID_PAGE_CO2_CALIB:
            return objects.page_co2_calib;
        case SCREEN_ID_PAGE_AUTO_NIGHT_MODE:
            return objects.page_auto_night_mode;
        case SCREEN_ID_PAGE_BACKLIGHT:
            return objects.page_backlight;
        case SCREEN_ID_PAGE_MQTT:
            return objects.page_mqtt;
        case SCREEN_ID_PAGE_SENSORS_INFO:
            return objects.page_sensors_info;
        default:
            return nullptr;
    }
}

bool UiEventBinder::objectBelongsToScreen(lv_obj_t *obj, lv_obj_t *screen_root) {
    if (!obj || !screen_root) {
        return false;
    }
    if (obj == screen_root) {
        return true;
    }
    return lv_obj_get_screen(obj) == screen_root;
}

void UiEventBinder::bindAvailableEvents(UiController &owner, int screen_id) {
    lv_obj_t *screen_root = screenRootById(screen_id);
    if (!screen_root) {
        return;
    }

    struct EventBinding {
        lv_obj_t *obj;
        lv_event_cb_t cb;
        lv_event_code_t code;
    };

    const EventBinding click_bindings[] = {
        {objects.btn_settings_1, UiController::on_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_back, UiController::on_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_about, UiController::on_about_event_cb, LV_EVENT_CLICKED},
        {objects.btn_about_back, UiController::on_about_back_event_cb, LV_EVENT_CLICKED},
        {objects.card_temp_pro, UiController::on_card_temp_event_cb, LV_EVENT_CLICKED},
        {objects.card_voc_pro, UiController::on_card_voc_event_cb, LV_EVENT_CLICKED},
        {objects.card_nox_pro, UiController::on_card_nox_event_cb, LV_EVENT_CLICKED},
        {objects.card_hcho_pro, UiController::on_card_hcho_event_cb, LV_EVENT_CLICKED},
        {objects.card_co2_pro, UiController::on_card_co2_event_cb, LV_EVENT_CLICKED},
        {objects.card_hum_pro, UiController::on_card_hum_event_cb, LV_EVENT_CLICKED},
        {objects.card_hum_2, UiController::on_dp_info_event_cb, LV_EVENT_CLICKED},
        {objects.card_pm1_pro, UiController::on_card_pm1_event_cb, LV_EVENT_CLICKED},
        {objects.card_pm25_pro, UiController::on_card_pm25_event_cb, LV_EVENT_CLICKED},
        {objects.card_pm10_pro, UiController::on_card_pm10_event_cb, LV_EVENT_CLICKED},
        {objects.card_co_pro, UiController::on_card_pm4_event_cb, LV_EVENT_CLICKED},
        {objects.card_pressure_pro, UiController::on_card_pressure_event_cb, LV_EVENT_CLICKED},
        {objects.btn_back_1, UiController::on_sensors_info_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_rh_info, UiController::on_rh_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_ah_info, UiController::on_ah_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mr_info, UiController::on_mr_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_dp_info, UiController::on_dp_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_3h_pressure_info, UiController::on_pressure_3h_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_24h_pressure_info, UiController::on_pressure_24h_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi, UiController::on_wifi_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_back, UiController::on_wifi_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mqtt, UiController::on_mqtt_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mqtt_back, UiController::on_mqtt_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mqtt_reconnect, UiController::on_mqtt_reconnect_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_reconnect, UiController::on_wifi_reconnect_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_start_ap, UiController::on_wifi_start_ap_event_cb, LV_EVENT_CLICKED},
        {objects.btn_time_date, UiController::on_time_date_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_dim, UiController::on_auto_night_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_head_status_1, UiController::on_backlight_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_language, UiController::on_language_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_back, UiController::on_backlight_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_back, UiController::on_auto_night_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_hours_minus, UiController::on_auto_night_start_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_hours_plus, UiController::on_auto_night_start_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_minutes_minus, UiController::on_auto_night_start_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_minutes_plus, UiController::on_auto_night_start_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_hours_minus, UiController::on_auto_night_end_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_hours_plus, UiController::on_auto_night_end_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_minutes_minus, UiController::on_auto_night_end_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_minutes_plus, UiController::on_auto_night_end_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_always_on, UiController::on_backlight_preset_always_on_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_30s, UiController::on_backlight_preset_30s_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_1m, UiController::on_backlight_preset_1m_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_5m, UiController::on_backlight_preset_5m_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_hours_minus, UiController::on_backlight_sleep_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_hours_plus, UiController::on_backlight_sleep_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_minutes_minus, UiController::on_backlight_sleep_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_minutes_plus, UiController::on_backlight_sleep_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_hours_minus, UiController::on_backlight_wake_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_hours_plus, UiController::on_backlight_wake_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_minutes_minus, UiController::on_backlight_wake_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_minutes_plus, UiController::on_backlight_wake_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_restart, UiController::on_restart_event_cb, LV_EVENT_CLICKED},
        {objects.btn_factory_reset, UiController::on_factory_reset_event_cb, LV_EVENT_CLICKED},
        {objects.btn_voc_reset, UiController::on_voc_reset_event_cb, LV_EVENT_CLICKED},
        {objects.btn_confirm_ok, UiController::on_confirm_ok_event_cb, LV_EVENT_CLICKED},
        {objects.btn_confirm_cancel, UiController::on_confirm_cancel_event_cb, LV_EVENT_CLICKED},
        {objects.btn_datetime_back, UiController::on_datetime_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_datetime_apply, UiController::on_datetime_apply_event_cb, LV_EVENT_CLICKED},
        {objects.btn_tz_plus, UiController::on_tz_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_tz_minus, UiController::on_tz_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_hours_minus, UiController::on_set_time_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_hours_plus, UiController::on_set_time_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_minutes_minus, UiController::on_set_time_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_minutes_plus, UiController::on_set_time_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_day_minus, UiController::on_set_date_day_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_day_plus, UiController::on_set_date_day_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_month_minus, UiController::on_set_date_month_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_month_plus, UiController::on_set_date_month_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_year_minus, UiController::on_set_date_year_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_year_plus, UiController::on_set_date_year_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_forget, UiController::on_wifi_forget_event_cb, LV_EVENT_CLICKED},
        {objects.btn_co2_calib, UiController::on_co2_calib_event_cb, LV_EVENT_CLICKED},
        {objects.btn_co2_calib_back, UiController::on_co2_calib_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_co2_calib_start, UiController::on_co2_calib_start_event_cb, LV_EVENT_CLICKED},
        {objects.btn_temp_offset_minus, UiController::on_temp_offset_minus_cb, LV_EVENT_CLICKED},
        {objects.btn_temp_offset_plus, UiController::on_temp_offset_plus_cb, LV_EVENT_CLICKED},
        {objects.btn_hum_offset_minus, UiController::on_hum_offset_minus_cb, LV_EVENT_CLICKED},
        {objects.btn_hum_offset_plus, UiController::on_hum_offset_plus_cb, LV_EVENT_CLICKED},
        {objects.btn_theme_color, UiController::on_theme_color_event_cb, LV_EVENT_CLICKED},
        {objects.btn_theme_back, UiController::on_theme_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_diag_continue, UiController::on_boot_diag_continue_cb, LV_EVENT_CLICKED},
    };

    const EventBinding value_bindings[] = {
        {objects.btn_head_status, UiController::on_head_status_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_wifi_toggle, UiController::on_wifi_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_mqtt_toggle, UiController::on_mqtt_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_auto_night_toggle, UiController::on_auto_night_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_backlight_schedule_toggle, UiController::on_backlight_schedule_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_night_mode, UiController::on_night_mode_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_units_c_f, UiController::on_units_c_f_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_units_mdy, UiController::on_units_mdy_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_led_indicators, UiController::on_led_indicators_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_alert_blink, UiController::on_alert_blink_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_co2_calib_asc, UiController::on_co2_calib_asc_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_ntp_toggle, UiController::on_ntp_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
    };

    auto bind_events = [screen_root](const EventBinding *bindings, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            const EventBinding &binding = bindings[i];
            if (!binding.obj || !binding.cb) {
                continue;
            }
            if (!objectBelongsToScreen(binding.obj, screen_root)) {
                continue;
            }
            lv_obj_remove_event_cb(binding.obj, binding.cb);
            lv_obj_add_event_cb(binding.obj, binding.cb, binding.code, nullptr);
        }
    };

    bind_events(click_bindings, sizeof(click_bindings) / sizeof(click_bindings[0]));
    bind_events(value_bindings, sizeof(value_bindings) / sizeof(value_bindings[0]));
}

void UiEventBinder::applyToggleStylesForAvailableObjects(UiController &owner, int screen_id) {
    lv_obj_t *screen_root = screenRootById(screen_id);
    if (!screen_root) {
        return;
    }

    lv_obj_t *toggle_buttons[] = {
        objects.btn_night_mode,
        objects.btn_auto_dim,
        objects.btn_wifi,
        objects.btn_mqtt,
        objects.btn_units_c_f,
        objects.btn_units_mdy,
        objects.btn_led_indicators,
        objects.btn_alert_blink,
        objects.btn_co2_calib_asc,
        objects.btn_head_status,
        objects.btn_wifi_toggle,
        objects.btn_mqtt_toggle,
        objects.btn_ntp_toggle,
        objects.btn_backlight_schedule_toggle,
        objects.btn_backlight_always_on,
        objects.btn_backlight_30s,
        objects.btn_backlight_1m,
        objects.btn_backlight_5m,
        objects.btn_auto_night_toggle,
        objects.btn_rh_info,
        objects.btn_ah_info,
        objects.btn_mr_info,
        objects.btn_dp_info,
        objects.btn_3h_pressure_info,
        objects.btn_24h_pressure_info,
    };

    for (lv_obj_t *btn : toggle_buttons) {
        if (!objectBelongsToScreen(btn, screen_root)) {
            continue;
        }
        owner.apply_toggle_style(btn);
    }
}

void UiEventBinder::applyCheckedStatesForAvailableObjects(UiController &owner, int screen_id) {
    lv_obj_t *screen_root = screenRootById(screen_id);
    if (!screen_root) {
        return;
    }

    auto set_checked = [screen_root](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (!objectBelongsToScreen(btn, screen_root)) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };

    set_checked(objects.btn_head_status, owner.header_status_enabled);
    set_checked(objects.btn_night_mode, owner.night_mode);
    set_checked(objects.btn_units_c_f, owner.temp_units_c);
    set_checked(objects.btn_units_mdy, owner.date_units_mdy);
    set_checked(objects.btn_led_indicators, owner.led_indicators_enabled);
    set_checked(objects.btn_alert_blink, owner.alert_blink_enabled);
    set_checked(objects.btn_co2_calib_asc, owner.co2_asc_enabled);
    set_checked(objects.btn_rh_info, true);
    set_checked(objects.btn_3h_pressure_info, true);
}

void UiEventBinder::initThemeControlsIfAvailable(UiController &owner) {
    if (!objects.page_theme) {
        return;
    }

    if (!owner.theme_events_bound_) {
        owner.themeManager.registerEvents(UiController::apply_toggle_style_cb,
                                          UiController::on_theme_swatch_event_cb,
                                          UiController::on_theme_tab_event_cb);
        owner.theme_events_bound_ = true;
    }

    owner.themeManager.selectSwatchByCurrent();
    bool presets = owner.themeManager.isCurrentPreset();
    if (objects.btn_theme_presets) {
        if (presets) lv_obj_add_state(objects.btn_theme_presets, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.btn_theme_presets, LV_STATE_CHECKED);
    }
    if (objects.btn_theme_custom) {
        if (presets) lv_obj_clear_state(objects.btn_theme_custom, LV_STATE_CHECKED);
        else lv_obj_add_state(objects.btn_theme_custom, LV_STATE_CHECKED);
    }
    owner.update_theme_custom_info(presets);
}
