// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"

#include <stdio.h>

#include "modules/FanControl.h"
#include "modules/StorageManager.h"
#include "ui/ui.h"

namespace {

void set_checked_state(lv_obj_t *obj, bool checked) {
    if (!obj) {
        return;
    }
    if (checked) {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(obj, LV_STATE_CHECKED);
    }
}

void set_button_accent(lv_obj_t *obj, lv_color_t color, lv_opa_t shadow_opa) {
    if (!obj) {
        return;
    }
    lv_obj_set_style_border_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(obj, shadow_opa, LV_PART_MAIN | LV_STATE_DEFAULT);
}

uint8_t manual_level_from_target(lv_obj_t *target) {
    if (target == objects.btn_dak_manual_toggle_1) return 1;
    if (target == objects.btn_dak_manual_toggle_2) return 2;
    if (target == objects.btn_dak_manual_toggle_3) return 3;
    if (target == objects.btn_dak_manual_toggle_4) return 4;
    if (target == objects.btn_dak_manual_toggle_5) return 5;
    if (target == objects.btn_dak_manual_toggle_6) return 6;
    if (target == objects.btn_dak_manual_toggle_7) return 7;
    if (target == objects.btn_dak_manual_toggle_8) return 8;
    if (target == objects.btn_dak_manual_toggle_9) return 9;
    if (target == objects.btn_dak_manual_toggle_10) return 10;
    return 0;
}

uint32_t timer_seconds_from_target(lv_obj_t *target) {
    if (target == objects.btn_dak_manual_timer_toggle_30sec) return 30;
    if (target == objects.btn_dak_manual_timer_toggle_1min) return 60;
    if (target == objects.btn_dak_manual_timer_toggle_5min) return 5 * 60;
    if (target == objects.btn_dak_manual_timer_toggle_15min) return 15 * 60;
    if (target == objects.btn_dak_manual_timer_toggle_30min) return 30 * 60;
    if (target == objects.btn_dak_manual_timer_toggle_1h) return 60 * 60;
    return 0;
}

void format_mmss(uint32_t total_seconds, char *out, size_t out_len) {
    const uint32_t minutes = total_seconds / 60;
    const uint32_t seconds = total_seconds % 60;
    snprintf(out, out_len, "%02lu:%02lu",
             static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds));
}

} // namespace

void UiController::update_dac_ui(uint32_t now_ms) {
    const bool available = fanControl.isAvailable();
    const bool faulted = fanControl.isFaulted();
    set_button_enabled(objects.btn_dac_settings, available);

    const bool manual_mode = (fanControl.mode() == FanControl::Mode::Manual);
    set_checked_state(objects.btn_dac_manual_on, manual_mode);
    set_checked_state(objects.btn_dac_auto_on, !manual_mode);
    set_visible(objects.dac_manual_container, manual_mode);
    set_visible(objects.dac_auto_container, !manual_mode);

    const uint8_t manual_step = fanControl.manualStep();
    set_checked_state(objects.btn_dak_manual_toggle_1, manual_step == 1);
    set_checked_state(objects.btn_dak_manual_toggle_2, manual_step == 2);
    set_checked_state(objects.btn_dak_manual_toggle_3, manual_step == 3);
    set_checked_state(objects.btn_dak_manual_toggle_4, manual_step == 4);
    set_checked_state(objects.btn_dak_manual_toggle_5, manual_step == 5);
    set_checked_state(objects.btn_dak_manual_toggle_6, manual_step == 6);
    set_checked_state(objects.btn_dak_manual_toggle_7, manual_step == 7);
    set_checked_state(objects.btn_dak_manual_toggle_8, manual_step == 8);
    set_checked_state(objects.btn_dak_manual_toggle_9, manual_step == 9);
    set_checked_state(objects.btn_dak_manual_toggle_10, manual_step == 10);

    const uint32_t timer_s = fanControl.selectedTimerSeconds();
    set_checked_state(objects.btn_dak_manual_timer_toggle_30sec, timer_s == 30);
    set_checked_state(objects.btn_dak_manual_timer_toggle_1min, timer_s == 60);
    set_checked_state(objects.btn_dak_manual_timer_toggle_5min, timer_s == 5 * 60);
    set_checked_state(objects.btn_dak_manual_timer_toggle_15min, timer_s == 15 * 60);
    set_checked_state(objects.btn_dak_manual_timer_toggle_30min, timer_s == 30 * 60);
    set_checked_state(objects.btn_dak_manual_timer_toggle_1h, timer_s == 60 * 60);

    const bool running = fanControl.isRunning();
    const bool start_active = available && manual_mode && !running;
    const bool stop_active = available && running;
    const lv_color_t neutral = color_card_border();
    set_button_accent(objects.btn_dak_manual_start,
                      start_active ? color_green() : neutral,
                      start_active ? LV_OPA_COVER : LV_OPA_TRANSP);
    set_button_accent(objects.btn_dak_manual_stop,
                      stop_active ? color_red() : neutral,
                      stop_active ? LV_OPA_COVER : LV_OPA_TRANSP);

    if (objects.label_dac_status) {
        const char *status_text = "OFFLINE";
        if (faulted) {
            status_text = "FAULT";
        } else if (available) {
            status_text = running ? "RUNNING" : "STOPPED";
        }
        safe_label_set_text(objects.label_dac_status, status_text);
    }
    if (objects.chip_dac_status) {
        if (faulted) {
            set_chip_color(objects.chip_dac_status, color_red());
        } else if (!available) {
            set_chip_color(objects.chip_dac_status, color_inactive());
        } else if (running) {
            set_chip_color(objects.chip_dac_status, color_green());
        } else {
            set_chip_color(objects.chip_dac_status, color_yellow());
        }
    }

    if (objects.label_dac_output_value) {
        if (!fanControl.isOutputKnown()) {
            safe_label_set_text(objects.label_dac_output_value, "UNKNOWN");
        } else {
            const uint16_t output_mv = fanControl.outputMillivolts();
            const uint8_t output_pct = fanControl.outputPercent();
            char output_buf[24];
            snprintf(output_buf, sizeof(output_buf), "%u.%uV (%u%%)",
                     static_cast<unsigned>(output_mv / 1000),
                     static_cast<unsigned>((output_mv % 1000) / 100),
                     static_cast<unsigned>(output_pct));
            safe_label_set_text(objects.label_dac_output_value, output_buf);
        }
    }

    if (objects.label_dac_timer_value) {
        char timer_buf[16] = "--:--";
        if (running) {
            const uint32_t remaining_s = fanControl.remainingSeconds(now_ms);
            if (remaining_s > 0) {
                format_mmss(remaining_s, timer_buf, sizeof(timer_buf));
            }
        }
        safe_label_set_text(objects.label_dac_timer_value, timer_buf);
    }
}

void UiController::on_dac_settings_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (!fanControl.isAvailable()) {
        return;
    }
    pending_screen_id = SCREEN_ID_PAGE_DAC_SETTINGS;
}

void UiController::on_dac_settings_back_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    pending_screen_id = SCREEN_ID_PAGE_SETTINGS;
}

void UiController::on_dac_manual_on_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    const bool was_auto = (fanControl.mode() == FanControl::Mode::Auto);
    fanControl.setMode(FanControl::Mode::Manual);
    if (was_auto || storage.config().dac_auto_mode) {
        storage.config().dac_auto_mode = false;
        storage.saveConfig(true);
    }
    update_dac_ui(millis());
}

void UiController::on_dac_auto_on_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    const bool was_manual = (fanControl.mode() == FanControl::Mode::Manual);
    fanControl.setMode(FanControl::Mode::Auto);
    if (was_manual || !storage.config().dac_auto_mode) {
        storage.config().dac_auto_mode = true;
        storage.saveConfig(true);
    }
    update_dac_ui(millis());
}

void UiController::on_dac_manual_level_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    const uint8_t level = manual_level_from_target(lv_event_get_target(e));
    if (level == 0) {
        return;
    }
    fanControl.setManualStep(level);
    update_dac_ui(millis());
}

void UiController::on_dac_manual_timer_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    lv_obj_t *target = lv_event_get_target(e);
    const uint32_t timer_s = timer_seconds_from_target(target);
    if (timer_s == 0) {
        return;
    }

    const bool is_checked = lv_obj_has_state(target, LV_STATE_CHECKED);
    if (is_checked) {
        fanControl.setTimerSeconds(timer_s);
    } else if (fanControl.selectedTimerSeconds() == timer_s) {
        fanControl.setTimerSeconds(0);
    }
    update_dac_ui(millis());
}

void UiController::on_dac_manual_start_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (fanControl.mode() != FanControl::Mode::Manual) {
        return;
    }
    fanControl.requestStart();
    update_dac_ui(millis());
}

void UiController::on_dac_manual_stop_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    fanControl.requestStop();
    update_dac_ui(millis());
}

void UiController::on_dac_settings_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_settings_event(e); }
void UiController::on_dac_settings_back_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_settings_back_event(e); }
void UiController::on_dac_manual_on_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_manual_on_event(e); }
void UiController::on_dac_auto_on_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_auto_on_event(e); }
void UiController::on_dac_manual_level_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_manual_level_event(e); }
void UiController::on_dac_manual_timer_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_manual_timer_event(e); }
void UiController::on_dac_manual_start_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_manual_start_event(e); }
void UiController::on_dac_manual_stop_event_cb(lv_event_t *e) { if (instance_) instance_->on_dac_manual_stop_event(e); }
