// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"

#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "config/AppConfig.h"
#include "core/MathUtils.h"
#include "modules/ChartsHistory.h"
#include "ui/UiText.h"
#include "ui/ui.h"

namespace {

bool format_pm05_count(float value, char *buf, size_t buf_size) {
    if (!isfinite(value) || value < 0.0f || !buf || buf_size == 0) {
        return false;
    }
    if (value >= 1000.0f) {
        snprintf(buf, buf_size, "%.1fk", value / 1000.0f);
    } else {
        snprintf(buf, buf_size, "%.0f", value);
    }
    return true;
}

float temperature_to_display(float celsius, bool units_c) {
    if (units_c) {
        return celsius;
    }
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

float graph_nice_step(float value) {
    if (!isfinite(value) || value <= 0.0f) {
        return 1.0f;
    }
    const float exponent = floorf(log10f(value));
    const float base = powf(10.0f, exponent);
    const float normalized = value / base;
    float nice = 1.0f;
    if (normalized <= 1.0f) {
        nice = 1.0f;
    } else if (normalized <= 2.0f) {
        nice = 2.0f;
    } else if (normalized <= 5.0f) {
        nice = 5.0f;
    } else {
        nice = 10.0f;
    }
    return nice * base;
}

bool format_epoch_hhmm(time_t epoch, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0 || epoch <= Config::TIME_VALID_EPOCH) {
        return false;
    }
    tm local_tm = {};
    if (!localtime_r(&epoch, &local_tm)) {
        return false;
    }
    snprintf(buf, buf_size, "%02d:%02d", local_tm.tm_hour, local_tm.tm_min);
    return true;
}

void format_relative_time_label(uint32_t offset_s, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return;
    }
    if (offset_s == 0) {
        snprintf(buf, buf_size, "now");
        return;
    }
    const uint32_t hours = offset_s / 3600U;
    const uint32_t minutes = (offset_s % 3600U) / 60U;
    if (hours > 0 && minutes == 0) {
        snprintf(buf, buf_size, "-%luh", static_cast<unsigned long>(hours));
        return;
    }
    if (hours > 0) {
        snprintf(buf, buf_size, "-%luh%02lum", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes));
        return;
    }
    snprintf(buf, buf_size, "-%lum", static_cast<unsigned long>(minutes > 0 ? minutes : 1U));
}

constexpr uint8_t kTempGraphTimeTickCount = 7;

} // namespace

uint16_t UiController::temperature_graph_points() const {
    switch (temp_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

void UiController::set_temperature_info_mode(bool graph_mode) {
    temp_graph_mode_ = graph_mode;
    if (objects.btn_info_graph) {
        lv_obj_add_flag(objects.btn_info_graph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_info_graph, 18);
    }
    if (objects.btn_temp_range_1h) {
        lv_obj_add_flag(objects.btn_temp_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_temp_range_1h, 12);
    }
    if (objects.btn_temp_range_3h) {
        lv_obj_add_flag(objects.btn_temp_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_temp_range_3h, 12);
    }
    if (objects.btn_temp_range_24h) {
        lv_obj_add_flag(objects.btn_temp_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_temp_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.temperature_info_thresholds, !graph_mode);
    set_visible(objects.temperature_info_graph, graph_mode);
    set_visible(objects.container_thresholds_dots, !graph_mode);

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

    set_checked(objects.btn_info_graph, graph_mode);
    set_checked(objects.btn_temp_range_1h, temp_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_temp_range_3h, temp_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_temp_range_24h, temp_graph_range_ == TEMP_GRAPH_RANGE_24H);
}

void UiController::apply_temperature_graph_theme() {
    if (!objects.chart_temp_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_temp_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_temp_info, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(objects.chart_temp_info, 5, 12);

    lv_obj_set_style_bg_color(objects.chart_temp_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_temp_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_temp_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_temp_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_temp_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_temp_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_temp_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_temp_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_temp_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_temp_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_temp_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_temp_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

void UiController::ensure_temperature_graph_overlays() {
    if (!objects.chart_temp_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_temp_info) {
            label = lv_label_create(objects.chart_temp_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(temp_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(temp_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(temp_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_temperature_graph_overlays(bool has_values, float min_temp, float max_temp, float latest_temp) {
    if (!objects.chart_temp_info) {
        return;
    }

    ensure_temperature_graph_overlays();
    if (!temp_graph_label_min_ || !temp_graph_label_now_ || !temp_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_temp_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_temp_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {temp_graph_label_min_, temp_graph_label_now_, temp_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(temp_graph_label_min_, "MIN --");
        safe_label_set_text(temp_graph_label_now_, "NOW --");
        safe_label_set_text(temp_graph_label_max_, "MAX --");
        return;
    }

    const char *unit = temp_units_c ? UiText::UnitC() : UiText::UnitF();
    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f%s", min_temp, unit ? unit : "");
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f%s", latest_temp, unit ? unit : "");
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f%s", max_temp, unit ? unit : "");
    safe_label_set_text(temp_graph_label_min_, min_buf);
    safe_label_set_text(temp_graph_label_now_, now_buf);
    safe_label_set_text(temp_graph_label_max_, max_buf);
}

void UiController::ensure_temperature_time_labels() {
    if (!objects.temperature_info_graph || !objects.chart_temp_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.temperature_info_graph) {
            label = lv_label_create(objects.temperature_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(temp_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_temp_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = temp_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_temperature_time_labels() {
    if (!objects.chart_temp_info || !objects.temperature_info_graph) {
        return;
    }

    ensure_temperature_time_labels();
    if (!temp_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = temperature_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = temp_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_temp_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_temp_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_temp_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_temp_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = temp_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_temperature_info_graph() {
    if (!objects.chart_temp_info) {
        return;
    }

    apply_temperature_graph_theme();

    const uint16_t points = temperature_graph_points();
    lv_chart_set_point_count(objects.chart_temp_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_temp_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_temp_info,
                                     lv_obj_get_style_line_color(objects.chart_temp_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_temp_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_temp_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_temp = FLT_MAX;
    float max_temp = -FLT_MAX;
    float latest_temp = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value_c = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_TEMPERATURE, value_c, valid) &&
                valid && isfinite(value_c)) {
                const float display_value = temperature_to_display(value_c, temp_units_c);
                if (!has_values) {
                    min_temp = display_value;
                    max_temp = display_value;
                    has_values = true;
                } else {
                    if (display_value < min_temp) {
                        min_temp = display_value;
                    }
                    if (display_value > max_temp) {
                        max_temp = display_value;
                    }
                }
                latest_temp = display_value;
                point_value = static_cast<lv_coord_t>(lroundf(display_value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_temp_info, series, i, point_value);
    }

    if (!has_values) {
        const float fallback = temp_units_c ? 22.0f : 71.6f;
        const lv_coord_t center = static_cast<lv_coord_t>(lroundf(fallback * 10.0f));
        lv_chart_set_range(objects.chart_temp_info,
                           LV_CHART_AXIS_PRIMARY_Y,
                           static_cast<lv_coord_t>(center - 100),
                           static_cast<lv_coord_t>(center + 100));
        update_temperature_graph_overlays(false, fallback, fallback, fallback);
        update_temperature_time_labels();
        lv_chart_refresh(objects.chart_temp_info);
        return;
    }

    const float min_span = temp_units_c ? 2.0f : 3.5f;
    const float span = max_temp - min_temp;
    float scale_span = span;
    if (!isfinite(scale_span) || scale_span < min_span) {
        scale_span = min_span;
    }
    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = temp_units_c ? 0.5f : 1.0f;
    }
    float y_min_f = floorf((min_temp - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((max_temp + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_temp) ? latest_temp : (temp_units_c ? 22.0f : 71.6f);
        y_min_f = center - min_span;
        y_max_f = center + min_span;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }
    if ((y_max - y_min) < 10) {
        const lv_coord_t center = static_cast<lv_coord_t>((y_min + y_max) / 2);
        y_min = static_cast<lv_coord_t>(center - 5);
        y_max = static_cast<lv_coord_t>(center + 5);
    }
    lv_chart_set_range(objects.chart_temp_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);

    if (!isfinite(latest_temp)) {
        latest_temp = max_temp;
    }
    update_temperature_graph_overlays(true, min_temp, max_temp, latest_temp);
    update_temperature_time_labels();

    lv_chart_refresh(objects.chart_temp_info);
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
            set_temperature_info_mode(temp_graph_mode_);
            if (temp_graph_mode_) {
                update_temperature_info_graph();
            }
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
            const char *unit = objects.label_hum_unit_1
                ? lv_label_get_text(objects.label_hum_unit_1)
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
            const char *unit = objects.label_ah_unit_1
                ? lv_label_get_text(objects.label_ah_unit_1)
                : "g/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t ah_col = getAbsoluteHumidityColor(ah_gm3);
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(ah_col));
            break;
        }
        case INFO_MR: {
            const int mold_risk =
                (currentData.temp_valid && currentData.hum_valid)
                    ? MathUtils::compute_mold_risk_index(currentData.temperature, currentData.humidity)
                    : -1;
            if (mold_risk >= 0) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", mold_risk);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text_static(objects.label_sensor_value, UiText::ValueMissingShort());
            }

            const char *unit = UiText::UnitIndex();
            if (objects.label_mr_unit) {
                unit = lv_label_get_text(objects.label_mr_unit);
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);

            lv_color_t mr_col = color_inactive();
            if (mold_risk >= 0) {
                if (mold_risk <= 2) {
                    mr_col = color_green();
                } else if (mold_risk <= 4) {
                    mr_col = color_yellow();
                } else if (mold_risk <= 7) {
                    mr_col = color_orange();
                } else {
                    mr_col = color_red();
                }
            }
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(mr_col));
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
        case INFO_PM05: {
            if (currentData.pm05_valid) {
                char buf[16];
                if (format_pm05_count(currentData.pm05, buf, sizeof(buf))) {
                    safe_label_set_text(objects.label_sensor_value, buf);
                } else {
                    safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
                }
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = objects.label_pm05_unit
                ? lv_label_get_text(objects.label_pm05_unit)
                : "#/cm3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm05_col = currentData.pm05_valid ? getPM05Color(currentData.pm05) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(pm05_col));
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
            if (objects.label_pm10_unit) {
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
            const bool pm1_available = currentData.pm1_valid && isfinite(currentData.pm1) && currentData.pm1 >= 0.0f;
            if (pm1_available) {
                char buf[16];
                if (currentData.pm1 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm1);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm1);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = objects.label_pm10_unit
                ? lv_label_get_text(objects.label_pm10_unit)
                : "ug/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm1_col = pm1_available ? getPM1Color(currentData.pm1) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(pm1_col));
            break;
        }
        case INFO_CO: {
            const bool co_valid = currentData.co_valid && isfinite(currentData.co_ppm) && currentData.co_ppm >= 0.0f;
            if (co_valid) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%.0f", currentData.co_ppm);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissingShort());
            }
            safe_label_set_text(objects.label_sensor_info_unit, "ppm");
            lv_color_t co_col = co_valid ? getCOColor(currentData.co_ppm) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(co_col));
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
            safe_label_set_text(objects.label_pressure_value_1, buf);

            const char *unit = nullptr;
            if (objects.label_pressure_unit_1) {
                unit = lv_label_get_text(objects.label_pressure_unit_1);
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
            safe_label_set_text(objects.label_delta_24h_value_1, buf);

            lv_color_t delta_3h_color = night_mode
                ? color_card_border()
                : getPressureDeltaColor(currentData.pressure_delta_3h, currentData.pressure_delta_3h_valid, false);
            lv_color_t delta_24h_color = night_mode
                ? color_card_border()
                : getPressureDeltaColor(currentData.pressure_delta_24h, currentData.pressure_delta_24h_valid, true);
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
            } else {
                value = UiText::ValueMissing();
            }
            safe_label_set_text(objects.label_sensor_value, value);
            const char *unit = nullptr;
            if (objects.label_temp_unit_1) {
                unit = lv_label_get_text(objects.label_temp_unit_1);
            } else {
                unit = temp_units_c ? UiText::UnitC() : UiText::UnitF();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            set_temperature_info_mode(temp_graph_mode_);
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
        case INFO_PM05:
        case INFO_PM25:
        case INFO_PM10:
        case INFO_PM1:
        case INFO_CO:
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

    if (sensor == INFO_CO) {
        set_visible(objects.co_info, true);
        if (objects.label_sensor_info_title) {
            safe_label_set_text(objects.label_sensor_info_title, "CO");
        }
        update_sensor_info_ui();
        return;
    }

    set_visible(objects.pm_info, true);
    const bool pm1_pm10_group = (sensor == INFO_PM1) || (sensor == INFO_PM10);
    set_visible(objects.pm1_pm10_info, pm1_pm10_group);
    set_visible(objects.pm05_info, sensor == INFO_PM05);
    set_visible(objects.pm25_info, sensor == INFO_PM25);
    set_visible(objects.pm10_info, sensor == INFO_PM10);
    set_visible(objects.pm1_info, sensor == INFO_PM1);
    if (pm1_pm10_group) {
        auto set_checked = [](lv_obj_t *btn, bool checked) {
            if (!btn) return;
            if (checked) lv_obj_add_state(btn, LV_STATE_CHECKED);
            else lv_obj_clear_state(btn, LV_STATE_CHECKED);
        };
        set_checked(objects.btn_pm10_info, sensor == INFO_PM10);
        set_checked(objects.btn_pm1_info, sensor == INFO_PM1);
        // Keep PM tab buttons above PM1/PM10 content layers so they remain clickable.
        if (objects.btn_pm10_info) lv_obj_move_foreground(objects.btn_pm10_info);
        if (objects.btn_pm1_info) lv_obj_move_foreground(objects.btn_pm1_info);
    }

    if (objects.label_sensor_info_title) {
        if (sensor == INFO_PM05) {
            safe_label_set_text(objects.label_sensor_info_title, "PM0.5");
        } else if (sensor == INFO_PM25) {
            safe_label_set_text(objects.label_sensor_info_title, "PM2.5");
        } else if (sensor == INFO_PM10) {
            safe_label_set_text(objects.label_sensor_info_title, "PM10");
        } else if (sensor == INFO_PM1) {
            safe_label_set_text(objects.label_sensor_info_title, "PM1");
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
    } else {
        title = "PRESSURE";
    }
    safe_label_set_text(objects.label_sensor_info_title, title);

    update_sensor_info_ui();
}

void UiController::set_visible(lv_obj_t *obj, bool visible) {
    if (!obj) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void UiController::hide_all_sensor_info_containers() {
    set_visible(objects.temperature_info, false);
    set_visible(objects.temperature_info_thresholds, false);
    set_visible(objects.temperature_info_graph, false);
    set_visible(objects.container_thresholds_dots, false);
    set_visible(objects.co2_info, false);
    set_visible(objects.voc_info, false);
    set_visible(objects.nox_info, false);
    set_visible(objects.hcho_info, false);
    set_visible(objects.co_info, false);
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
    set_visible(objects.pm1_pm10_info, false);
    set_visible(objects.pm05_info, false);
    set_visible(objects.pm10_info, false);
    set_visible(objects.pm25_info, false);
    set_visible(objects.pm1_info, false);
}

