// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"
#include "ui/UiLocalization.h"
#include "ui/UiRenderLoop.h"
#include "ui/UiScreenFlow.h"
#include "ui/UiText.h"

#include <math.h>
#include <string.h>
#include <time.h>

#include "lvgl_v8_port.h"
#include "ui/ui.h"
#include "ui/styles.h"
#include "ui/images.h"
#include "ui/StatusMessages.h"
#include "web/WebHandlers.h"
#include "config/AppConfig.h"
#include "core/Logger.h"
#include "modules/StorageManager.h"
#include "modules/NetworkManager.h"
#include "modules/MqttManager.h"
#include "modules/SensorManager.h"
#include "modules/TimeManager.h"
#include "ui/ThemeManager.h"
#include "ui/BacklightManager.h"
#include "ui/NightModeManager.h"
#include "ui/UiEventBinder.h"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

using namespace Config;

namespace {

constexpr uint32_t STATUS_ROTATE_MS = 5000;
constexpr int UI_LVGL_LOCK_TIMEOUT_MS = 500;
constexpr uint32_t UI_LVGL_LOCK_WARN_MS = 60000;
constexpr uint16_t UI_LVGL_LOCK_WARN_FAIL_STREAK = 3;

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

int score_from_co(float co_ppm) {
    if (co_ppm <= 0.0f) {
        return 0;
    }
    if (co_ppm < 9.0f) {
        return static_cast<int>(lroundf(map_float_clamped(co_ppm, 0.0f, 9.0f, 0.0f, 25.0f)));
    }
    if (co_ppm <= 35.0f) {
        return static_cast<int>(lroundf(map_float_clamped(co_ppm, 9.0f, 35.0f, 80.0f, 90.0f)));
    }
    if (co_ppm <= 100.0f) {
        return static_cast<int>(lroundf(map_float_clamped(co_ppm, 35.0f, 100.0f, 90.0f, 100.0f)));
    }
    return 100;
}

bool should_wake_backlight_on_alert(const SensorData &data, bool gas_warmup) {
    StatusMessages::StatusMessageResult result = StatusMessages::build_status_messages(data, gas_warmup);
    for (size_t i = 0; i < result.count; ++i) {
        const StatusMessages::StatusMessage &msg = result.messages[i];
        if (msg.sensor == StatusMessages::STATUS_SENSOR_CO &&
            msg.severity >= StatusMessages::STATUS_YELLOW) {
            return true;
        }
        if (msg.severity == StatusMessages::STATUS_RED) {
            return true;
        }
    }
    return false;
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

void UiController::bind_available_events(int screen_id) {
    UiEventBinder::bindAvailableEvents(*this, screen_id);
}

void UiController::apply_toggle_styles_for_available_objects(int screen_id) {
    UiEventBinder::applyToggleStylesForAvailableObjects(*this, screen_id);
}

void UiController::apply_checked_states_for_available_objects(int screen_id) {
    UiEventBinder::applyCheckedStatesForAvailableObjects(*this, screen_id);
}

void UiController::init_theme_controls_if_available() {
    UiEventBinder::initThemeControlsIfAvailable(*this);
}

void UiController::bind_screen_events_once(int screen_id) {
    if (screen_id <= 0 || screen_id >= static_cast<int>(kScreenSlotCount)) {
        return;
    }
    if (!UiEventBinder::screenRootById(screen_id)) {
        return;
    }
    if (screen_events_bound_[screen_id]) {
        return;
    }

    bind_available_events(screen_id);
    apply_toggle_styles_for_available_objects(screen_id);
    apply_checked_states_for_available_objects(screen_id);
    refresh_texts_for_screen(screen_id);
    init_theme_controls_if_available();
    if (screen_id == SCREEN_ID_PAGE_SENSORS_INFO) {
        restore_sensor_info_selection();
    }

    screen_events_bound_[screen_id] = true;
}

void UiController::refresh_texts_for_screen(int screen_id) {
    UiLocalization::refreshTextsForScreen(*this, screen_id);
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
    lvgl_lock_fail_streak = 0;
    last_lvgl_lock_warn_ms = 0;
    boot_release_at_ms = 0;
    boot_ui_released = false;
    deferred_unload_.reset();
    wifi_icon_state = -1;
    mqtt_icon_state = -1;
    wifi_icon_state_main = -1;
    mqtt_icon_state_main = -1;
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

void UiController::onSensorPoll(const SensorManager::PollResult &poll) {
    backlightManager.setAlarmWakeActive(
        should_wake_backlight_on_alert(currentData, sensorManager.isWarmupActive()));
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
    // UI state is refreshed in UiRenderLoop under lvgl_port_lock.
}

void UiController::poll(uint32_t now) {
    backlightManager.setAlarmWakeActive(
        should_wake_backlight_on_alert(currentData, sensorManager.isWarmupActive()));
    bool desired = false;
    if (nightModeManager.poll(night_mode, desired)) {
        set_night_mode_state(desired, true);
    }
    if (now - last_clock_tick_ms >= CLOCK_TICK_MS) {
        last_clock_tick_ms = now;
        clock_ui_dirty = true;
        if (current_screen_id == SCREEN_ID_PAGE_CLOCK && !datetime_changed) {
            datetime_ui_dirty = true;
        }
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

    if (!lvgl_port_lock(UI_LVGL_LOCK_TIMEOUT_MS)) {
        if (lvgl_lock_fail_streak != 0xFFFFu) {
            ++lvgl_lock_fail_streak;
        }
        if (lvgl_lock_fail_streak >= UI_LVGL_LOCK_WARN_FAIL_STREAK &&
            (now - last_lvgl_lock_warn_ms) >= UI_LVGL_LOCK_WARN_MS) {
            last_lvgl_lock_warn_ms = now;
            LOGW("UI", "LVGL lock timeout in poll (screen=%d, backlight=%s, streak=%u)",
                 current_screen_id,
                 backlightManager.isOn() ? "ON" : "OFF",
                 static_cast<unsigned>(lvgl_lock_fail_streak));
        }
        return;
    }
    lvgl_lock_fail_streak = 0;
    mqtt_apply_pending();
    if ((now - last_ui_tick_ms) >= UI_TICK_MS) {
        ui_tick();
        last_ui_tick_ms = now;
    }
    backlightManager.poll(lvgl_ready);
    update_status_icons();
    UiScreenFlow::processPendingScreen(*this, now);
    UiScreenFlow::processBootRelease(*this, now);
    UiScreenFlow::processDeferredUnloads(*this, now);

    UiRenderLoop::process(*this, now);
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

lv_color_t UiController::getCOColor(float co_ppm) {
    if (co_ppm < 9.0f) return color_green();
    if (co_ppm <= 35.0f) return color_yellow();
    if (co_ppm <= 100.0f) return color_orange();
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

lv_color_t UiController::getPM1Color(float pm) {
    if (pm <= 10.0f) return color_green();
    if (pm <= 25.0f) return color_yellow();
    if (pm <= 50.0f) return color_orange();
    return color_red();
}

lv_color_t UiController::getPM4Color(float pm) {
    if (pm <= 25.0f) return color_green();
    if (pm <= 50.0f) return color_yellow();
    if (pm <= 75.0f) return color_orange();
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

    if (data.co_sensor_present &&
        data.co_valid &&
        isfinite(data.co_ppm) &&
        data.co_ppm >= 0.0f) {
        int score = score_from_co(data.co_ppm);
        max_score = max(max_score, score);
        has_valid = true;
    }

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

void UiController::compute_header_style(const AirQuality &aq,
                                        uint8_t status_severity,
                                        bool co_alert_active,
                                        lv_color_t &color,
                                        lv_opa_t &shadow_opa) {
    (void)aq;

    lv_color_t base = color_card_border();
    if (header_status_enabled) {
        if (status_severity >= static_cast<uint8_t>(StatusMessages::STATUS_RED)) {
            base = color_red();
        } else if (status_severity >= static_cast<uint8_t>(StatusMessages::STATUS_ORANGE)) {
            base = color_orange();
        } else if (status_severity >= static_cast<uint8_t>(StatusMessages::STATUS_YELLOW)) {
            base = color_yellow();
        } else {
            // "All good" stays green by design.
            base = color_green();
        }
    }
    if (co_alert_active) {
        base = color_red();
    }
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
    if (date_units_mdy) {
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d",
                 local_tm.tm_mon + 1,
                 local_tm.tm_mday,
                 local_tm.tm_year + 1900);
    } else {
        snprintf(buf, sizeof(buf), "%02d.%02d.%04d",
                 local_tm.tm_mday,
                 local_tm.tm_mon + 1,
                 local_tm.tm_year + 1900);
    }
    if (objects.label_date_value_1) safe_label_set_text(objects.label_date_value_1, buf);
    if (objects.label_date_value_2) safe_label_set_text(objects.label_date_value_2, buf);
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

void UiController::sync_backlight_settings_button_state() {
    if (!objects.btn_head_status_1) {
        return;
    }
    if (backlightManager.isScheduleEnabled()) {
        lv_obj_add_state(objects.btn_head_status_1, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(objects.btn_head_status_1, LV_STATE_CHECKED);
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
    compute_header_style(aq, status_max_severity, co_status_alert_active, header_col, header_shadow);
    if (night_mode && header_status_enabled) {
        const bool status_red =
            status_max_severity >= static_cast<uint8_t>(StatusMessages::STATUS_RED);
        header_col = (co_status_alert_active || status_red) ? color_red() : color_inactive();
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

void UiController::update_settings_header() {
    if (!objects.container_settings_header) {
        return;
    }
    AirQuality aq = getAirQuality(currentData);
    bool co_alert_active = false;
    uint8_t status_severity = static_cast<uint8_t>(StatusMessages::STATUS_NONE);
    compute_status_summary(sensorManager.isWarmupActive(), co_alert_active, status_severity);
    lv_color_t header_col;
    lv_opa_t header_shadow;
    compute_header_style(aq, status_severity, co_alert_active, header_col, header_shadow);
    if (night_mode && header_status_enabled) {
        const bool status_red =
            status_severity >= static_cast<uint8_t>(StatusMessages::STATUS_RED);
        header_col = (co_alert_active || status_red) ? color_red() : color_inactive();
        header_shadow = (header_col.full == color_red().full) ? LV_OPA_COVER : LV_OPA_TRANSP;
    }
    lv_obj_set_style_border_color(objects.container_settings_header, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(objects.container_settings_header, header_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(objects.container_settings_header, header_shadow, LV_PART_MAIN | LV_STATE_DEFAULT);
    sync_night_mode_toggle_ui();
    sync_auto_dim_button_state();
    sync_backlight_settings_button_state();
}

void UiController::update_theme_custom_info(bool presets) {
    set_visible(objects.container_theme_custom_info, !presets);
    if (!presets && objects.qrcode_theme_custom) {
        lv_qrcode_update(objects.qrcode_theme_custom, UiText::ThemePortalUrl(), strlen(UiText::ThemePortalUrl()));
    }
}

void UiController::compute_status_summary(bool gas_warmup,
                                          bool &co_alert_active,
                                          uint8_t &max_severity) const {
    co_alert_active = false;
    max_severity = static_cast<uint8_t>(StatusMessages::STATUS_NONE);

    StatusMessages::StatusMessageResult result =
        StatusMessages::build_status_messages(currentData, gas_warmup);
    for (size_t i = 0; i < result.count; ++i) {
        const StatusMessages::StatusMessage &msg = result.messages[i];
        if (msg.sensor == StatusMessages::STATUS_SENSOR_CO) {
            co_alert_active = true;
        }
        const uint8_t sev = static_cast<uint8_t>(msg.severity);
        if (sev > max_severity) {
            max_severity = sev;
        }
    }
}

void UiController::update_status_message(uint32_t now_ms, bool gas_warmup) {
    StatusMessages::StatusMessageResult result = StatusMessages::build_status_messages(currentData, gas_warmup);
    const StatusMessages::StatusMessage *messages = result.messages;
    const size_t count = result.count;
    const bool has_valid = result.has_valid;
    co_status_alert_active = false;
    status_max_severity = static_cast<uint8_t>(StatusMessages::STATUS_NONE);
    for (size_t i = 0; i < count; ++i) {
        const uint8_t sev = static_cast<uint8_t>(messages[i].severity);
        if (sev > status_max_severity) {
            status_max_severity = sev;
        }
        if (messages[i].sensor == StatusMessages::STATUS_SENSOR_CO) {
            co_status_alert_active = true;
        }
    }

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
    date_units_mdy = storage.config().units_mdy;
    language_dirty = false;
    header_status_enabled = storage.config().header_status_enabled;
    UiLocalization::applyCurrentLanguage(*this);

    update_clock_labels();
    timeManager.syncInputsFromSystem(set_hour, set_minute, set_day, set_month, set_year);
    update_datetime_ui();
    backlightManager.updateUi();
    nightModeManager.updateUi();
    sync_backlight_settings_button_state();
    update_led_indicators();
    update_temp_offset_label();
    update_hum_offset_label();
    update_wifi_ui();
    update_mqtt_ui();
    update_ui();
    confirm_hide();
}


