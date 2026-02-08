// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"
#include "ui/UiText.h"
#include "core/MathUtils.h"
#include "ui/fonts.h"

#include <math.h>
#include <string.h>
#include <time.h>

#include <WiFi.h>
#include <esp_heap_caps.h>

#include "lvgl_v8_port.h"
#include "ui/ui.h"
#include "ui/styles.h"
#include "ui/images.h"
#include "ui/StatusMessages.h"
#include "web/WebHandlers.h"
#include "config/AppConfig.h"
#include "core/Logger.h"
#include "core/BootState.h"
#include "modules/StorageManager.h"
#include "modules/NetworkManager.h"
#include "modules/MqttManager.h"
#include "modules/SensorManager.h"
#include "modules/TimeManager.h"
#include "ui/ThemeManager.h"
#include "ui/BacklightManager.h"
#include "ui/NightModeManager.h"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

using namespace Config;

namespace {

const char *reset_reason_to_string(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "POWERON";
        case ESP_RST_EXT: return "EXT";
        case ESP_RST_SW: return "SW";
        case ESP_RST_PANIC: return "PANIC";
        case ESP_RST_INT_WDT: return "INT_WDT";
        case ESP_RST_TASK_WDT: return "TASK_WDT";
        case ESP_RST_WDT: return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_SDIO: return "SDIO";
        default: return "UNKNOWN";
    }
}

bool is_crash_reset(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
            return true;
        default:
            return false;
    }
}

const char *language_label(Language lang) {
    switch (lang) {
        case Language::DE: return "DEUTSCH";
        case Language::ES: return "ESPAÑOL";
        case Language::FR: return "FRANÇAIS";
        case Language::IT: return "ITALIANO";
        case Language::PT: return "PORTUGUÊS BR";
        case Language::NL: return "NEDERLANDS";
        case Language::ZH: return "简体中文";
        case Language::EN:
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

void set_visible(lv_obj_t *obj, bool visible) {
    if (!obj) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *screen_root_by_id(int screen_id) {
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

constexpr uint32_t STATUS_ROTATE_MS = 5000;

float map_float_clamped(float value, float in_min, float in_max, float out_min, float out_max) {
    if (in_max <= in_min) return out_min;
    float v = value;
    if (v < in_min) v = in_min;
    if (v > in_max) v = in_max;
    return out_min + (out_max - out_min) * (v - in_min) / (in_max - in_min);
}

int score_from_thresholds(float value, float min_val, float t_good, float t_mod, float t_poor) {
    if (value <= t_good) {
        return static_cast<int>(lroundf(map_float_clamped(value, min_val, t_good, 0.0f, 25.0f)));
    }
    if (value <= t_mod) {
        return static_cast<int>(lroundf(map_float_clamped(value, t_good, t_mod, 25.0f, 50.0f)));
    }
    if (value <= t_poor) {
        return static_cast<int>(lroundf(map_float_clamped(value, t_mod, t_poor, 50.0f, 75.0f)));
    }
    float cap = t_poor * 1.5f;
    float score = map_float_clamped(value, t_poor, cap, 75.0f, 100.0f);
    if (score < 75.0f) score = 75.0f;
    if (score > 100.0f) score = 100.0f;
    return static_cast<int>(lroundf(score));
}

int score_from_voc(float value) {
    return score_from_thresholds(value, 0.0f, 150.0f, 250.0f, 350.0f);
}

} // namespace

UiController *UiController::instance_ = nullptr;

UiController::UiController(const UiContext &context)
    : storage(context.storage),
      networkManager(context.networkManager),
      mqttManager(context.mqttManager),
      sensorManager(context.sensorManager),
      timeManager(context.timeManager),
      themeManager(context.themeManager),
      backlightManager(context.backlightManager),
      nightModeManager(context.nightModeManager),
      currentData(context.currentData),
      night_mode(context.night_mode),
      temp_units_c(context.temp_units_c),
      led_indicators_enabled(context.led_indicators_enabled),
      alert_blink_enabled(context.alert_blink_enabled),
      co2_asc_enabled(context.co2_asc_enabled),
      temp_offset(context.temp_offset),
      hum_offset(context.hum_offset) {
    instance_ = this;
}

void UiController::setLvglReady(bool ready) {
    lvgl_ready = ready;
}

void UiController::bind_available_events() {
    struct EventBinding {
        lv_obj_t *obj;
        lv_event_cb_t cb;
        lv_event_code_t code;
    };

    const EventBinding click_bindings[] = {
        {objects.btn_settings_1, on_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_back, on_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_about, on_about_event_cb, LV_EVENT_CLICKED},
        {objects.btn_about_back, on_about_back_event_cb, LV_EVENT_CLICKED},
        {objects.card_temp_pro, on_card_temp_event_cb, LV_EVENT_CLICKED},
        {objects.card_voc_pro, on_card_voc_event_cb, LV_EVENT_CLICKED},
        {objects.card_nox_pro, on_card_nox_event_cb, LV_EVENT_CLICKED},
        {objects.card_hcho_pro, on_card_hcho_event_cb, LV_EVENT_CLICKED},
        {objects.card_co2_pro, on_card_co2_event_cb, LV_EVENT_CLICKED},
        {objects.card_hum_pro, on_card_hum_event_cb, LV_EVENT_CLICKED},
        {objects.card_hum_2, on_dp_info_event_cb, LV_EVENT_CLICKED},
        {objects.card_pm1_pro, on_card_pm1_event_cb, LV_EVENT_CLICKED},
        {objects.card_pm25_pro, on_card_pm25_event_cb, LV_EVENT_CLICKED},
        {objects.card_pm10_pro, on_card_pm10_event_cb, LV_EVENT_CLICKED},
        {objects.card_co_pro, on_card_pm4_event_cb, LV_EVENT_CLICKED},
        {objects.card_pressure_pro, on_card_pressure_event_cb, LV_EVENT_CLICKED},
        {objects.btn_back_1, on_sensors_info_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_rh_info, on_rh_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_ah_info, on_ah_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mr_info, on_mr_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_dp_info, on_dp_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_3h_pressure_info, on_pressure_3h_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_24h_pressure_info, on_pressure_24h_info_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi, on_wifi_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_back, on_wifi_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mqtt, on_mqtt_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mqtt_back, on_mqtt_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_mqtt_reconnect, on_mqtt_reconnect_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_reconnect, on_wifi_reconnect_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_start_ap, on_wifi_start_ap_event_cb, LV_EVENT_CLICKED},
        {objects.btn_time_date, on_time_date_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_dim, on_auto_night_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_head_status_1, on_backlight_settings_event_cb, LV_EVENT_CLICKED},
        {objects.btn_language, on_language_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_back, on_backlight_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_back, on_auto_night_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_hours_minus, on_auto_night_start_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_hours_plus, on_auto_night_start_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_minutes_minus, on_auto_night_start_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_start_minutes_plus, on_auto_night_start_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_hours_minus, on_auto_night_end_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_hours_plus, on_auto_night_end_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_minutes_minus, on_auto_night_end_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_auto_night_end_minutes_plus, on_auto_night_end_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_always_on, on_backlight_preset_always_on_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_30s, on_backlight_preset_30s_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_1m, on_backlight_preset_1m_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_5m, on_backlight_preset_5m_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_hours_minus, on_backlight_sleep_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_hours_plus, on_backlight_sleep_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_minutes_minus, on_backlight_sleep_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_sleep_minutes_plus, on_backlight_sleep_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_hours_minus, on_backlight_wake_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_hours_plus, on_backlight_wake_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_minutes_minus, on_backlight_wake_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_backlight_wake_minutes_plus, on_backlight_wake_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_restart, on_restart_event_cb, LV_EVENT_CLICKED},
        {objects.btn_factory_reset, on_factory_reset_event_cb, LV_EVENT_CLICKED},
        {objects.btn_voc_reset, on_voc_reset_event_cb, LV_EVENT_CLICKED},
        {objects.btn_confirm_ok, on_confirm_ok_event_cb, LV_EVENT_CLICKED},
        {objects.btn_confirm_cancel, on_confirm_cancel_event_cb, LV_EVENT_CLICKED},
        {objects.btn_datetime_back, on_datetime_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_datetime_apply, on_datetime_apply_event_cb, LV_EVENT_CLICKED},
        {objects.btn_tz_plus, on_tz_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_tz_minus, on_tz_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_hours_minus, on_set_time_hours_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_hours_plus, on_set_time_hours_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_minutes_minus, on_set_time_minutes_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_time_minutes_plus, on_set_time_minutes_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_day_minus, on_set_date_day_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_day_plus, on_set_date_day_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_month_minus, on_set_date_month_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_month_plus, on_set_date_month_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_year_minus, on_set_date_year_minus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_set_date_year_plus, on_set_date_year_plus_event_cb, LV_EVENT_CLICKED},
        {objects.btn_wifi_forget, on_wifi_forget_event_cb, LV_EVENT_CLICKED},
        {objects.btn_co2_calib, on_co2_calib_event_cb, LV_EVENT_CLICKED},
        {objects.btn_co2_calib_back, on_co2_calib_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_co2_calib_start, on_co2_calib_start_event_cb, LV_EVENT_CLICKED},
        {objects.btn_temp_offset_minus, on_temp_offset_minus_cb, LV_EVENT_CLICKED},
        {objects.btn_temp_offset_plus, on_temp_offset_plus_cb, LV_EVENT_CLICKED},
        {objects.btn_hum_offset_minus, on_hum_offset_minus_cb, LV_EVENT_CLICKED},
        {objects.btn_hum_offset_plus, on_hum_offset_plus_cb, LV_EVENT_CLICKED},
        {objects.btn_theme_color, on_theme_color_event_cb, LV_EVENT_CLICKED},
        {objects.btn_theme_back, on_theme_back_event_cb, LV_EVENT_CLICKED},
        {objects.btn_diag_continue, on_boot_diag_continue_cb, LV_EVENT_CLICKED},
    };

    const EventBinding value_bindings[] = {
        {objects.btn_head_status, on_head_status_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_wifi_toggle, on_wifi_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_mqtt_toggle, on_mqtt_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_auto_night_toggle, on_auto_night_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_backlight_schedule_toggle, on_backlight_schedule_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_night_mode, on_night_mode_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_units_c_f, on_units_c_f_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_led_indicators, on_led_indicators_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_alert_blink, on_alert_blink_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_co2_calib_asc, on_co2_calib_asc_event_cb, LV_EVENT_VALUE_CHANGED},
        {objects.btn_ntp_toggle, on_ntp_toggle_event_cb, LV_EVENT_VALUE_CHANGED},
    };

    auto bind_events = [](const EventBinding *bindings, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            const EventBinding &binding = bindings[i];
            if (!binding.obj || !binding.cb) {
                continue;
            }
            lv_obj_remove_event_cb(binding.obj, binding.cb);
            lv_obj_add_event_cb(binding.obj, binding.cb, binding.code, nullptr);
        }
    };

    bind_events(click_bindings, sizeof(click_bindings) / sizeof(click_bindings[0]));
    bind_events(value_bindings, sizeof(value_bindings) / sizeof(value_bindings[0]));
}

void UiController::apply_toggle_styles_for_available_objects() {
    lv_obj_t *toggle_buttons[] = {
        objects.btn_night_mode,
        objects.btn_auto_dim,
        objects.btn_wifi,
        objects.btn_mqtt,
        objects.btn_units_c_f,
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
        apply_toggle_style(btn);
    }
}

void UiController::apply_checked_states_for_available_objects() {
    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };

    set_checked(objects.btn_head_status, true);
    set_checked(objects.btn_night_mode, night_mode);
    set_checked(objects.btn_units_c_f, temp_units_c);
    set_checked(objects.btn_led_indicators, led_indicators_enabled);
    set_checked(objects.btn_alert_blink, alert_blink_enabled);
    set_checked(objects.btn_co2_calib_asc, co2_asc_enabled);
    set_checked(objects.btn_rh_info, true);
    set_checked(objects.btn_3h_pressure_info, true);
}

void UiController::init_theme_controls_if_available() {
    if (!objects.page_theme) {
        return;
    }

    if (!theme_events_bound_) {
        themeManager.registerEvents(apply_toggle_style_cb, on_theme_swatch_event_cb, on_theme_tab_event_cb);
        theme_events_bound_ = true;
    }

    // Theme screen can be created lazily; refresh selected swatch against current active colors.
    themeManager.selectSwatchByCurrent();
    bool presets = themeManager.isCurrentPreset();
    if (objects.btn_theme_presets) {
        if (presets) lv_obj_add_state(objects.btn_theme_presets, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.btn_theme_presets, LV_STATE_CHECKED);
    }
    if (objects.btn_theme_custom) {
        if (presets) lv_obj_clear_state(objects.btn_theme_custom, LV_STATE_CHECKED);
        else lv_obj_add_state(objects.btn_theme_custom, LV_STATE_CHECKED);
    }
    update_theme_custom_info(presets);
}

void UiController::bind_screen_events_once(int screen_id) {
    // Keep compatibility with stale references to old MAIN screen id.
    if (screen_id == SCREEN_ID_PAGE_MAIN) {
        screen_id = SCREEN_ID_PAGE_MAIN_PRO;
    }
    if (screen_id <= 0 || screen_id >= static_cast<int>(kScreenSlotCount)) {
        return;
    }
    if (!screen_root_by_id(screen_id)) {
        return;
    }
    if (screen_events_bound_[screen_id]) {
        return;
    }

    bind_available_events();
    apply_toggle_styles_for_available_objects();
    apply_checked_states_for_available_objects();
    refresh_texts_for_screen(screen_id);
    init_theme_controls_if_available();
    if (screen_id == SCREEN_ID_PAGE_SENSORS_INFO) {
        restore_sensor_info_selection();
    }

    screen_events_bound_[screen_id] = true;
}

void UiController::refresh_texts_for_screen(int screen_id) {
    switch (screen_id) {
        case SCREEN_ID_PAGE_MAIN_PRO:
            update_main_texts();
            break;
        case SCREEN_ID_PAGE_SETTINGS:
            update_settings_texts();
            update_confirm_texts();
            break;
        case SCREEN_ID_PAGE_WIFI:
            update_wifi_texts();
            break;
        case SCREEN_ID_PAGE_THEME:
            update_theme_texts();
            break;
        case SCREEN_ID_PAGE_CLOCK:
            update_datetime_texts();
            break;
        case SCREEN_ID_PAGE_CO2_CALIB:
            update_co2_calib_texts();
            break;
        case SCREEN_ID_PAGE_AUTO_NIGHT_MODE:
            update_auto_night_texts();
            break;
        case SCREEN_ID_PAGE_BACKLIGHT:
            update_backlight_texts();
            break;
        case SCREEN_ID_PAGE_MQTT:
            update_mqtt_texts();
            break;
        case SCREEN_ID_PAGE_SENSORS_INFO:
            update_sensor_info_texts();
            break;
        case SCREEN_ID_PAGE_BOOT_DIAG:
            update_boot_diag_texts();
            break;
        case SCREEN_ID_PAGE_BOOT_LOGO:
        default:
            break;
    }

    if (ui_language == Config::Language::ZH) {
        update_language_fonts();
    }
}

void UiController::begin() {
    instance_ = this;
    if (!lvgl_ready) {
        return;
    }
    lvgl_port_lock(-1);
    ui_init();
    themeManager.initAfterUi(storage, night_mode, datetime_ui_dirty);
    if (night_mode) {
        night_mode_on_enter();
    }
    init_ui_defaults();
    if (objects.label_boot_ver) {
        char version_text[24];
        snprintf(version_text, sizeof(version_text), "v%s", APP_VERSION);
        safe_label_set_text(objects.label_boot_ver, version_text);
    }
    current_screen_id = SCREEN_ID_PAGE_MAIN_PRO;
    pending_screen_id = SCREEN_ID_PAGE_MAIN_PRO;
    memset(screen_events_bound_, 0, sizeof(screen_events_bound_));
    theme_events_bound_ = false;
    boot_release_at_ms = 0;
    boot_ui_released = false;
    if (objects.page_boot_logo) {
        loadScreen(SCREEN_ID_PAGE_BOOT_LOGO);
        bind_screen_events_once(SCREEN_ID_PAGE_BOOT_LOGO);
        current_screen_id = SCREEN_ID_PAGE_BOOT_LOGO;
        pending_screen_id = 0;
        boot_logo_active = true;
        boot_logo_start_ms = millis();
    }
    lvgl_port_unlock();
    last_clock_tick_ms = millis();
}

void UiController::clear_boot_object_refs() {
    objects.page_boot_logo = nullptr;
    objects.page_boot_diag = nullptr;
    objects.label_boot_ver = nullptr;
    objects.background_boot_diag = nullptr;
    objects.btn_diag_continue = nullptr;
    objects.label_btn_diag_continue = nullptr;
    objects.lbl_diag_title = nullptr;
    objects.lbl_diag_system_title = nullptr;
    objects.lbl_diag_app_label = nullptr;
    objects.lbl_diag_mac_label = nullptr;
    objects.lbl_diag_reason_label = nullptr;
    objects.lbl_diag_heap_label = nullptr;
    objects.lbl_diag_storage_label = nullptr;
    objects.lbl_diag_app_ver = nullptr;
    objects.lbl_diag_mac = nullptr;
    objects.lbl_diag_reason = nullptr;
    objects.lbl_diag_heap = nullptr;
    objects.lbl_diag_storage = nullptr;
    objects.lbl_diag_sensors_title = nullptr;
    objects.lbl_diag_i2c_label = nullptr;
    objects.lbl_diag_touch_label = nullptr;
    objects.lbl_diag_sen_label = nullptr;
    objects.lbl_diag_dps_label = nullptr;
    objects.lbl_diag_sfa_label = nullptr;
    objects.lbl_diag_i2c = nullptr;
    objects.lbl_diag_touch = nullptr;
    objects.lbl_diag_sen = nullptr;
    objects.lbl_diag_dps = nullptr;
    objects.lbl_diag_sfa = nullptr;
    objects.lbl_diag_rtc_label = nullptr;
    objects.lbl_diag_rtc = nullptr;
    objects.lbl_diag_error = nullptr;
}

void UiController::release_boot_screens() {
    if (boot_ui_released) {
        return;
    }

    lv_obj_t *boot_logo = objects.page_boot_logo;
    lv_obj_t *boot_diag = objects.page_boot_diag;
    if (boot_logo && lv_obj_is_valid(boot_logo)) {
        lv_obj_del_async(boot_logo);
    }
    if (boot_diag && lv_obj_is_valid(boot_diag)) {
        lv_obj_del_async(boot_diag);
    }

    clear_boot_object_refs();
    screen_events_bound_[SCREEN_ID_PAGE_BOOT_LOGO] = false;
    screen_events_bound_[SCREEN_ID_PAGE_BOOT_DIAG] = false;
    boot_logo_active = false;
    boot_diag_active = false;
    boot_diag_has_error = false;
    boot_release_at_ms = 0;
    boot_ui_released = true;

    LOGI("UI", "boot screens released");
}

void UiController::onSensorPoll(const SensorManager::PollResult &poll) {
    if (poll.data_changed || poll.warmup_changed) {
        data_dirty = true;
    }
}

void UiController::onTimePoll(const TimeManager::PollResult &poll) {
    if (poll.state_changed) {
        datetime_ui_dirty = true;
    }
    if (poll.time_updated) {
        apply_auto_night_now();
        clock_ui_dirty = true;
        datetime_ui_dirty = true;
    }
}

void UiController::markDatetimeDirty() {
    datetime_ui_dirty = true;
}

void UiController::mqtt_sync_with_wifi() {
    mqttManager.syncWithWifi();
    sync_mqtt_toggle_state();
}

void UiController::poll(uint32_t now) {
    bool desired = false;
    if (nightModeManager.poll(night_mode, desired)) {
        set_night_mode_state(desired, true);
    }
    if (now - last_clock_tick_ms >= CLOCK_TICK_MS) {
        last_clock_tick_ms = now;
        clock_ui_dirty = true;
    }
    if (now - last_blink_ms >= BLINK_PERIOD_MS) {
        last_blink_ms = now;
            if (alert_blink_enabled) {
                blink_state = !blink_state;
                if (current_screen_id == SCREEN_ID_PAGE_MAIN_PRO ||
                    current_screen_id == SCREEN_ID_PAGE_SETTINGS ||
                    current_screen_id == SCREEN_ID_PAGE_SENSORS_INFO) {
                    data_dirty = true;
                }
            }
    }
    if (current_screen_id == SCREEN_ID_PAGE_MAIN_PRO &&
        status_msg_count > 1 &&
        (now - status_msg_last_ms) >= STATUS_ROTATE_MS) {
        data_dirty = true;
    }

    if (!lvgl_ready) {
        return;
    }

    if (boot_logo_active &&
        (now - boot_logo_start_ms) >= Config::BOOT_LOGO_MS &&
        current_screen_id == SCREEN_ID_PAGE_BOOT_LOGO &&
        pending_screen_id == 0) {
        if (objects.page_boot_diag) {
            pending_screen_id = SCREEN_ID_PAGE_BOOT_DIAG;
            boot_diag_active = true;
            boot_diag_has_error = false;
            boot_diag_start_ms = now;
            last_boot_diag_update_ms = 0;
        } else {
            pending_screen_id = SCREEN_ID_PAGE_MAIN_PRO;
        }
        boot_logo_active = false;
        data_dirty = true;
    }

    if (boot_diag_active &&
        current_screen_id == SCREEN_ID_PAGE_BOOT_DIAG &&
        pending_screen_id == 0 &&
        !boot_diag_has_error &&
        (now - boot_diag_start_ms) >= Config::BOOT_DIAG_MS) {
        pending_screen_id = SCREEN_ID_PAGE_MAIN_PRO;
        boot_diag_active = false;
        data_dirty = true;
    }

    bool allow_ui_update = true;
    if (networkManager.state() == AuraNetworkManager::WIFI_STATE_AP_CONFIG &&
        (now - last_ui_update_ms) < WIFI_UI_UPDATE_MS) {
        allow_ui_update = false;
    }
    lvgl_port_lock(-1);
    mqtt_apply_pending();
    if ((now - last_ui_tick_ms) >= UI_TICK_MS) {
        ui_tick();
        last_ui_tick_ms = now;
    }
    backlightManager.poll(lvgl_ready);
    update_status_icons();
    if (pending_screen_id != 0) {
        int next_screen = pending_screen_id;
        // Keep compatibility with stale references to old MAIN screen id.
        if (next_screen == SCREEN_ID_PAGE_MAIN) {
            next_screen = SCREEN_ID_PAGE_MAIN_PRO;
        }
        ScreensEnum next_screen_enum = static_cast<ScreensEnum>(next_screen);
        loadScreen(next_screen_enum);
        bind_screen_events_once(next_screen);
        current_screen_id = next_screen;
        pending_screen_id = 0;
        if (current_screen_id == SCREEN_ID_PAGE_SETTINGS) {
            temp_offset_ui_dirty = true;
            hum_offset_ui_dirty = true;
            data_dirty = true;
        } else if (current_screen_id == SCREEN_ID_PAGE_MAIN_PRO) {
            data_dirty = true;
        } else if (current_screen_id == SCREEN_ID_PAGE_SENSORS_INFO) {
            data_dirty = true;
        } else if (current_screen_id == SCREEN_ID_PAGE_CLOCK) {
            datetime_ui_dirty = true;
            clock_ui_dirty = true;
        } else if (current_screen_id == SCREEN_ID_PAGE_WIFI) {
            networkManager.markUiDirty();
        } else if (current_screen_id == SCREEN_ID_PAGE_BACKLIGHT) {
            backlightManager.markUiDirty();
        } else if (current_screen_id == SCREEN_ID_PAGE_AUTO_NIGHT_MODE) {
            nightModeManager.markUiDirty();
        }

        if (current_screen_id == SCREEN_ID_PAGE_MAIN_PRO &&
            !boot_ui_released &&
            (objects.page_boot_logo || objects.page_boot_diag)) {
            boot_release_at_ms = now + 500;
        }
    }

    if (!boot_ui_released &&
        boot_release_at_ms != 0 &&
        pending_screen_id == 0 &&
        current_screen_id == SCREEN_ID_PAGE_MAIN_PRO &&
        now >= boot_release_at_ms) {
        release_boot_screens();
    }

    if (allow_ui_update &&
        current_screen_id == SCREEN_ID_PAGE_BOOT_DIAG &&
        (now - last_boot_diag_update_ms) >= 200) {
        update_boot_diag(now);
        last_boot_diag_update_ms = now;
    }
    if (allow_ui_update) {
        bool did_update = false;
        if (temp_offset_ui_dirty) {
            update_temp_offset_label();
            temp_offset_ui_dirty = false;
            did_update = true;
        }
        if (hum_offset_ui_dirty) {
            update_hum_offset_label();
            hum_offset_ui_dirty = false;
            did_update = true;
        }
        if (networkManager.isUiDirty()) {
            update_wifi_ui();
            networkManager.clearUiDirty();
            did_update = true;
        }
        if (mqttManager.isUiDirty()) {
            update_mqtt_ui();
            mqttManager.clearUiDirty();
            did_update = true;
        }
        if (clock_ui_dirty) {
            update_clock_labels();
            clock_ui_dirty = false;
            did_update = true;
        }
        if (datetime_ui_dirty && current_screen_id == SCREEN_ID_PAGE_CLOCK) {
            update_datetime_ui();
            datetime_ui_dirty = false;
            did_update = true;
        }
        if (backlightManager.isUiDirty() && current_screen_id == SCREEN_ID_PAGE_BACKLIGHT) {
            backlightManager.updateUi();
            did_update = true;
        }
        if (nightModeManager.isUiDirty() && current_screen_id == SCREEN_ID_PAGE_AUTO_NIGHT_MODE) {
            nightModeManager.updateUi();
            did_update = true;
        }
            if (data_dirty) {
                if (current_screen_id == SCREEN_ID_PAGE_MAIN_PRO) {
                    update_ui();
                } else if (current_screen_id == SCREEN_ID_PAGE_SETTINGS) {
                    update_settings_header();
                } else if (current_screen_id == SCREEN_ID_PAGE_SENSORS_INFO) {
                    update_sensor_info_ui();
                }
                data_dirty = false;
                did_update = true;
            }
        if (did_update) {
            last_ui_update_ms = now;
        }
    }
    lvgl_port_unlock();
}

void UiController::safe_label_set_text(lv_obj_t *obj, const char *new_text) {
    if (!obj) return;
    const char *current = lv_label_get_text(obj);
    if (current && strcmp(current, new_text) == 0) return;
    lv_label_set_text(obj, new_text);
}

void UiController::safe_label_set_text_static(lv_obj_t *obj, const char *new_text) {
    if (!obj) return;
    lv_label_set_text_static(obj, new_text);
}

lv_color_t UiController::color_inactive() { return lv_color_hex(0x3a3a3a); }

lv_color_t UiController::color_green() { return lv_color_hex(0x00c853); }
lv_color_t UiController::color_yellow() { return lv_color_hex(0xffeb3b); }
lv_color_t UiController::color_orange() { return lv_color_hex(0xff9800); }
lv_color_t UiController::color_red() { return lv_color_hex(0xff1100); }
lv_color_t UiController::color_blue() { return lv_color_hex(0x2196f3); }
lv_color_t UiController::color_card_border() {
    if (objects.card_co2_pro) {
        return lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }
    if (objects.card_co2) {
        return lv_obj_get_style_border_color(objects.card_co2, LV_PART_MAIN);
    }
    return lv_color_hex(0xffe19756);
}

lv_color_t UiController::getTempColor(float t) {
    if (t >= 20.0f && t <= 25.0f) return color_green();
    if ((t >= 18.0f && t < 20.0f) || (t > 25.0f && t <= 26.0f)) return color_yellow();
    if ((t >= 16.0f && t < 18.0f) || (t > 26.0f && t <= 28.0f)) return color_orange();
    return color_red();
}

lv_color_t UiController::getHumidityColor(float h) {
    if (h >= 40.0f && h <= 60.0f) return color_green();
    if ((h >= 30.0f && h < 40.0f) || (h > 60.0f && h <= 65.0f)) return color_yellow();
    if ((h >= 20.0f && h < 30.0f) || (h > 65.0f && h <= 70.0f)) return color_orange();
    return color_red();
}

lv_color_t UiController::getAbsoluteHumidityColor(float ah) {
    if (!isfinite(ah)) return color_inactive();
    if (ah < 4.0f) return color_red();
    if (ah < 5.0f) return color_orange();
    if (ah < 7.0f) return color_yellow();
    if (ah < 15.0f) return color_green();
    if (ah <= 18.0f) return color_yellow();
    if (ah <= 20.0f) return color_orange();
    return color_red();
}

lv_color_t UiController::getDewPointColor(float dew_c) {
    if (!isfinite(dew_c)) return color_inactive();
    if (dew_c < 5.0f) return color_red();
    if (dew_c <= 8.0f) return color_orange();
    if (dew_c <= 10.0f) return color_yellow();
    if (dew_c <= 16.0f) return color_green();
    if (dew_c <= 18.0f) return color_yellow();
    if (dew_c <= 21.0f) return color_orange();
    return color_red();
}

lv_color_t UiController::getCO2Color(int co2) {
    if (co2 < 800) return color_green();
    if (co2 <= 1000) return color_yellow();
    if (co2 <= 1500) return color_orange();
    return color_red();
}

lv_color_t UiController::getPM25Color(float pm) {
    if (pm <= 12.0f) return color_green();
    if (pm <= 35.0f) return color_yellow();
    if (pm <= 55.0f) return color_orange();
    return color_red();
}

lv_color_t UiController::getPM10Color(float pm) {
    if (pm <= 54.0f) return color_green();
    if (pm <= 154.0f) return color_yellow();
    if (pm <= 254.0f) return color_orange();
    return color_red();
}

lv_color_t UiController::getPressureDeltaColor(float delta, bool valid, bool is24h) {
    if (!valid) return color_inactive();
    float d = fabsf(delta);
    if (is24h) {
        if (d < 2.0f) return color_green();
        if (d <= 6.0f) return color_yellow();
        if (d <= 10.0f) return color_orange();
        return color_red();
    }
    if (d < 1.0f) return color_green();
    if (d <= 3.0f) return color_yellow();
    if (d <= 6.0f) return color_orange();
    return color_red();
}

lv_color_t UiController::getVOCColor(int voc) {
    if (voc <= 150) return color_green();
    if (voc <= 250) return color_yellow();
    if (voc <= 350) return color_orange();
    return color_red();
}

lv_color_t UiController::getNOxColor(int nox) {
    if (nox <= 50) return color_green();
    if (nox <= 100) return color_yellow();
    if (nox <= 200) return color_orange();
    return color_red();
}

lv_color_t UiController::getHCHOColor(float hcho_ppb, bool valid) {
    if (!valid || !isfinite(hcho_ppb) || hcho_ppb < 0.0f) return color_inactive();
    if (hcho_ppb < 30.0f) return color_green();
    if (hcho_ppb <= 60.0f) return color_yellow();
    if (hcho_ppb <= 100.0f) return color_orange();
    return color_red();
}

AirQuality UiController::getAirQuality(const SensorData &data) {
    AirQuality aq;
    bool gas_warmup = sensorManager.isWarmupActive();
    bool has_valid = false;
    int max_score = 0;

    if (data.co2_valid && data.co2 > 0) {
        int score = score_from_thresholds(static_cast<float>(data.co2), 400.0f, 800.0f, 1000.0f, 1500.0f);
        max_score = max(max_score, score);
        has_valid = true;
    }

    if (data.pm25_valid && isfinite(data.pm25) && data.pm25 >= 0.0f) {
        int score = score_from_thresholds(data.pm25, 0.0f, 12.0f, 35.0f, 55.0f);
        max_score = max(max_score, score);
        has_valid = true;
    }

    if (data.hcho_valid && isfinite(data.hcho) && data.hcho >= 0.0f) {
        int score = score_from_thresholds(data.hcho, 0.0f, 30.0f, 60.0f, 100.0f);
        max_score = max(max_score, score);
        has_valid = true;
    }

    if (!gas_warmup && data.nox_valid && data.nox_index >= 0) {
        int score = score_from_thresholds(static_cast<float>(data.nox_index), 1.0f, 50.0f, 100.0f, 200.0f);
        max_score = max(max_score, score);
        has_valid = true;
    }

    if (!gas_warmup && data.voc_valid && data.voc_index >= 0) {
        int score = score_from_voc(static_cast<float>(data.voc_index));
        max_score = max(max_score, score);
        has_valid = true;
    }

    if (!has_valid) {
        aq.status = UiText::StatusInitializing();
        aq.score = 0;
        aq.color = color_blue();
        return aq;
    }

    aq.score = max_score;

    if (aq.score <= 25)      { aq.status = UiText::QualityExcellent(); aq.color = color_green(); }
    else if (aq.score <= 50) { aq.status = UiText::QualityGood();      aq.color = color_green(); }
    else if (aq.score <= 75) { aq.status = UiText::QualityModerate();  aq.color = color_yellow(); }
    else                     { aq.status = UiText::QualityPoor();     aq.color = color_red(); }

    return aq;
}

void UiController::set_dot_color(lv_obj_t *obj, lv_color_t color) {
    if (!obj) return;
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (color.full == color_inactive().full) {
        lv_obj_set_style_shadow_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_shadow_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

lv_color_t UiController::blink_red(lv_color_t color) {
    if (alert_blink_enabled && (color.full == color_red().full) && !blink_state) {
        return color_inactive();
    }
    return color;
}

lv_color_t UiController::night_alert_color(lv_color_t color) {
    if (color.full == color_red().full) {
        return color_red();
    }
    return color_inactive();
}

lv_color_t UiController::alert_color_for_mode(lv_color_t color) {
    if (night_mode) {
        return night_alert_color(color);
    }
    return blink_red(color);
}

void UiController::compute_header_style(const AirQuality &aq, lv_color_t &color, lv_opa_t &shadow_opa) {
    lv_color_t base = header_status_enabled ? aq.color : color_card_border();
    shadow_opa = header_status_enabled ? LV_OPA_COVER : LV_OPA_TRANSP;
    if (alert_blink_enabled && header_status_enabled && (base.full == color_red().full) && !blink_state) {
        color = color_inactive();
        shadow_opa = LV_OPA_TRANSP;
        return;
    }
    color = base;
}

void UiController::apply_toggle_style(lv_obj_t *btn) {
    if (!btn) return;
    lv_obj_set_style_border_color(btn, color_green(), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_color(btn, color_green(), LV_PART_MAIN | LV_STATE_CHECKED);
}

void UiController::update_clock_labels() {
    char buf[16];
    tm local_tm = {};
    if (!timeManager.getLocalTime(local_tm)) {
        if (objects.label_time_value_1) safe_label_set_text(objects.label_time_value_1, UiText::TimeMissing());
        if (objects.label_date_value_1) safe_label_set_text(objects.label_date_value_1, UiText::DateMissing());
        if (objects.label_time_value_2) safe_label_set_text(objects.label_time_value_2, UiText::TimeMissing());
        if (objects.label_date_value_2) safe_label_set_text(objects.label_date_value_2, UiText::DateMissing());
        return;
    }
    snprintf(buf, sizeof(buf), "%02d:%02d", local_tm.tm_hour, local_tm.tm_min);
    if (objects.label_time_value_1) safe_label_set_text(objects.label_time_value_1, buf);
    if (objects.label_time_value_2) safe_label_set_text(objects.label_time_value_2, buf);
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d",
             local_tm.tm_mday,
             local_tm.tm_mon + 1,
             local_tm.tm_year + 1900);
    if (objects.label_date_value_1) safe_label_set_text(objects.label_date_value_1, buf);
    if (objects.label_date_value_2) safe_label_set_text(objects.label_date_value_2, buf);
}

bool UiController::boot_diag_has_errors(uint32_t now_ms) {
    bool has_error = false;
    if (!storage.isMounted()) {
        has_error = true;
    }
    if (!boot_i2c_recovered) {
        has_error = true;
    }
    if (!boot_touch_detected) {
        has_error = true;
    }
    if (is_crash_reset(boot_reset_reason)) {
        has_error = true;
    }
    if (!sensorManager.isOk()) {
        uint32_t retry_at = sensorManager.retryAtMs();
        if (retry_at == 0 || now_ms >= retry_at) {
            has_error = true;
        }
    }
    if (!sensorManager.isDpsOk()) {
        has_error = true;
    }
    if (!sensorManager.isSfaOk()) {
        has_error = true;
    }
    if (timeManager.isRtcPresent()) {
        if (timeManager.isRtcLostPower() || !timeManager.isRtcValid()) {
            has_error = true;
        }
    }
    return has_error;
}

void UiController::update_boot_diag(uint32_t now_ms) {
    update_boot_diag_texts();
    char buf[64];

    if (objects.lbl_diag_app_ver) {
        snprintf(buf, sizeof(buf), "v%s", APP_VERSION);
        safe_label_set_text(objects.lbl_diag_app_ver, buf);
    }
    if (objects.lbl_diag_mac) {
        String mac = WiFi.macAddress();
        safe_label_set_text(objects.lbl_diag_mac, mac.c_str());
    }
    if (objects.lbl_diag_reason) {
        const char *reason = reset_reason_to_string(boot_reset_reason);
        if (safe_boot_stage > 0) {
            snprintf(buf, sizeof(buf), "%s / boot=%lu safe=%lu",
                     reason,
                     static_cast<unsigned long>(boot_count),
                     static_cast<unsigned long>(safe_boot_stage));
        } else {
            snprintf(buf, sizeof(buf), "%s / boot=%lu",
                     reason,
                     static_cast<unsigned long>(boot_count));
        }
        safe_label_set_text(objects.lbl_diag_reason, buf);
    }
    if (objects.lbl_diag_heap) {
        size_t free_bytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        size_t min_bytes = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
        size_t max_bytes = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        snprintf(buf, sizeof(buf), "free %uk / min %uk / max %uk",
                 static_cast<unsigned>(free_bytes / 1024),
                 static_cast<unsigned>(min_bytes / 1024),
                 static_cast<unsigned>(max_bytes / 1024));
        safe_label_set_text(objects.lbl_diag_heap, buf);
    }
    if (objects.lbl_diag_storage) {
        const char *status = UiText::StatusErr();
        if (storage.isMounted()) {
            status = storage.isConfigLoaded() ? UiText::BootDiagStorageOkConfig()
                                              : UiText::BootDiagStorageOkDefaults();
        }
        safe_label_set_text(objects.lbl_diag_storage, status);
    }
    if (objects.lbl_diag_i2c) {
        safe_label_set_text(objects.lbl_diag_i2c,
                            boot_i2c_recovered ? UiText::BootDiagRecovered() : UiText::BootDiagFail());
    }
    if (objects.lbl_diag_touch) {
        safe_label_set_text(objects.lbl_diag_touch,
                            boot_touch_detected ? UiText::BootDiagDetected() : UiText::BootDiagFail());
    }
    if (objects.lbl_diag_sen) {
        const char *status = UiText::StatusErr();
        if (sensorManager.isOk()) {
            status = UiText::StatusOk();
        } else {
            uint32_t retry_at = sensorManager.retryAtMs();
            if (retry_at != 0 && now_ms < retry_at) {
                status = UiText::BootDiagStarting();
            }
        }
        safe_label_set_text(objects.lbl_diag_sen, status);
    }
    if (objects.lbl_diag_dps_label) {
        safe_label_set_text(objects.lbl_diag_dps_label, sensorManager.pressureSensorLabel());
    }
    if (objects.lbl_diag_dps) {
        safe_label_set_text(objects.lbl_diag_dps,
                            sensorManager.isDpsOk() ? UiText::StatusOk() : UiText::StatusErr());
    }
    if (objects.lbl_diag_sfa) {
        safe_label_set_text(objects.lbl_diag_sfa,
                            sensorManager.isSfaOk() ? UiText::StatusOk() : UiText::StatusErr());
    }
    if (objects.lbl_diag_rtc) {
        const char *status = UiText::BootDiagNotFound();
        if (timeManager.isRtcPresent()) {
            if (timeManager.isRtcLostPower()) {
                status = UiText::BootDiagLost();
            } else if (timeManager.isRtcValid()) {
                status = UiText::StatusOk();
            } else {
                status = UiText::StatusErr();
            }
        }
        safe_label_set_text(objects.lbl_diag_rtc, status);
    }

    bool has_errors = boot_diag_has_errors(now_ms);
    boot_diag_has_error = has_errors;
    set_visible(objects.lbl_diag_error, has_errors);
    set_visible(objects.btn_diag_continue, has_errors);
}

void UiController::set_button_enabled(lv_obj_t *btn, bool enabled) {
    if (!btn) return;
    if (enabled) lv_obj_clear_state(btn, LV_STATE_DISABLED);
    else lv_obj_add_state(btn, LV_STATE_DISABLED);
}

lv_color_t UiController::active_text_color() {
    return themeManager.activeTextColor(night_mode);
}

void UiController::set_night_mode_state(bool enabled, bool save_pref) {
    if (enabled == night_mode) {
        return;
    }
    if (save_pref) {
        storage.config().night_mode = enabled;
        storage.saveConfig(true);
    }
    night_mode = enabled;
    if (!lvgl_ready) {
        return;
    }
    if (enabled) {
        night_mode_on_enter();
    }
    themeManager.applyActive(night_mode, datetime_ui_dirty);
    if (!enabled) {
        night_mode_on_exit();
    }
    data_dirty = true;
}

void UiController::apply_auto_night_now() {
    bool desired = false;
    if (nightModeManager.applyNow(night_mode, desired)) {
        set_night_mode_state(desired, true);
    }
}

void UiController::sync_night_mode_toggle_ui() {
    if (!objects.btn_night_mode) {
        return;
    }
    set_button_enabled(objects.btn_night_mode, !nightModeManager.isAutoEnabled());
    if (night_mode) {
        lv_obj_add_state(objects.btn_night_mode, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(objects.btn_night_mode, LV_STATE_CHECKED);
    }
}

void UiController::sync_auto_dim_button_state() {
    if (!objects.btn_auto_dim) {
        return;
    }
    if (nightModeManager.isAutoEnabled()) {
        lv_obj_add_state(objects.btn_auto_dim, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(objects.btn_auto_dim, LV_STATE_CHECKED);
    }
}

void UiController::confirm_show(ConfirmAction action) {
    confirm_action = action;
    if (!objects.container_confirm) {
        return;
    }
    bool show_voc = (action == CONFIRM_VOC_RESET);
    bool show_restart = (action == CONFIRM_RESTART);
    bool show_reset = (action == CONFIRM_FACTORY_RESET);

    set_visible(objects.container_confirm, true);
    set_visible(objects.container_confirm_card, true);
    set_visible(objects.btn_confirm_ok, true);
    set_visible(objects.btn_confirm_cancel, true);
    set_visible(objects.label_btn_confirm_cancel, true);
    lv_obj_add_flag(objects.container_confirm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(objects.container_confirm);

    set_visible(objects.label_btn_confirm_voc, show_voc);
    set_visible(objects.label_confirm_title_voc, show_voc);
    set_visible(objects.container_confirm_voc_text, show_voc);

    set_visible(objects.label_btn_confirm_restart, show_restart);
    set_visible(objects.label_confirm_title_restart, show_restart);
    set_visible(objects.container_confirm_restart_text, show_restart);

    set_visible(objects.label_btn_confirm_reset, show_reset);
    set_visible(objects.label_confirm_title_reset, show_reset);
    set_visible(objects.container_confirm_reset_text, show_reset);
}

void UiController::confirm_hide() {
    confirm_action = CONFIRM_NONE;
    set_visible(objects.container_confirm, false);
}

void UiController::mqtt_apply_pending() {
    MqttManager::PendingCommands pending;
    if (!mqttManager.takePending(pending)) {
        return;
    }
    bool publish_needed = false;
    if (pending.night_mode) {
        bool prev_night = night_mode;
        set_night_mode_state(pending.night_mode_value, true);
        sync_night_mode_toggle_ui();
        if (night_mode != prev_night) {
            publish_needed = true;
        }
    }
    if (pending.alert_blink) {
        if (alert_blink_enabled != pending.alert_blink_value) {
            alert_blink_enabled = pending.alert_blink_value;
            storage.config().alert_blink = alert_blink_enabled;
            storage.saveConfig(true);
            if (alert_blink_enabled) {
                blink_state = true;
                last_blink_ms = millis();
            }
            sync_alert_blink_toggle_state();
            data_dirty = true;
            publish_needed = true;
        }
    }
    if (pending.backlight) {
        bool prev_backlight = backlightManager.isOn();
        backlightManager.setOn(pending.backlight_value);
        if (backlightManager.isOn() != prev_backlight) {
            publish_needed = true;
        }
    }
    if (pending.restart) {
        LOGI("UI", "MQTT restart requested");
        delay(100);
        ESP.restart();
    }
    if (publish_needed) {
        mqttManager.requestPublish();
    }
}

void UiController::sync_alert_blink_toggle_state() {
    if (!objects.btn_alert_blink) {
        return;
    }
    alert_blink_syncing = true;
    if (alert_blink_enabled) {
        lv_obj_add_state(objects.btn_alert_blink, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(objects.btn_alert_blink, LV_STATE_CHECKED);
    }
    alert_blink_syncing = false;
}

void UiController::night_mode_on_enter() {
    alert_blink_before_night = alert_blink_enabled;
    night_blink_restore_pending = true;
    night_blink_user_changed = false;
    if (alert_blink_enabled) {
        alert_blink_enabled = false;
        sync_alert_blink_toggle_state();
    }
}

void UiController::night_mode_on_exit() {
    if (night_blink_restore_pending && !night_blink_user_changed) {
        if (alert_blink_enabled != alert_blink_before_night) {
            alert_blink_enabled = alert_blink_before_night;
            if (alert_blink_enabled) {
                blink_state = true;
                last_blink_ms = millis();
            }
            sync_alert_blink_toggle_state();
        }
    }
    night_blink_restore_pending = false;
    night_blink_user_changed = false;
}

void UiController::sync_wifi_toggle_state() {
    bool wifi_enabled = networkManager.isEnabled();
    if (objects.btn_wifi) {
        if (wifi_enabled) lv_obj_add_state(objects.btn_wifi, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.btn_wifi, LV_STATE_CHECKED);
    }
    if (objects.btn_wifi_toggle) {
        if (wifi_enabled) lv_obj_add_state(objects.btn_wifi_toggle, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.btn_wifi_toggle, LV_STATE_CHECKED);
    }
}

void UiController::sync_mqtt_toggle_state() {
    if (objects.btn_mqtt) {
        if (mqttManager.isEnabled()) lv_obj_add_state(objects.btn_mqtt, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.btn_mqtt, LV_STATE_CHECKED);
    }
    if (objects.btn_mqtt_toggle) {
        if (mqttManager.isEnabled()) lv_obj_add_state(objects.btn_mqtt_toggle, LV_STATE_CHECKED);
        else lv_obj_clear_state(objects.btn_mqtt_toggle, LV_STATE_CHECKED);
    }
}

void UiController::update_temp_offset_label() {
    if (!objects.label_temp_offset_value) {
        return;
    }
    float val = temp_offset;
    if (fabsf(val) < 0.05f) {
        val = 0.0f;
    }
    char buf[16];
    if (val > 0.0f) {
        snprintf(buf, sizeof(buf), "+%.1f", val);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", val);
    }
    safe_label_set_text(objects.label_temp_offset_value, buf);
}

void UiController::update_hum_offset_label() {
    if (!objects.label_hum_offset_value) {
        return;
    }
    float val = hum_offset;
    if (fabsf(val) < 0.5f) {
        val = 0.0f;
    }
    char buf[16];
    if (val > 0.0f) {
        snprintf(buf, sizeof(buf), "+%.0f%%", val);
    } else if (val < 0.0f) {
        snprintf(buf, sizeof(buf), "%.0f%%", val);
    } else {
        strcpy(buf, UiText::ValueZeroPercent());
    }
    safe_label_set_text(objects.label_hum_offset_value, buf);
}

void UiController::update_led_indicators() {
    const bool visible = led_indicators_enabled;
    if (objects.dot_co2) visible ? lv_obj_clear_flag(objects.dot_co2, LV_OBJ_FLAG_HIDDEN)
                                 : lv_obj_add_flag(objects.dot_co2, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_co2_1) visible ? lv_obj_clear_flag(objects.dot_co2_1, LV_OBJ_FLAG_HIDDEN)
                                   : lv_obj_add_flag(objects.dot_co2_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_temp) visible ? lv_obj_clear_flag(objects.dot_temp, LV_OBJ_FLAG_HIDDEN)
                                  : lv_obj_add_flag(objects.dot_temp, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_temp_1) visible ? lv_obj_clear_flag(objects.dot_temp_1, LV_OBJ_FLAG_HIDDEN)
                                    : lv_obj_add_flag(objects.dot_temp_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_hum) visible ? lv_obj_clear_flag(objects.dot_hum, LV_OBJ_FLAG_HIDDEN)
                                 : lv_obj_add_flag(objects.dot_hum, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_hum_1) visible ? lv_obj_clear_flag(objects.dot_hum_1, LV_OBJ_FLAG_HIDDEN)
                                   : lv_obj_add_flag(objects.dot_hum_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_dp) visible ? lv_obj_clear_flag(objects.dot_dp, LV_OBJ_FLAG_HIDDEN)
                                : lv_obj_add_flag(objects.dot_dp, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_dp_1) visible ? lv_obj_clear_flag(objects.dot_dp_1, LV_OBJ_FLAG_HIDDEN)
                                  : lv_obj_add_flag(objects.dot_dp_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_ah) visible ? lv_obj_clear_flag(objects.dot_ah, LV_OBJ_FLAG_HIDDEN)
                                : lv_obj_add_flag(objects.dot_ah, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_ah_1) visible ? lv_obj_clear_flag(objects.dot_ah_1, LV_OBJ_FLAG_HIDDEN)
                                  : lv_obj_add_flag(objects.dot_ah_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_pm25) visible ? lv_obj_clear_flag(objects.dot_pm25, LV_OBJ_FLAG_HIDDEN)
                                  : lv_obj_add_flag(objects.dot_pm25, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_pm25_1) visible ? lv_obj_clear_flag(objects.dot_pm25_1, LV_OBJ_FLAG_HIDDEN)
                                    : lv_obj_add_flag(objects.dot_pm25_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_pm10) visible ? lv_obj_clear_flag(objects.dot_pm10, LV_OBJ_FLAG_HIDDEN)
                                  : lv_obj_add_flag(objects.dot_pm10, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_pm10_pro) visible ? lv_obj_clear_flag(objects.dot_pm10_pro, LV_OBJ_FLAG_HIDDEN)
                                      : lv_obj_add_flag(objects.dot_pm10_pro, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_pm1) visible ? lv_obj_clear_flag(objects.dot_pm1, LV_OBJ_FLAG_HIDDEN)
                                 : lv_obj_add_flag(objects.dot_pm1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_voc) visible ? lv_obj_clear_flag(objects.dot_voc, LV_OBJ_FLAG_HIDDEN)
                                 : lv_obj_add_flag(objects.dot_voc, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_voc_1) visible ? lv_obj_clear_flag(objects.dot_voc_1, LV_OBJ_FLAG_HIDDEN)
                                   : lv_obj_add_flag(objects.dot_voc_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_nox) visible ? lv_obj_clear_flag(objects.dot_nox, LV_OBJ_FLAG_HIDDEN)
                                 : lv_obj_add_flag(objects.dot_nox, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_nox_1) visible ? lv_obj_clear_flag(objects.dot_nox_1, LV_OBJ_FLAG_HIDDEN)
                                   : lv_obj_add_flag(objects.dot_nox_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_hcho) visible ? lv_obj_clear_flag(objects.dot_hcho, LV_OBJ_FLAG_HIDDEN)
                                  : lv_obj_add_flag(objects.dot_hcho, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_hcho_1) visible ? lv_obj_clear_flag(objects.dot_hcho_1, LV_OBJ_FLAG_HIDDEN)
                                    : lv_obj_add_flag(objects.dot_hcho_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_co) visible ? lv_obj_clear_flag(objects.dot_co, LV_OBJ_FLAG_HIDDEN)
                                : lv_obj_add_flag(objects.dot_co, LV_OBJ_FLAG_HIDDEN);
    if (objects.dot_mr) visible ? lv_obj_clear_flag(objects.dot_mr, LV_OBJ_FLAG_HIDDEN)
                                : lv_obj_add_flag(objects.dot_mr, LV_OBJ_FLAG_HIDDEN);
}

void UiController::set_chip_color(lv_obj_t *obj, lv_color_t color) {
    if (!obj) return;
    lv_obj_set_style_border_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (color.full == color_inactive().full) {
        lv_obj_set_style_shadow_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_shadow_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void UiController::update_co2_bar(int co2, bool valid) {
    if (!objects.co2_bar_fill || !objects.co2_marker) {
        return;
    }
    if (!valid) {
        if (objects.co2_bar_mask) {
            lv_obj_set_width(objects.co2_bar_mask, 0);
        } else {
            lv_obj_set_width(objects.co2_bar_fill, 0);
        }
        lv_obj_set_x(objects.co2_marker, 2);
        return;
    }

    int bar_max = 330;
    int fill_w = lv_obj_get_width(objects.co2_bar_fill);
    if (fill_w > 0) {
        bar_max = fill_w;
    }
    int clamped = constrain(co2, 400, 2000);
    int w = map(clamped, 400, 2000, 0, bar_max);
    w = constrain(w, 0, bar_max);
    if (objects.co2_bar_mask) {
        lv_obj_set_width(objects.co2_bar_mask, w);
    } else {
        lv_obj_set_width(objects.co2_bar_fill, w);
    }

    const int marker_w = 14;
    int center = 4 + w;
    int x = center - (marker_w / 2);
    int track_w = objects.co2_bar_track ? lv_obj_get_width(objects.co2_bar_track) : 0;
    int max_x = (track_w > 0) ? (track_w - marker_w - 2) : (340 - marker_w - 2);
    x = constrain(x, 2, max_x);
    lv_obj_set_x(objects.co2_marker, x);
}

void UiController::update_ui() {
    AirQuality aq = getAirQuality(currentData);
    bool gas_warmup = sensorManager.isWarmupActive();
    bool show_co2_bar = !night_mode;
    const uint32_t now_ms = millis();
    update_status_message(now_ms, gas_warmup);
    lv_color_t header_col;
    lv_opa_t header_shadow;
    compute_header_style(aq, header_col, header_shadow);
    if (night_mode && header_status_enabled) {
        header_col = night_alert_color(aq.color);
        header_shadow = (header_col.full == color_red().full) ? LV_OPA_COVER : LV_OPA_TRANSP;
    }
    if (objects.container_header_pro) {
        lv_obj_set_style_border_color(objects.container_header_pro, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(objects.container_header_pro, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(objects.container_header_pro, header_shadow, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (objects.container_settings_header) {
        lv_obj_set_style_border_color(objects.container_settings_header, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(objects.container_settings_header, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(objects.container_settings_header, header_shadow, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    update_sensor_cards(aq, gas_warmup, show_co2_bar);
}

void UiController::update_sensor_info_ui() {
    if (info_sensor == INFO_NONE) {
        return;
    }
    switch (info_sensor) {
        case INFO_TEMP: {
            char buf[16];
            if (currentData.temp_valid) {
                float temp_display = currentData.temperature;
                if (!temp_units_c) {
                    temp_display = (temp_display * 9.0f / 5.0f) + 32.0f;
                }
                snprintf(buf, sizeof(buf), "%.1f", temp_display);
            } else {
                strcpy(buf, UiText::ValueMissing());
            }
            safe_label_set_text(objects.label_sensor_value, buf);
            lv_color_t temp_col = currentData.temp_valid ? getTempColor(currentData.temperature) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(temp_col));
            break;
        }
        case INFO_VOC: {
            const bool gas_warmup = sensorManager.isWarmupActive();
            if (gas_warmup) {
                safe_label_set_text(objects.label_sensor_value, "---");
            } else if (currentData.voc_valid) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", currentData.voc_index);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            lv_color_t voc_col = gas_warmup ? color_blue()
                                            : (currentData.voc_valid ? getVOCColor(currentData.voc_index) : color_inactive());
            set_dot_color(objects.dot_sensor_info, gas_warmup ? voc_col : alert_color_for_mode(voc_col));
            break;
        }
        case INFO_NOX: {
            const bool gas_warmup = sensorManager.isWarmupActive();
            if (gas_warmup) {
                safe_label_set_text(objects.label_sensor_value, "---");
            } else if (currentData.nox_valid) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", currentData.nox_index);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            lv_color_t nox_col = gas_warmup ? color_blue()
                                            : (currentData.nox_valid ? getNOxColor(currentData.nox_index) : color_inactive());
            set_dot_color(objects.dot_sensor_info, gas_warmup ? nox_col : alert_color_for_mode(nox_col));
            break;
        }
        case INFO_HCHO: {
            if (currentData.hcho_valid) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", static_cast<int>(lroundf(currentData.hcho)));
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = nullptr;
            if (objects.label_hcho_unit_1) {
                unit = lv_label_get_text(objects.label_hcho_unit_1);
            } else if (objects.label_hcho_unit) {
                unit = lv_label_get_text(objects.label_hcho_unit);
            } else {
                unit = UiText::UnitPpb();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t hcho_col = getHCHOColor(currentData.hcho, currentData.hcho_valid);
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(hcho_col));
            break;
        }
        case INFO_CO2: {
            if (currentData.co2_valid) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", currentData.co2);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = nullptr;
            if (objects.label_co2_unit_1) {
                unit = lv_label_get_text(objects.label_co2_unit_1);
            } else if (objects.label_co2_unit) {
                unit = lv_label_get_text(objects.label_co2_unit);
            } else {
                unit = "ppm";
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t co2_col = currentData.co2_valid ? getCO2Color(currentData.co2) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(co2_col));
            break;
        }
        case INFO_RH: {
            if (currentData.hum_valid) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%.0f", currentData.humidity);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissingShort());
            }
            const char *unit = objects.label_hum_unit
                ? lv_label_get_text(objects.label_hum_unit)
                : "%";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t hum_col = currentData.hum_valid ? getHumidityColor(currentData.humidity) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(hum_col));
            break;
        }
        case INFO_AH: {
            float ah_gm3 = NAN;
            if (currentData.temp_valid && currentData.hum_valid) {
                ah_gm3 = MathUtils::compute_absolute_humidity_gm3(currentData.temperature, currentData.humidity);
            }
            if (isfinite(ah_gm3)) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%.0f", ah_gm3);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissingShort());
            }
            const char *unit = objects.label_ah_unit
                ? lv_label_get_text(objects.label_ah_unit)
                : "g/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t ah_col = getAbsoluteHumidityColor(ah_gm3);
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(ah_col));
            break;
        }
        case INFO_MR: {
            const char *value = UiText::ValueMissingShort();
            if (objects.label_mr_value) {
                value = lv_label_get_text(objects.label_mr_value);
            }
            safe_label_set_text(objects.label_sensor_value, value);

            const char *unit = UiText::UnitIndex();
            if (objects.label_mr_unit) {
                unit = lv_label_get_text(objects.label_mr_unit);
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);

            lv_color_t mr_col = color_inactive();
            if (objects.dot_mr) {
                mr_col = lv_obj_get_style_bg_color(objects.dot_mr, LV_PART_MAIN);
            }
            set_dot_color(objects.dot_sensor_info, mr_col);
            break;
        }
        case INFO_DP: {
            float dew_c = NAN;
            float dew_c_rounded = NAN;
            if (currentData.temp_valid && currentData.hum_valid) {
                dew_c = MathUtils::compute_dew_point_c(currentData.temperature, currentData.humidity);
                if (isfinite(dew_c)) {
                    dew_c_rounded = roundf(dew_c);
                }
            }
            if (isfinite(dew_c)) {
                float dew_display = dew_c;
                if (!temp_units_c) {
                    dew_display = (dew_display * 9.0f / 5.0f) + 32.0f;
                }
                char buf[16];
                snprintf(buf, sizeof(buf), "%.1f", dew_display);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissingShort());
            }
            safe_label_set_text(objects.label_sensor_info_unit, temp_units_c ? UiText::UnitC() : UiText::UnitF());
            float dp_color_c = isfinite(dew_c_rounded) ? dew_c_rounded : dew_c;
            lv_color_t dp_col = getDewPointColor(dp_color_c);
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(dp_col));
            break;
        }
        case INFO_PM25: {
            if (currentData.pm25_valid) {
                char buf[16];
                if (currentData.pm25 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm25);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm25);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = nullptr;
            if (objects.label_pm25_unit_1) {
                unit = lv_label_get_text(objects.label_pm25_unit_1);
            } else if (objects.label_pm25_unit) {
                unit = lv_label_get_text(objects.label_pm25_unit);
            } else {
                unit = "ug/m3";
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm25_col = currentData.pm25_valid ? getPM25Color(currentData.pm25) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(pm25_col));
            break;
        }
        case INFO_PM10: {
            if (currentData.pm10_valid) {
                char buf[16];
                if (currentData.pm10 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm10);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm10);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = nullptr;
            if (objects.label_pm10_unit_pro) {
                unit = lv_label_get_text(objects.label_pm10_unit_pro);
            } else if (objects.label_pm10_unit) {
                unit = lv_label_get_text(objects.label_pm10_unit);
            } else {
                unit = "ug/m3";
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm10_col = currentData.pm10_valid ? getPM10Color(currentData.pm10) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(pm10_col));
            break;
        }
        case INFO_PM1: {
            const bool pm1_available = currentData.pm_valid && isfinite(currentData.pm1) && currentData.pm1 >= 0.0f;
            if (pm1_available) {
                char buf[16];
                if (currentData.pm1 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm1);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm1);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = objects.label_pm1_unit
                ? lv_label_get_text(objects.label_pm1_unit)
                : "ug/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm1_col = color_inactive();
            if (objects.dot_pm1) {
                pm1_col = lv_obj_get_style_bg_color(objects.dot_pm1, LV_PART_MAIN);
            }
            set_dot_color(objects.dot_sensor_info, pm1_col);
            break;
        }
        case INFO_PM4: {
            const bool pm4_available = currentData.pm_valid && isfinite(currentData.pm4) && currentData.pm4 >= 0.0f;
            if (pm4_available) {
                char buf[16];
                if (currentData.pm4 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm4);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm4);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = objects.label_co_unit
                ? lv_label_get_text(objects.label_co_unit)
                : "ug/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm4_col = color_inactive();
            if (objects.dot_co) {
                pm4_col = lv_obj_get_style_bg_color(objects.dot_co, LV_PART_MAIN);
            }
            set_dot_color(objects.dot_sensor_info, pm4_col);
            break;
        }
        case INFO_PRESSURE_3H:
        case INFO_PRESSURE_24H: {
            char buf[16];
            if (currentData.pressure_valid) {
                snprintf(buf, sizeof(buf), "%.0f", currentData.pressure);
            } else {
                strcpy(buf, UiText::ValueMissing());
            }
            safe_label_set_text(objects.label_sensor_value, buf);
            safe_label_set_text(objects.label_pressure_value, buf);

            const char *unit = nullptr;
            if (objects.label_pressure_unit_1) {
                unit = lv_label_get_text(objects.label_pressure_unit_1);
            } else if (objects.label_pressure_unit) {
                unit = lv_label_get_text(objects.label_pressure_unit);
            } else {
                unit = "hPa";
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);

            if (currentData.pressure_delta_3h_valid) {
                if (currentData.pressure_delta_3h > 0.05f) {
                    snprintf(buf, sizeof(buf), "+%.1f", currentData.pressure_delta_3h);
                } else {
                    snprintf(buf, sizeof(buf), "%.1f", currentData.pressure_delta_3h);
                }
            } else {
                strcpy(buf, UiText::ValueMissingShort());
            }
            safe_label_set_text(objects.label_delta_3h_value, buf);
            safe_label_set_text(objects.label_delta_3h_value_1, buf);

            if (currentData.pressure_delta_24h_valid) {
                if (currentData.pressure_delta_24h > 0.05f) {
                    snprintf(buf, sizeof(buf), "+%.1f", currentData.pressure_delta_24h);
                } else {
                    snprintf(buf, sizeof(buf), "%.1f", currentData.pressure_delta_24h);
                }
            } else {
                strcpy(buf, UiText::ValueMissingShort());
            }
            safe_label_set_text(objects.label_delta_24h_value, buf);
            safe_label_set_text(objects.label_delta_24h_value_1, buf);

            lv_color_t delta_3h_color = night_mode
                ? color_card_border()
                : getPressureDeltaColor(currentData.pressure_delta_3h, currentData.pressure_delta_3h_valid, false);
            lv_color_t delta_24h_color = night_mode
                ? color_card_border()
                : getPressureDeltaColor(currentData.pressure_delta_24h, currentData.pressure_delta_24h_valid, true);
            set_chip_color(objects.chip_delta_3h, delta_3h_color);
            set_chip_color(objects.chip_delta_24h, delta_24h_color);
            set_chip_color(objects.chip_delta_3h_1, delta_3h_color);
            set_chip_color(objects.chip_delta_24h_1, delta_24h_color);
            set_dot_color(objects.dot_sensor_info, delta_3h_color);
            break;
        }
        case INFO_NONE:
        default:
            break;
    }
}

void UiController::restore_sensor_info_selection() {
    switch (info_sensor) {
        case INFO_TEMP: {
            hide_all_sensor_info_containers();
            set_visible(objects.temperature_info, true);
            if (objects.label_sensor_info_title) {
                safe_label_set_text(objects.label_sensor_info_title, UiText::SensorInfoTitleTemperature());
            }
            const char *value = nullptr;
            if (objects.label_temp_value_1) {
                value = lv_label_get_text(objects.label_temp_value_1);
            } else if (objects.label_temp_value) {
                value = lv_label_get_text(objects.label_temp_value);
            } else {
                value = UiText::ValueMissing();
            }
            safe_label_set_text(objects.label_sensor_value, value);
            const char *unit = nullptr;
            if (objects.label_temp_unit_1) {
                unit = lv_label_get_text(objects.label_temp_unit_1);
            } else if (objects.label_temp_unit) {
                unit = lv_label_get_text(objects.label_temp_unit);
            } else {
                unit = temp_units_c ? UiText::UnitC() : UiText::UnitF();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            update_sensor_info_ui();
            break;
        }
        case INFO_VOC: {
            hide_all_sensor_info_containers();
            set_visible(objects.voc_info, true);
            if (objects.label_sensor_info_title) {
                safe_label_set_text(objects.label_sensor_info_title, "VOC");
            }
            const char *unit = nullptr;
            if (objects.label_voc_unit_1) {
                unit = lv_label_get_text(objects.label_voc_unit_1);
            } else if (objects.label_voc_unit) {
                unit = lv_label_get_text(objects.label_voc_unit);
            } else {
                unit = UiText::UnitIndex();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            update_sensor_info_ui();
            break;
        }
        case INFO_NOX: {
            hide_all_sensor_info_containers();
            set_visible(objects.nox_info, true);
            if (objects.label_sensor_info_title) {
                safe_label_set_text(objects.label_sensor_info_title, "NOx");
            }
            const char *unit = nullptr;
            if (objects.label_nox_unit_1) {
                unit = lv_label_get_text(objects.label_nox_unit_1);
            } else if (objects.label_nox_unit) {
                unit = lv_label_get_text(objects.label_nox_unit);
            } else {
                unit = UiText::UnitIndex();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            update_sensor_info_ui();
            break;
        }
        case INFO_HCHO: {
            hide_all_sensor_info_containers();
            set_visible(objects.hcho_info, true);
            if (objects.label_sensor_info_title) {
                safe_label_set_text(objects.label_sensor_info_title, UiText::SensorInfoTitleFormaldehyde());
            }
            const char *unit = nullptr;
            if (objects.label_hcho_unit_1) {
                unit = lv_label_get_text(objects.label_hcho_unit_1);
            } else if (objects.label_hcho_unit) {
                unit = lv_label_get_text(objects.label_hcho_unit);
            } else {
                unit = UiText::UnitPpb();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            update_sensor_info_ui();
            break;
        }
        case INFO_CO2: {
            hide_all_sensor_info_containers();
            set_visible(objects.co2_info, true);
            if (objects.label_sensor_info_title) {
                safe_label_set_text(objects.label_sensor_info_title, "CO2");
            }
            const char *unit = nullptr;
            if (objects.label_co2_unit_1) {
                unit = lv_label_get_text(objects.label_co2_unit_1);
            } else if (objects.label_co2_unit) {
                unit = lv_label_get_text(objects.label_co2_unit);
            } else {
                unit = "ppm";
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            update_sensor_info_ui();
            break;
        }
        case INFO_RH:
        case INFO_AH:
        case INFO_MR:
        case INFO_DP:
            select_humidity_info(info_sensor);
            break;
        case INFO_PM25:
        case INFO_PM10:
        case INFO_PM1:
        case INFO_PM4:
            select_pm_info(info_sensor);
            break;
        case INFO_PRESSURE_3H:
        case INFO_PRESSURE_24H:
            select_pressure_info(info_sensor);
            break;
        case INFO_NONE:
        default:
            hide_all_sensor_info_containers();
            break;
    }
}

void UiController::select_humidity_info(InfoSensor sensor) {
    info_sensor = sensor;
    hide_all_sensor_info_containers();

    const bool show_rh_ah = (sensor == INFO_RH) || (sensor == INFO_AH);
    const bool show_mr_dp = (sensor == INFO_MR) || (sensor == INFO_DP);
    set_visible(objects.humidity_info_rh_ah, show_rh_ah);
    set_visible(objects.humidity_info_mr_dp, show_mr_dp);
    set_visible(objects.rh_info, sensor == INFO_RH);
    set_visible(objects.ah_info, sensor == INFO_AH);
    set_visible(objects.mr_info, sensor == INFO_MR);
    set_visible(objects.dp_info, sensor == INFO_DP);

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) return;
        if (checked) lv_obj_add_state(btn, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn, LV_STATE_CHECKED);
    };
    set_checked(objects.btn_rh_info, sensor == INFO_RH);
    set_checked(objects.btn_ah_info, sensor == INFO_AH);
    set_checked(objects.btn_mr_info, sensor == INFO_MR);
    set_checked(objects.btn_dp_info, sensor == INFO_DP);

    if (objects.label_sensor_info_title) {
        if (sensor == INFO_RH) {
            safe_label_set_text(objects.label_sensor_info_title, UiText::SensorInfoTitleRh());
        } else if (sensor == INFO_AH) {
            safe_label_set_text(objects.label_sensor_info_title, UiText::SensorInfoTitleAh());
        } else if (sensor == INFO_MR) {
            safe_label_set_text(objects.label_sensor_info_title, UiText::SensorInfoTitleMr());
        } else if (sensor == INFO_DP) {
            safe_label_set_text(objects.label_sensor_info_title, UiText::SensorInfoTitleDp());
        }
    }
    update_sensor_info_ui();
}

void UiController::select_pm_info(InfoSensor sensor) {
    info_sensor = sensor;
    hide_all_sensor_info_containers();
    set_visible(objects.pm_info, true);
    set_visible(objects.pm25_info, sensor == INFO_PM25);
    set_visible(objects.pm10_info, sensor == INFO_PM10);
    set_visible(objects.pm1_info, sensor == INFO_PM1);
    set_visible(objects.pm4_info, sensor == INFO_PM4);

    if (objects.label_sensor_info_title) {
        if (sensor == INFO_PM25) {
            safe_label_set_text(objects.label_sensor_info_title, "PM2.5");
        } else if (sensor == INFO_PM10) {
            safe_label_set_text(objects.label_sensor_info_title, "PM10");
        } else if (sensor == INFO_PM1) {
            safe_label_set_text(objects.label_sensor_info_title, "PM1");
        } else if (sensor == INFO_PM4) {
            safe_label_set_text(objects.label_sensor_info_title, "PM4");
        }
    }
    update_sensor_info_ui();
}

void UiController::select_pressure_info(InfoSensor sensor) {
    info_sensor = sensor;
    hide_all_sensor_info_containers();
    set_visible(objects.pressure_info, true);
    set_visible(objects.pressure_3h_info, sensor == INFO_PRESSURE_3H);
    set_visible(objects.pressure_24h_info, sensor == INFO_PRESSURE_24H);

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) return;
        if (checked) lv_obj_add_state(btn, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn, LV_STATE_CHECKED);
    };
    set_checked(objects.btn_3h_pressure_info, sensor == INFO_PRESSURE_3H);
    set_checked(objects.btn_24h_pressure_info, sensor == INFO_PRESSURE_24H);

    const char *title = nullptr;
    if (objects.label_pressure_title_1) {
        title = lv_label_get_text(objects.label_pressure_title_1);
    } else if (objects.label_pressure_title) {
        title = lv_label_get_text(objects.label_pressure_title);
    } else {
        title = "PRESSURE";
    }
    safe_label_set_text(objects.label_sensor_info_title, title);

    update_sensor_info_ui();
}

void UiController::set_visible(lv_obj_t *obj, bool visible) {
    ::set_visible(obj, visible);
}

void UiController::hide_all_sensor_info_containers() {
    set_visible(objects.temperature_info, false);
    set_visible(objects.co2_info, false);
    set_visible(objects.voc_info, false);
    set_visible(objects.nox_info, false);
    set_visible(objects.hcho_info, false);
    set_visible(objects.humidity_info_rh_ah, false);
    set_visible(objects.humidity_info_mr_dp, false);
    set_visible(objects.rh_info, false);
    set_visible(objects.ah_info, false);
    set_visible(objects.mr_info, false);
    set_visible(objects.dp_info, false);
    set_visible(objects.pressure_info, false);
    set_visible(objects.pressure_3h_info, false);
    set_visible(objects.pressure_24h_info, false);
    set_visible(objects.pm_info, false);
    set_visible(objects.pm10_info, false);
    set_visible(objects.pm25_info, false);
    set_visible(objects.pm1_info, false);
    set_visible(objects.pm4_info, false);
}

void UiController::update_settings_header() {
    if (!objects.container_settings_header) {
        return;
    }
    AirQuality aq = getAirQuality(currentData);
    lv_color_t header_col;
    lv_opa_t header_shadow;
    compute_header_style(aq, header_col, header_shadow);
    if (night_mode && header_status_enabled) {
        header_col = night_alert_color(aq.color);
        header_shadow = (header_col.full == color_red().full) ? LV_OPA_COVER : LV_OPA_TRANSP;
    }
    lv_obj_set_style_border_color(objects.container_settings_header, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(objects.container_settings_header, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(objects.container_settings_header, header_shadow, LV_PART_MAIN | LV_STATE_DEFAULT);
    sync_night_mode_toggle_ui();
    sync_auto_dim_button_state();
}

Config::Language UiController::next_language(Config::Language current) {
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

void UiController::update_language_label() {
    if (objects.label_language_value) {
        safe_label_set_text(objects.label_language_value, language_label(ui_language));
    }
}

void UiController::update_language_fonts() {
    const bool is_zh = (ui_language == Config::Language::ZH);
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
    };

    for (lv_obj_t *root : roots) {
        replace_font_recursive(root, from_14, to_14);
        replace_font_recursive(root, from_18, to_18);
    }
}

void UiController::update_settings_texts() {
    if (objects.label_settings_title) safe_label_set_text(objects.label_settings_title, UiText::LabelSettingsTitle());
    if (objects.label_btn_back) safe_label_set_text(objects.label_btn_back, UiText::LabelSettingsBack());
    if (objects.label_temp_offset_title) safe_label_set_text(objects.label_temp_offset_title, UiText::LabelTempOffsetTitle());
    if (objects.label_hum_offset_title) safe_label_set_text(objects.label_hum_offset_title, UiText::LabelHumOffsetTitle());
    if (objects.label_btn_night_mode) safe_label_set_text(objects.label_btn_night_mode, UiText::LabelNightMode());
    if (objects.label_btn_units_c_f) safe_label_set_text(objects.label_btn_units_c_f, UiText::LabelUnitsCF());
    if (objects.label_btn_head_status) safe_label_set_text(objects.label_btn_head_status, UiText::LabelHeadStatus());
    if (objects.label_btn_wifi) safe_label_set_text(objects.label_btn_wifi, UiText::LabelWifi());
    if (objects.label_btn_time_date) safe_label_set_text(objects.label_btn_time_date, UiText::LabelTimeDate());
    if (objects.label_btn_theme_color) safe_label_set_text(objects.label_btn_theme_color, UiText::LabelThemeColor());
    if (objects.label_btn_mqtt) safe_label_set_text(objects.label_btn_mqtt, UiText::LabelMqtt());
    if (objects.label_btn_auto_dim) safe_label_set_text(objects.label_btn_auto_dim, UiText::LabelAutoNight());
    if (objects.label_btn_restart) safe_label_set_text(objects.label_btn_restart, UiText::LabelRestart());
    if (objects.label_btn_factory_reset) safe_label_set_text(objects.label_btn_factory_reset, UiText::LabelFactoryReset());
    if (objects.label_btn_co2_calib) safe_label_set_text(objects.label_btn_co2_calib, UiText::LabelCo2Calibration());
    if (objects.label_btn_about) safe_label_set_text(objects.label_btn_about, UiText::LabelAbout());
    if (objects.label_btn_about_back) {
        const char *back_label = UiText::LabelSettingsBack();
        if (ui_language == Language::EN) {
            back_label = "BACK";
        }
        safe_label_set_text(objects.label_btn_about_back, back_label);
    }
    if (objects.label_btn_units_led_indicators) safe_label_set_text(objects.label_btn_units_led_indicators, UiText::LabelLedIndicators());
    if (objects.label_btn_alert_blink) safe_label_set_text(objects.label_btn_alert_blink, UiText::LabelAlertBlink());
    if (objects.label_voc_reset) safe_label_set_text(objects.label_voc_reset, UiText::LabelVocRelearn());
    if (objects.label_btn_head_status_1) safe_label_set_text(objects.label_btn_head_status_1, UiText::LabelBacklight());
}

void UiController::update_main_texts() {
    if (objects.label_status_title_1) safe_label_set_text(objects.label_status_title_1, UiText::LabelStatusTitle());
    if (objects.label_btn_settings_1) safe_label_set_text(objects.label_btn_settings_1, UiText::LabelSettingsButton());
    if (objects.label_temp_title_1) safe_label_set_text(objects.label_temp_title_1, UiText::LabelTemperatureTitle());
    if (objects.label_pressure_title_1) safe_label_set_text(objects.label_pressure_title_1, UiText::LabelPressureTitle());
    if (objects.label_time_title_2) safe_label_set_text(objects.label_time_title_2, UiText::LabelTimeCard());
    if (objects.label_voc_warmup_1) safe_label_set_text(objects.label_voc_warmup_1, UiText::LabelWarmup());
    if (objects.label_nox_warmup_1) safe_label_set_text(objects.label_nox_warmup_1, UiText::LabelWarmup());
    if (objects.label_voc_unit_1) safe_label_set_text(objects.label_voc_unit_1, UiText::UnitIndex());
    if (objects.label_nox_unit_1) safe_label_set_text(objects.label_nox_unit_1, UiText::UnitIndex());
}

void UiController::update_sensor_info_texts() {
    if (objects.label_btn_back_1) safe_label_set_text_static(objects.label_btn_back_1, UiText::LabelSettingsBack());
    if (objects.label_temperature_text) safe_label_set_text_static(objects.label_temperature_text, UiText::InfoTempText());
    if (objects.label_temperature_excellent) safe_label_set_text_static(objects.label_temperature_excellent, UiText::InfoTempExcellent());
    if (objects.label_temperature_acceptable) safe_label_set_text_static(objects.label_temperature_acceptable, UiText::InfoTempAcceptable());
    if (objects.label_temperature_uncomfortable) safe_label_set_text_static(objects.label_temperature_uncomfortable, UiText::InfoTempUncomfortable());
    if (objects.label_temperature_poor) safe_label_set_text_static(objects.label_temperature_poor, UiText::InfoTempPoor());
    if (objects.label_voc_text) safe_label_set_text_static(objects.label_voc_text, UiText::InfoVocText());
    if (objects.label_voc_excellent) safe_label_set_text_static(objects.label_voc_excellent, UiText::InfoVocExcellent());
    if (objects.label_voc_acceptable) safe_label_set_text_static(objects.label_voc_acceptable, UiText::InfoVocAcceptable());
    if (objects.label_voc_uncomfortable) safe_label_set_text_static(objects.label_voc_uncomfortable, UiText::InfoVocUncomfortable());
    if (objects.label_voc_poor) safe_label_set_text_static(objects.label_voc_poor, UiText::InfoVocPoor());
    if (objects.label_nox_text) safe_label_set_text_static(objects.label_nox_text, UiText::InfoNoxText());
    if (objects.label_nox_excellent) safe_label_set_text_static(objects.label_nox_excellent, UiText::InfoNoxExcellent());
    if (objects.label_nox_acceptable) safe_label_set_text_static(objects.label_nox_acceptable, UiText::InfoNoxAcceptable());
    if (objects.label_nox_uncomfortable) safe_label_set_text_static(objects.label_nox_uncomfortable, UiText::InfoNoxUncomfortable());
    if (objects.label_nox_poor) safe_label_set_text_static(objects.label_nox_poor, UiText::InfoNoxPoor());
    if (objects.label_hcho_text) safe_label_set_text_static(objects.label_hcho_text, UiText::InfoHchoText());
    if (objects.label_hcho_excellent) safe_label_set_text_static(objects.label_hcho_excellent, UiText::InfoHchoExcellent());
    if (objects.label_hcho_acceptable) safe_label_set_text_static(objects.label_hcho_acceptable, UiText::InfoHchoAcceptable());
    if (objects.label_hcho_uncomfortable) safe_label_set_text_static(objects.label_hcho_uncomfortable, UiText::InfoHchoUncomfortable());
    if (objects.label_hcho_poor) safe_label_set_text_static(objects.label_hcho_poor, UiText::InfoHchoPoor());
    if (objects.label_co2_text) safe_label_set_text_static(objects.label_co2_text, UiText::InfoCo2Text());
    if (objects.label_co2_excellent) safe_label_set_text_static(objects.label_co2_excellent, UiText::InfoCo2Excellent());
    if (objects.label_co2_acceptable) safe_label_set_text_static(objects.label_co2_acceptable, UiText::InfoCo2Acceptable());
    if (objects.label_co2_uncomfortable) safe_label_set_text_static(objects.label_co2_uncomfortable, UiText::InfoCo2Uncomfortable());
    if (objects.label_co2_poor) safe_label_set_text_static(objects.label_co2_poor, UiText::InfoCo2Poor());
    if (objects.label_pm25_text) safe_label_set_text_static(objects.label_pm25_text, UiText::InfoPm25Text());
    if (objects.label_pm25_excellent) safe_label_set_text_static(objects.label_pm25_excellent, UiText::InfoPm25Excellent());
    if (objects.label_pm25_acceptable) safe_label_set_text_static(objects.label_pm25_acceptable, UiText::InfoPm25Acceptable());
    if (objects.label_pm25_uncomfortable) safe_label_set_text_static(objects.label_pm25_uncomfortable, UiText::InfoPm25Uncomfortable());
    if (objects.label_pm25_poor) safe_label_set_text_static(objects.label_pm25_poor, UiText::InfoPm25Poor());
    if (objects.label_pm10_text) safe_label_set_text_static(objects.label_pm10_text, UiText::InfoPm10Text());
    if (objects.label_pm10_excellent) safe_label_set_text_static(objects.label_pm10_excellent, UiText::InfoPm10Excellent());
    if (objects.label_pm10_acceptable) safe_label_set_text_static(objects.label_pm10_acceptable, UiText::InfoPm10Acceptable());
    if (objects.label_pm10_uncomfortable) safe_label_set_text_static(objects.label_pm10_uncomfortable, UiText::InfoPm10Uncomfortable());
    if (objects.label_pm10_poor) safe_label_set_text_static(objects.label_pm10_poor, UiText::InfoPm10Poor());
    if (objects.label_pm1_text) safe_label_set_text_static(objects.label_pm1_text, UiText::InfoPm1Text());
    if (objects.label_pm1_excellent) safe_label_set_text_static(objects.label_pm1_excellent, UiText::InfoPm1Excellent());
    if (objects.label_pm1_acceptable) safe_label_set_text_static(objects.label_pm1_acceptable, UiText::InfoPm1Acceptable());
    if (objects.label_pm1_uncomfortable) safe_label_set_text_static(objects.label_pm1_uncomfortable, UiText::InfoPm1Uncomfortable());
    if (objects.label_pm1_poor) safe_label_set_text_static(objects.label_pm1_poor, UiText::InfoPm1Poor());
    if (objects.label_pm4_text) safe_label_set_text_static(objects.label_pm4_text, UiText::InfoPm4Text());
    if (objects.label_pm4_excellent) safe_label_set_text_static(objects.label_pm4_excellent, UiText::InfoPm4Excellent());
    if (objects.label_pm4_acceptable) safe_label_set_text_static(objects.label_pm4_acceptable, UiText::InfoPm4Acceptable());
    if (objects.label_pm4_uncomfortable) safe_label_set_text_static(objects.label_pm4_uncomfortable, UiText::InfoPm4Uncomfortable());
    if (objects.label_pm4_poor) safe_label_set_text_static(objects.label_pm4_poor, UiText::InfoPm4Poor());
    if (objects.label_3h_pressure_text) safe_label_set_text_static(objects.label_3h_pressure_text, UiText::InfoPressure3hText());
    if (objects.label_3h_pressure_excellent) safe_label_set_text_static(objects.label_3h_pressure_excellent, UiText::InfoPressure3hExcellent());
    if (objects.label_3h_pressure_acceptable) safe_label_set_text_static(objects.label_3h_pressure_acceptable, UiText::InfoPressure3hAcceptable());
    if (objects.label_3h_pressure_uncomfortable) safe_label_set_text_static(objects.label_3h_pressure_uncomfortable, UiText::InfoPressure3hUncomfortable());
    if (objects.label_3h_pressure_poor) safe_label_set_text_static(objects.label_3h_pressure_poor, UiText::InfoPressure3hPoor());
    if (objects.label_24h_pressure_text) safe_label_set_text_static(objects.label_24h_pressure_text, UiText::InfoPressure24hText());
    if (objects.label_24h_pressure_excellent) safe_label_set_text_static(objects.label_24h_pressure_excellent, UiText::InfoPressure24hExcellent());
    if (objects.label_24h_pressure_acceptable) safe_label_set_text_static(objects.label_24h_pressure_acceptable, UiText::InfoPressure24hAcceptable());
    if (objects.label_24h_pressure_uncomfortable) safe_label_set_text_static(objects.label_24h_pressure_uncomfortable, UiText::InfoPressure24hUncomfortable());
    if (objects.label_24h_pressure_poor) safe_label_set_text_static(objects.label_24h_pressure_poor, UiText::InfoPressure24hPoor());
    if (objects.label_rh_text) safe_label_set_text_static(objects.label_rh_text, UiText::InfoRhText());
    if (objects.label_rh_excellent) safe_label_set_text_static(objects.label_rh_excellent, UiText::InfoRhExcellent());
    if (objects.label_rh_acceptable) safe_label_set_text_static(objects.label_rh_acceptable, UiText::InfoRhAcceptable());
    if (objects.label_rh_uncomfortable) safe_label_set_text_static(objects.label_rh_uncomfortable, UiText::InfoRhUncomfortable());
    if (objects.label_rh_poor) safe_label_set_text_static(objects.label_rh_poor, UiText::InfoRhPoor());
    if (objects.label_ah_text) safe_label_set_text_static(objects.label_ah_text, UiText::InfoAhText());
    if (objects.label_ah_excellent) safe_label_set_text_static(objects.label_ah_excellent, UiText::InfoAhExcellent());
    if (objects.label_ah_acceptable) safe_label_set_text_static(objects.label_ah_acceptable, UiText::InfoAhAcceptable());
    if (objects.label_ah_uncomfortable) safe_label_set_text_static(objects.label_ah_uncomfortable, UiText::InfoAhUncomfortable());
    if (objects.label_ah_poor) safe_label_set_text_static(objects.label_ah_poor, UiText::InfoAhPoor());
    if (objects.label_rh_text_1) safe_label_set_text_static(objects.label_rh_text_1, UiText::InfoMrText());
    if (objects.label_rh_excellent_1) safe_label_set_text_static(objects.label_rh_excellent_1, UiText::InfoMrExcellent());
    if (objects.label_rh_acceptable_1) safe_label_set_text_static(objects.label_rh_acceptable_1, UiText::InfoMrAcceptable());
    if (objects.label_rh_uncomfortable_1) safe_label_set_text_static(objects.label_rh_uncomfortable_1, UiText::InfoMrUncomfortable());
    if (objects.label_rh_poor_1) safe_label_set_text_static(objects.label_rh_poor_1, UiText::InfoMrPoor());
    if (objects.label_dp_text_1) safe_label_set_text_static(objects.label_dp_text_1, UiText::InfoDpText());
    if (objects.label_dp_excellent_1) safe_label_set_text_static(objects.label_dp_excellent_1, UiText::InfoDpExcellent());
    if (objects.label_dp_acceptable_1) safe_label_set_text_static(objects.label_dp_acceptable_1, UiText::InfoDpAcceptable());
    if (objects.label_dp_uncomfortable_1) safe_label_set_text_static(objects.label_dp_uncomfortable_1, UiText::InfoDpUncomfortable());
    if (objects.label_dp_poor_1) safe_label_set_text_static(objects.label_dp_poor_1, UiText::InfoDpPoor());
}

void UiController::update_confirm_texts() {
    if (objects.label_btn_confirm_voc) safe_label_set_text(objects.label_btn_confirm_voc, UiText::LabelConfirmVocButton());
    if (objects.label_btn_confirm_restart) safe_label_set_text(objects.label_btn_confirm_restart, UiText::LabelConfirmRestartButton());
    if (objects.label_btn_confirm_reset) safe_label_set_text(objects.label_btn_confirm_reset, UiText::LabelConfirmResetButton());
    if (objects.label_btn_confirm_cancel) safe_label_set_text(objects.label_btn_confirm_cancel, UiText::LabelConfirmCancelButton());
    if (objects.label_confirm_title_voc) safe_label_set_text(objects.label_confirm_title_voc, UiText::LabelConfirmTitleVoc());
    if (objects.container_confirm_voc_text) safe_label_set_text(objects.container_confirm_voc_text, UiText::LabelConfirmTextVoc());
    if (objects.label_confirm_title_restart) safe_label_set_text(objects.label_confirm_title_restart, UiText::LabelConfirmTitleRestart());
    if (objects.container_confirm_restart_text) safe_label_set_text(objects.container_confirm_restart_text, UiText::LabelConfirmTextRestart());
    if (objects.label_confirm_title_reset) safe_label_set_text(objects.label_confirm_title_reset, UiText::LabelConfirmTitleReset());
    if (objects.container_confirm_reset_text) safe_label_set_text(objects.container_confirm_reset_text, UiText::LabelConfirmTextReset());
}

void UiController::update_theme_texts() {
    if (objects.label_theme_title) safe_label_set_text(objects.label_theme_title, UiText::LabelThemeTitle());
    if (objects.label_btn_theme_back) safe_label_set_text(objects.label_btn_theme_back, UiText::LabelSettingsBack());
    if (objects.label_btn_theme_custom) safe_label_set_text(objects.label_btn_theme_custom, UiText::LabelThemeCustom());
    if (objects.label_btn_theme_presets) safe_label_set_text(objects.label_btn_theme_presets, UiText::LabelThemePresets());
    if (objects.label_theme_preview_title) safe_label_set_text(objects.label_theme_preview_title, UiText::LabelThemeExample());
    if (objects.label_theme_custom_text) safe_label_set_text(objects.label_theme_custom_text, UiText::LabelThemeCustomInfo());
    if (objects.label_theme_preview_hum_title) safe_label_set_text(objects.label_theme_preview_hum_title, UiText::LabelHumidityTitle());
}

void UiController::update_auto_night_texts() {
    if (objects.label_auto_night_title) safe_label_set_text(objects.label_auto_night_title, UiText::LabelAutoNightTitle());
    if (objects.label_btn_auto_night_back) safe_label_set_text(objects.label_btn_auto_night_back, UiText::LabelSettingsBack());
    if (objects.label_auto_night_hint) safe_label_set_text(objects.label_auto_night_hint, UiText::LabelAutoNightHint());
    if (objects.label_auto_night_start_title) safe_label_set_text(objects.label_auto_night_start_title, UiText::LabelAutoNightStartTitle());
    if (objects.label_auto_night_end_title) safe_label_set_text(objects.label_auto_night_end_title, UiText::LabelAutoNightEndTitle());
    if (objects.label_auto_night_start_hours) safe_label_set_text(objects.label_auto_night_start_hours, UiText::LabelSetTimeHours());
    if (objects.label_auto_night_start_minutes) safe_label_set_text(objects.label_auto_night_start_minutes, UiText::LabelSetTimeMinutes());
    if (objects.label_auto_night_end_hours) safe_label_set_text(objects.label_auto_night_end_hours, UiText::LabelSetTimeHours());
    if (objects.label_auto_night_end_minutes) safe_label_set_text(objects.label_auto_night_end_minutes, UiText::LabelSetTimeMinutes());
    if (objects.label_btn_auto_night_toggle) safe_label_set_text(objects.label_btn_auto_night_toggle, UiText::MqttToggleLabel());
}

void UiController::update_backlight_texts() {
    if (objects.label_backlight_title) safe_label_set_text(objects.label_backlight_title, UiText::LabelBacklightTitle());
    if (objects.label_backlight_hint) safe_label_set_text(objects.label_backlight_hint, UiText::LabelBacklightHint());
    if (objects.label_backlight_schedule_title) safe_label_set_text(objects.label_backlight_schedule_title, UiText::LabelBacklightScheduleTitle());
    if (objects.label_backlight_presets_title) safe_label_set_text(objects.label_backlight_presets_title, UiText::LabelBacklightPresetsTitle());
    if (objects.label_backlight_sleep_title) safe_label_set_text(objects.label_backlight_sleep_title, UiText::LabelBacklightSleepTitle());
    if (objects.label_backlight_wake_title) safe_label_set_text(objects.label_backlight_wake_title, UiText::LabelBacklightWakeTitle());
    if (objects.label_backlight_sleep_hours) safe_label_set_text(objects.label_backlight_sleep_hours, UiText::LabelSetTimeHours());
    if (objects.label_backlight_sleep_minutes) safe_label_set_text(objects.label_backlight_sleep_minutes, UiText::LabelSetTimeMinutes());
    if (objects.label_backlight_wake_hours) safe_label_set_text(objects.label_backlight_wake_hours, UiText::LabelSetTimeHours());
    if (objects.label_backlight_wake_minutes) safe_label_set_text(objects.label_backlight_wake_minutes, UiText::LabelSetTimeMinutes());
    if (objects.label_btn_backlight_back) safe_label_set_text(objects.label_btn_backlight_back, UiText::LabelSettingsBack());
    if (objects.label_btn_backlight_schedule_toggle) safe_label_set_text(objects.label_btn_backlight_schedule_toggle, UiText::MqttToggleLabel());
    if (objects.label_btn_backlight_always_on) safe_label_set_text(objects.label_btn_backlight_always_on, UiText::LabelBacklightAlwaysOn());
    if (objects.label_btn_backlight_30s) safe_label_set_text(objects.label_btn_backlight_30s, UiText::LabelBacklight30s());
    if (objects.label_btn_backlight_1m) safe_label_set_text(objects.label_btn_backlight_1m, UiText::LabelBacklight1m());
    if (objects.label_btn_backlight_5m) safe_label_set_text(objects.label_btn_backlight_5m, UiText::LabelBacklight5m());
}

void UiController::update_co2_calib_texts() {
    if (objects.label_co2_calib_title) safe_label_set_text(objects.label_co2_calib_title, UiText::LabelCo2CalibTitle());
    if (objects.label_btn_co2_calib_back) safe_label_set_text(objects.label_btn_co2_calib_back, UiText::LabelSettingsBack());
    if (objects.label_btn_co2_calib_start) safe_label_set_text(objects.label_btn_co2_calib_start, UiText::LabelCo2CalibStart());
    if (objects.label_co2_calib_asc_text) safe_label_set_text(objects.label_co2_calib_asc_text, UiText::LabelCo2CalibAscInfo());
    if (objects.label_co2_calib_fresh_text) safe_label_set_text(objects.label_co2_calib_fresh_text, UiText::LabelCo2CalibFreshInfo());
}

void UiController::update_boot_diag_texts() {
    if (objects.label_btn_diag_continue) safe_label_set_text(objects.label_btn_diag_continue, UiText::LabelBootTapToContinue());
    if (objects.lbl_diag_title) safe_label_set_text(objects.lbl_diag_title, UiText::LabelBootDiagTitle());
    if (objects.lbl_diag_system_title) safe_label_set_text(objects.lbl_diag_system_title, UiText::LabelBootDiagSystemTitle());
    if (objects.lbl_diag_sensors_title) safe_label_set_text(objects.lbl_diag_sensors_title, UiText::LabelBootDiagSensorsTitle());
    if (objects.lbl_diag_app_label) safe_label_set_text(objects.lbl_diag_app_label, UiText::LabelBootDiagAppLabel());
    if (objects.lbl_diag_mac_label) safe_label_set_text(objects.lbl_diag_mac_label, UiText::LabelBootDiagMacLabel());
    if (objects.lbl_diag_reason_label) safe_label_set_text(objects.lbl_diag_reason_label, UiText::LabelBootDiagResetLabel());
    if (objects.lbl_diag_heap_label) safe_label_set_text(objects.lbl_diag_heap_label, UiText::LabelBootDiagHeapLabel());
    if (objects.lbl_diag_storage_label) safe_label_set_text(objects.lbl_diag_storage_label, UiText::LabelBootDiagStorageLabel());
    if (objects.lbl_diag_i2c_label) safe_label_set_text(objects.lbl_diag_i2c_label, UiText::LabelBootDiagI2cLabel());
    if (objects.lbl_diag_touch_label) safe_label_set_text(objects.lbl_diag_touch_label, UiText::LabelBootDiagTouchLabel());
    if (objects.lbl_diag_sen_label) safe_label_set_text(objects.lbl_diag_sen_label, UiText::LabelBootDiagSenLabel());
    if (objects.lbl_diag_sfa_label) safe_label_set_text(objects.lbl_diag_sfa_label, UiText::LabelBootDiagSfaLabel());
    if (objects.lbl_diag_rtc_label) safe_label_set_text(objects.lbl_diag_rtc_label, UiText::LabelBootDiagRtcLabel());
    if (objects.lbl_diag_error) safe_label_set_text(objects.lbl_diag_error, UiText::LabelBootDiagErrorsDetected());
}

void UiController::update_theme_custom_info(bool presets) {
    set_visible(objects.container_theme_custom_info, !presets);
    if (!presets && objects.qrcode_theme_custom) {
        lv_qrcode_update(objects.qrcode_theme_custom, UiText::ThemePortalUrl(), strlen(UiText::ThemePortalUrl()));
    }
}

void UiController::update_status_message(uint32_t now_ms, bool gas_warmup) {
    StatusMessages::StatusMessageResult result = StatusMessages::build_status_messages(currentData, gas_warmup);
    const StatusMessages::StatusMessage *messages = result.messages;
    const size_t count = result.count;
    const bool has_valid = result.has_valid;

    uint32_t signature = static_cast<uint32_t>(count);
    for (size_t i = 0; i < count; ++i) {
        signature = signature * 131u + (static_cast<uint32_t>(messages[i].sensor) << 2) +
                    static_cast<uint32_t>(messages[i].severity);
    }
    if (signature != status_msg_signature) {
        status_msg_signature = signature;
        status_msg_index = 0;
        status_msg_last_ms = now_ms;
    }

    status_msg_count = static_cast<uint8_t>(count);

    if (count > 1 && (now_ms - status_msg_last_ms) >= STATUS_ROTATE_MS) {
        status_msg_index = static_cast<uint8_t>((status_msg_index + 1) % count);
        status_msg_last_ms = now_ms;
    }
    if (status_msg_index >= count) {
        status_msg_index = 0;
    }

    const char *status_text = nullptr;
    if (!has_valid) {
        status_text = UiText::StatusInitializing();
    } else if (count == 0) {
        status_text = UiText::StatusAllGood();
    } else {
        status_text = messages[status_msg_index].text;
    }

    if (objects.label_status_value_1) {
        safe_label_set_text(objects.label_status_value_1, status_text ? status_text : UiText::ValueMissing());
    }
}

void UiController::init_ui_defaults() {
    if (objects.co2_bar_mask) {
        lv_obj_clear_flag(objects.co2_bar_mask, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_clear_flag(objects.co2_bar_mask, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.co2_bar_mask_1) {
        lv_obj_clear_flag(objects.co2_bar_mask_1, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_clear_flag(objects.co2_bar_mask_1, LV_OBJ_FLAG_SCROLLABLE);
    }

    set_visible(objects.container_about, false);

    if (objects.wifi_status_icon) lv_obj_add_flag(objects.wifi_status_icon, LV_OBJ_FLAG_HIDDEN);
    if (objects.wifi_status_icon_1) lv_obj_add_flag(objects.wifi_status_icon_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.wifi_status_icon_2) lv_obj_add_flag(objects.wifi_status_icon_2, LV_OBJ_FLAG_HIDDEN);
    if (objects.wifi_status_icon_3) lv_obj_add_flag(objects.wifi_status_icon_3, LV_OBJ_FLAG_HIDDEN);
    if (objects.wifi_status_icon_4) lv_obj_add_flag(objects.wifi_status_icon_4, LV_OBJ_FLAG_HIDDEN);
    if (objects.mqtt_status_icon) lv_obj_add_flag(objects.mqtt_status_icon, LV_OBJ_FLAG_HIDDEN);
    if (objects.mqtt_status_icon_1) lv_obj_add_flag(objects.mqtt_status_icon_1, LV_OBJ_FLAG_HIDDEN);
    if (objects.mqtt_status_icon_2) lv_obj_add_flag(objects.mqtt_status_icon_2, LV_OBJ_FLAG_HIDDEN);
    if (objects.mqtt_status_icon_3) lv_obj_add_flag(objects.mqtt_status_icon_3, LV_OBJ_FLAG_HIDDEN);
    if (objects.mqtt_status_icon_4) lv_obj_add_flag(objects.mqtt_status_icon_4, LV_OBJ_FLAG_HIDDEN);

    if (objects.btn_mqtt) {
        lv_obj_set_style_bg_color(objects.btn_mqtt, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_border_color(objects.btn_mqtt, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_shadow_color(objects.btn_mqtt, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
    }
    if (objects.label_btn_mqtt) {
        lv_obj_set_style_text_color(objects.label_btn_mqtt, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
    }
    if (objects.btn_night_mode) {
        lv_obj_set_style_bg_color(objects.btn_night_mode, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_border_color(objects.btn_night_mode, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_shadow_color(objects.btn_night_mode, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
    }
    if (objects.label_btn_night_mode) {
        lv_obj_set_style_text_color(objects.label_btn_night_mode, color_inactive(), LV_PART_MAIN | LV_STATE_DISABLED);
    }

    ui_language = storage.config().language;
    language_dirty = false;
    UiStrings::setLanguage(ui_language);
    update_language_label();
    update_settings_texts();
    update_main_texts();
    update_sensor_info_texts();
    update_confirm_texts();
    update_wifi_texts();
    update_mqtt_texts();
    update_datetime_texts();
    update_theme_texts();
    update_auto_night_texts();
    update_backlight_texts();
    update_co2_calib_texts();
    update_boot_diag_texts();
    update_language_fonts();

    update_clock_labels();
    timeManager.syncInputsFromSystem(set_hour, set_minute, set_day, set_month, set_year);
    update_datetime_ui();
    backlightManager.updateUi();
    nightModeManager.updateUi();
    update_led_indicators();
    update_temp_offset_label();
    update_hum_offset_label();
    update_wifi_ui();
    update_mqtt_ui();
    update_ui();
    confirm_hide();
}
