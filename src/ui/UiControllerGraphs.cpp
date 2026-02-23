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
#include "modules/ChartsHistory.h"
#include "ui/UiText.h"
#include "ui/ui.h"

#include "ui/UiControllerGraphsShared.h"

uint16_t UiController::graph_points_for_range(TempGraphRange range) const {
    switch (range) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::temperature_graph_points() const {
    return graph_points_for_range(temp_graph_range_);
}

uint16_t UiController::humidity_graph_points() const {
    return graph_points_for_range(rh_graph_range_);
}

uint16_t UiController::voc_graph_points() const {
    return graph_points_for_range(voc_graph_range_);
}

uint16_t UiController::nox_graph_points() const {
    return graph_points_for_range(nox_graph_range_);
}

uint16_t UiController::hcho_graph_points() const {
    return graph_points_for_range(hcho_graph_range_);
}

uint16_t UiController::co2_graph_points() const {
    return graph_points_for_range(co2_graph_range_);
}

uint16_t UiController::co_graph_points() const {
    return graph_points_for_range(co_graph_range_);
}

uint16_t UiController::pm05_graph_points() const {
    return graph_points_for_range(pm05_graph_range_);
}

uint16_t UiController::pm25_4_graph_points() const {
    return graph_points_for_range(pm25_4_graph_range_);
}

uint16_t UiController::pm1_10_graph_points() const {
    return graph_points_for_range(pm1_10_graph_range_);
}

uint16_t UiController::pressure_graph_points() const {
    return graph_points_for_range(pressure_graph_range_);
}

uint8_t UiController::graph_vertical_divisions_for_range(TempGraphRange range) const {
    return (range == TEMP_GRAPH_RANGE_24H) ? 25U : 13U;
}

void UiController::apply_standard_info_chart_theme(lv_obj_t *chart, uint8_t horizontal_divisions, uint8_t vertical_divisions) {
    if (!chart) {
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

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, horizontal_divisions, vertical_divisions);

    lv_obj_set_style_bg_color(chart, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chart, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(chart, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chart, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(chart, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(chart, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(chart, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(chart, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(chart, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

lv_chart_series_t *UiController::ensure_info_chart_series(lv_obj_t *chart, uint16_t points) {
    if (!chart) {
        return nullptr;
    }

    lv_chart_set_point_count(chart, points);

    lv_chart_series_t *series = lv_chart_get_series_next(chart, nullptr);
    if (!series) {
        series = lv_chart_add_series(chart,
                                     lv_obj_get_style_line_color(chart, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return nullptr;
    }

    series->color = lv_obj_get_style_line_color(chart, LV_PART_ITEMS);
    lv_chart_set_all_value(chart, series, LV_CHART_POINT_NONE);
    return series;
}

UiController::GraphSeriesStats UiController::populate_info_chart_series(lv_obj_t *chart,
                                                                        lv_chart_series_t *series,
                                                                        uint16_t points,
                                                                        int metric_id,
                                                                        float point_scale,
                                                                        bool require_non_negative,
                                                                        bool convert_temperature_to_display) {
    GraphSeriesStats stats{};
    stats.has_values = false;
    stats.min_value = FLT_MAX;
    stats.max_value = -FLT_MAX;
    stats.latest_value = NAN;

    if (!chart || !series || points == 0) {
        return stats;
    }

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;
    const ChartsHistory::Metric metric = static_cast<ChartsHistory::Metric>(metric_id);

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float raw_value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, metric, raw_value, valid) &&
                valid && isfinite(raw_value)) {
                float display_value = raw_value;
                if (convert_temperature_to_display) {
                    display_value = temperature_to_display(raw_value, temp_units_c);
                }
                if (isfinite(display_value) && (!require_non_negative || display_value >= 0.0f)) {
                    if (!stats.has_values) {
                        stats.min_value = display_value;
                        stats.max_value = display_value;
                        stats.has_values = true;
                    } else {
                        if (display_value < stats.min_value) {
                            stats.min_value = display_value;
                        }
                        if (display_value > stats.max_value) {
                            stats.max_value = display_value;
                        }
                    }
                    stats.latest_value = display_value;
                    point_value = static_cast<lv_coord_t>(lroundf(display_value * point_scale));
                }
            }
        }
        lv_chart_set_value_by_id(chart, series, i, point_value);
    }

    return stats;
}

UiController::SensorGraphProfile UiController::build_temperature_graph_profile() const {
    SensorGraphProfile profile{};
    profile.min_span = temp_units_c ? 2.0f : 3.5f;
    profile.fallback_value = temp_units_c ? 22.0f : 71.6f;
    switch (temp_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            // 0..60 min with 5 min step => 13 vertical marks
            profile.vertical_divisions = 13;
            break;
        case TEMP_GRAPH_RANGE_24H:
            // 0..24 h with 1 h step => 25 vertical marks
            profile.vertical_divisions = 25;
            break;
        case TEMP_GRAPH_RANGE_3H:
        default:
            // 0..180 min with 15 min step => 13 vertical marks
            profile.vertical_divisions = 13;
            break;
    }
    profile.horizontal_divisions_min = 3;
    profile.horizontal_divisions_max = 12;
    profile.unit = temp_units_c ? UiText::UnitC() : UiText::UnitF();
    profile.label_min = "MIN";
    profile.label_now = "NOW";
    profile.label_max = "MAX";
    profile.zone_count = 7;

    const float bounds_c[kMaxGraphZoneBounds] = {-1000.0f, 16.0f, 18.0f, 20.0f, 25.0f, 26.0f, 28.0f, 1000.0f};
    for (uint8_t i = 0; i < kMaxGraphZoneBounds; ++i) {
        const bool edge = (i == 0) || (i == (kMaxGraphZoneBounds - 1));
        profile.zone_bounds[i] = edge ? bounds_c[i] : temperature_to_display(bounds_c[i], temp_units_c);
    }

    profile.zone_tones[0] = GRAPH_ZONE_RED;
    profile.zone_tones[1] = GRAPH_ZONE_ORANGE;
    profile.zone_tones[2] = GRAPH_ZONE_YELLOW;
    profile.zone_tones[3] = GRAPH_ZONE_GREEN;
    profile.zone_tones[4] = GRAPH_ZONE_YELLOW;
    profile.zone_tones[5] = GRAPH_ZONE_ORANGE;
    profile.zone_tones[6] = GRAPH_ZONE_RED;

    return profile;
}

lv_color_t UiController::resolve_graph_zone_color(GraphZoneTone tone, lv_color_t chart_bg) {
    switch (tone) {
        case GRAPH_ZONE_RED:
            return lv_color_mix(color_red(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_ORANGE:
            return lv_color_mix(color_orange(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_YELLOW:
            return lv_color_mix(color_yellow(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_GREEN:
            return lv_color_mix(color_green(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_BLUE:
            return lv_color_mix(color_blue(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_NONE:
        default:
            return lv_color_mix(color_card_border(), chart_bg, LV_OPA_40);
    }
}

void UiController::ensure_graph_time_labels(lv_obj_t *graph_container,
                                            lv_obj_t *chart,
                                            lv_obj_t **labels,
                                            uint8_t label_count) {
    if (!graph_container || !chart || !labels || label_count == 0) {
        return;
    }

    auto ensure_label = [graph_container](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != graph_container) {
            label = lv_label_create(graph_container);
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

    for (uint8_t i = 0; i < label_count; ++i) {
        ensure_label(labels[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(chart, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < label_count; ++i) {
        lv_obj_t *label = labels[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_graph_time_labels(lv_obj_t *graph_container,
                                            lv_obj_t *chart,
                                            lv_obj_t **labels,
                                            uint8_t label_count,
                                            uint16_t points,
                                            bool clear_when_points_lt_two,
                                            bool chart_layout_before_position,
                                            bool move_foreground_after_position) {
    if (!graph_container || !chart || !labels || label_count == 0) {
        return;
    }

    ensure_graph_time_labels(graph_container, chart, labels, label_count);
    if (!labels[0]) {
        return;
    }

    if (clear_when_points_lt_two && points < 2U) {
        for (uint8_t i = 0; i < label_count; ++i) {
            if (labels[i]) {
                safe_label_set_text(labels[i], "");
            }
        }
        return;
    }

    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1U) ? static_cast<uint32_t>(points - 1U) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0U) {
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

    const uint8_t last_tick = (label_count > 0) ? static_cast<uint8_t>(label_count - 1U) : 0U;
    for (uint8_t i = 0; i < label_count; ++i) {
        lv_obj_t *label = labels[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(last_tick - i);
        const uint32_t offset_s = (last_tick > 0)
            ? static_cast<uint32_t>((static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) /
                                    static_cast<uint64_t>(last_tick))
            : 0U;

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

    if (chart_layout_before_position) {
        lv_obj_update_layout(chart);
    }

    const lv_coord_t chart_x = lv_obj_get_x(chart);
    const lv_coord_t chart_y = lv_obj_get_y(chart);
    const lv_coord_t chart_w = lv_obj_get_width(chart);
    const lv_coord_t chart_h = lv_obj_get_height(chart);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < label_count; ++i) {
        lv_obj_t *label = labels[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1 && last_tick > 0) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(last_tick));
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
        if (move_foreground_after_position) {
            lv_obj_move_foreground(label);
        }
    }
}

void UiController::ensure_graph_stat_overlays(lv_obj_t *chart,
                                              lv_obj_t *&label_min,
                                              lv_obj_t *&label_now,
                                              lv_obj_t *&label_max) {
    if (!chart) {
        return;
    }

    auto ensure_label = [chart](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != chart) {
            label = lv_label_create(chart);
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

    ensure_label(label_min, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(label_now, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(label_max, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::style_graph_stat_overlays(lv_obj_t *chart,
                                             lv_obj_t *label_min,
                                             lv_obj_t *label_now,
                                             lv_obj_t *label_max) {
    if (!chart) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(chart, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(chart, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {label_min, label_now, label_max};
    for (lv_obj_t *label : labels) {
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void UiController::sync_info_graph_button_state() {
    if (!objects.btn_info_graph) {
        return;
    }

    const bool voc_selected = (info_sensor == INFO_VOC);
    const bool nox_selected = (info_sensor == INFO_NOX);
    const bool hcho_selected = (info_sensor == INFO_HCHO);
    const bool co2_selected = (info_sensor == INFO_CO2);
    const bool co_selected = (info_sensor == INFO_CO);
    const bool pm05_selected = (info_sensor == INFO_PM05);
    const bool pm25_4_selected = (info_sensor == INFO_PM25) || (info_sensor == INFO_PM4);
    const bool pm1_10_selected = (info_sensor == INFO_PM1) || (info_sensor == INFO_PM10);
    const bool pressure_selected = (info_sensor == INFO_PRESSURE_3H) || (info_sensor == INFO_PRESSURE_24H);
    const bool graph_supported = (info_sensor == INFO_TEMP) || (info_sensor == INFO_RH) ||
                                 voc_selected || nox_selected || hcho_selected || co2_selected ||
                                 pm05_selected ||
                                 pm25_4_selected ||
                                 pm1_10_selected ||
                                 co_selected || pressure_selected;
    const bool graph_checked =
        ((info_sensor == INFO_TEMP) && temp_graph_mode_) ||
        ((info_sensor == INFO_RH) && rh_graph_mode_) ||
        (voc_selected && voc_graph_mode_) ||
        (nox_selected && nox_graph_mode_) ||
        (hcho_selected && hcho_graph_mode_) ||
        (co2_selected && co2_graph_mode_) ||
        (pm05_selected && pm05_graph_mode_) ||
        (pm25_4_selected && pm25_4_graph_mode_) ||
        (pm1_10_selected && pm1_10_graph_mode_) ||
        (co_selected && co_graph_mode_) ||
        (pressure_selected && pressure_graph_mode_);

    if (graph_checked) {
        lv_obj_add_state(objects.btn_info_graph, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(objects.btn_info_graph, LV_STATE_CHECKED);
    }

    if (graph_supported) {
        set_visible(objects.btn_info_graph, true);
        lv_obj_clear_state(objects.btn_info_graph, LV_STATE_DISABLED);
        lv_obj_add_flag(objects.btn_info_graph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_move_foreground(objects.btn_info_graph);
    } else {
        lv_obj_clear_state(objects.btn_info_graph, LV_STATE_CHECKED);
        lv_obj_add_state(objects.btn_info_graph, LV_STATE_DISABLED);
        set_visible(objects.btn_info_graph, false);
    }

    lv_obj_set_ext_click_area(objects.btn_info_graph, 18);
}

bool UiController::should_show_threshold_dots() const {
    if (info_sensor == INFO_NONE) {
        return false;
    }
    if (info_sensor == INFO_TEMP) {
        return !temp_graph_mode_;
    }
    if (info_sensor == INFO_RH) {
        return !rh_graph_mode_;
    }
    if (info_sensor == INFO_VOC) {
        return !voc_graph_mode_;
    }
    if (info_sensor == INFO_NOX) {
        return !nox_graph_mode_;
    }
    if (info_sensor == INFO_HCHO) {
        return !hcho_graph_mode_;
    }
    if (info_sensor == INFO_CO2) {
        return !co2_graph_mode_;
    }
    if (info_sensor == INFO_PM05) {
        return !pm05_graph_mode_;
    }
    if (info_sensor == INFO_PM25 || info_sensor == INFO_PM4) {
        return !pm25_4_graph_mode_;
    }
    if (info_sensor == INFO_PM1 || info_sensor == INFO_PM10) {
        return !pm1_10_graph_mode_;
    }
    if (info_sensor == INFO_CO) {
        return !co_graph_mode_;
    }
    if (info_sensor == INFO_PRESSURE_3H || info_sensor == INFO_PRESSURE_24H) {
        return !pressure_graph_mode_;
    }
    return true;
}

void UiController::sync_threshold_dots_visibility() {
    set_visible(objects.container_thresholds_dots, should_show_threshold_dots());
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
    sync_threshold_dots_visibility();

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

    sync_info_graph_button_state();
    set_checked(objects.btn_temp_range_1h, temp_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_temp_range_3h, temp_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_temp_range_24h, temp_graph_range_ == TEMP_GRAPH_RANGE_24H);
}

void UiController::set_rh_info_mode(bool graph_mode) {
    rh_graph_mode_ = graph_mode;
    if (objects.btn_rh_range_1h) {
        lv_obj_add_flag(objects.btn_rh_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_rh_range_1h, 12);
    }
    if (objects.btn_rh_range_3h) {
        lv_obj_add_flag(objects.btn_rh_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_rh_range_3h, 12);
    }
    if (objects.btn_rh_range_24h) {
        lv_obj_add_flag(objects.btn_rh_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_rh_range_24h, 12);
    }
    set_visible(objects.rh_info_thresholds, !graph_mode);
    set_visible(objects.rh_info_graph, graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_rh_range_1h, rh_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_rh_range_3h, rh_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_rh_range_24h, rh_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_voc_info_mode(bool graph_mode) {
    voc_graph_mode_ = graph_mode;
    if (objects.btn_voc_range_1h) {
        lv_obj_add_flag(objects.btn_voc_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_voc_range_1h, 12);
    }
    if (objects.btn_voc_range_3h) {
        lv_obj_add_flag(objects.btn_voc_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_voc_range_3h, 12);
    }
    if (objects.btn_voc_range_24h) {
        lv_obj_add_flag(objects.btn_voc_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_voc_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.voc_info_thresholds, !graph_mode);
    set_visible(objects.voc_info_graph, graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_voc_range_1h, voc_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_voc_range_3h, voc_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_voc_range_24h, voc_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_nox_info_mode(bool graph_mode) {
    nox_graph_mode_ = graph_mode;
    if (objects.btn_nox_range_1h) {
        lv_obj_add_flag(objects.btn_nox_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_nox_range_1h, 12);
    }
    if (objects.btn_nox_range_3h) {
        lv_obj_add_flag(objects.btn_nox_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_nox_range_3h, 12);
    }
    if (objects.btn_nox_range_24h) {
        lv_obj_add_flag(objects.btn_nox_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_nox_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.nox_info_thresholds, !graph_mode);
    set_visible(objects.nox_info_graph, graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_nox_range_1h, nox_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_nox_range_3h, nox_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_nox_range_24h, nox_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_hcho_info_mode(bool graph_mode) {
    hcho_graph_mode_ = graph_mode;
    if (objects.btn_hcho_range_1h) {
        lv_obj_add_flag(objects.btn_hcho_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_hcho_range_1h, 12);
    }
    if (objects.btn_hcho_range_3h) {
        lv_obj_add_flag(objects.btn_hcho_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_hcho_range_3h, 12);
    }
    if (objects.btn_hcho_range_24h) {
        lv_obj_add_flag(objects.btn_hcho_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_hcho_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.hcho_info_thresholds, !graph_mode);
    set_visible(objects.hcho_info_graph, graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_hcho_range_1h, hcho_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_hcho_range_3h, hcho_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_hcho_range_24h, hcho_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_co2_info_mode(bool graph_mode) {
    co2_graph_mode_ = graph_mode;
    if (objects.btn_co2_range_1h) {
        lv_obj_add_flag(objects.btn_co2_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co2_range_1h, 12);
    }
    if (objects.btn_co2_range_3h) {
        lv_obj_add_flag(objects.btn_co2_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co2_range_3h, 12);
    }
    if (objects.btn_co2_range_24h) {
        lv_obj_add_flag(objects.btn_co2_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co2_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.co2_info_thresholds, !graph_mode);
    set_visible(objects.co2_info_graph, graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_co2_range_1h, co2_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_co2_range_3h, co2_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_co2_range_24h, co2_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_pm05_info_mode(bool graph_mode) {
    pm05_graph_mode_ = graph_mode;
    if (objects.btn_pm05_range_1h) {
        lv_obj_add_flag(objects.btn_pm05_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm05_range_1h, 12);
    }
    if (objects.btn_pm05_range_3h) {
        lv_obj_add_flag(objects.btn_pm05_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm05_range_3h, 12);
    }
    if (objects.btn_pm05_range_24h) {
        lv_obj_add_flag(objects.btn_pm05_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm05_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }

    set_visible(objects.pm05_info_thresholds, !graph_mode);
    set_visible(objects.pm05_info_graph, graph_mode && (info_sensor == INFO_PM05));
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_pm05_range_1h, pm05_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_pm05_range_3h, pm05_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_pm05_range_24h, pm05_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_pm25_4_info_mode(bool graph_mode) {
    pm25_4_graph_mode_ = graph_mode;
    if (objects.btn_pm25_4_range_1h) {
        lv_obj_add_flag(objects.btn_pm25_4_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm25_4_range_1h, 12);
    }
    if (objects.btn_pm25_4_range_3h) {
        lv_obj_add_flag(objects.btn_pm25_4_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm25_4_range_3h, 12);
    }
    if (objects.btn_pm25_4_range_24h) {
        lv_obj_add_flag(objects.btn_pm25_4_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm25_4_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    if (objects.btn_pm25_info) {
        lv_obj_move_foreground(objects.btn_pm25_info);
    }
    if (objects.btn_pm4_info) {
        lv_obj_move_foreground(objects.btn_pm4_info);
    }
    if (objects.pm25_info) {
        lv_obj_clear_flag(objects.pm25_info, LV_OBJ_FLAG_CLICKABLE);
    }

    const bool pm25_selected = info_sensor == INFO_PM25;
    const bool pm4_selected = info_sensor == INFO_PM4;
    const bool pm25_4_selected = pm25_selected || pm4_selected;
    set_visible(objects.pm25_4_graph, graph_mode && pm25_4_selected);
    set_visible(objects.label_pm25_text, pm25_selected);
    set_visible(objects.label_pm4_text, pm4_selected);
    set_visible(objects.pm25_info_thresholds, !graph_mode && pm25_selected);
    set_visible(objects.pm4_info_thresholds, !graph_mode && pm4_selected);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_pm25_4_range_1h, pm25_4_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_pm25_4_range_3h, pm25_4_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_pm25_4_range_24h, pm25_4_graph_range_ == TEMP_GRAPH_RANGE_24H);
    set_checked(objects.btn_pm25_info, pm25_selected);
    set_checked(objects.btn_pm4_info, pm4_selected);

    sync_info_graph_button_state();
}

void UiController::set_pm1_10_info_mode(bool graph_mode) {
    pm1_10_graph_mode_ = graph_mode;
    if (objects.btn_pm1_10_range_1h) {
        lv_obj_add_flag(objects.btn_pm1_10_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm1_10_range_1h, 12);
    }
    if (objects.btn_pm1_10_range_3h) {
        lv_obj_add_flag(objects.btn_pm1_10_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm1_10_range_3h, 12);
    }
    if (objects.btn_pm1_10_range_24h) {
        lv_obj_add_flag(objects.btn_pm1_10_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pm1_10_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    if (objects.btn_pm1_info) {
        lv_obj_move_foreground(objects.btn_pm1_info);
    }
    if (objects.btn_pm10_info) {
        lv_obj_move_foreground(objects.btn_pm10_info);
    }
    // PM1/PM10 info containers are decorative only; keep them non-clickable so range buttons remain tappable.
    if (objects.pm1_info) {
        lv_obj_clear_flag(objects.pm1_info, LV_OBJ_FLAG_CLICKABLE);
    }
    if (objects.pm10_info) {
        lv_obj_clear_flag(objects.pm10_info, LV_OBJ_FLAG_CLICKABLE);
    }

    const bool pm1_selected = info_sensor == INFO_PM1;
    const bool pm10_selected = info_sensor == INFO_PM10;
    const bool pm1_10_selected = pm1_selected || pm10_selected;
    set_visible(objects.pm1_10_info_graph, graph_mode && pm1_10_selected);
    set_visible(objects.pm1_info, pm1_selected);
    set_visible(objects.pm10_info, pm10_selected);
    set_visible(objects.pm1_info_thresholds, !graph_mode && pm1_selected);
    set_visible(objects.pm10_info_thresholds, !graph_mode && pm10_selected);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_pm1_10_range_1h, pm1_10_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_pm1_10_range_3h, pm1_10_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_pm1_10_range_24h, pm1_10_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_co_info_mode(bool graph_mode) {
    co_graph_mode_ = graph_mode;
    if (objects.btn_co_range_1h) {
        lv_obj_add_flag(objects.btn_co_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co_range_1h, 12);
    }
    if (objects.btn_co_range_3h) {
        lv_obj_add_flag(objects.btn_co_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co_range_3h, 12);
    }
    if (objects.btn_co_range_24h) {
        lv_obj_add_flag(objects.btn_co_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.co_info_thresholds, !graph_mode);
    set_visible(objects.co_info_graph, graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_co_range_1h, co_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_co_range_3h, co_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_co_range_24h, co_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_pressure_info_mode(bool graph_mode) {
    pressure_graph_mode_ = graph_mode;

    if (objects.btn_pressure_range_1h) {
        lv_obj_add_flag(objects.btn_pressure_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pressure_range_1h, 12);
    }
    if (objects.btn_pressure_range_3h) {
        lv_obj_add_flag(objects.btn_pressure_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pressure_range_3h, 12);
    }
    if (objects.btn_pressure_range_24h) {
        lv_obj_add_flag(objects.btn_pressure_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pressure_range_24h, 12);
    }

    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }

    const bool show_3h = info_sensor == INFO_PRESSURE_3H;
    const bool show_24h = info_sensor == INFO_PRESSURE_24H;
    set_visible(objects.pressure_3h_pressure_thresholds, !graph_mode && show_3h);
    set_visible(objects.pressure_24h_pressure_thresholds, !graph_mode && show_24h);
    set_visible(objects.pressure_info_graph, graph_mode && (show_3h || show_24h));
    set_visible(objects.btn_3h_pressure_info, !graph_mode);
    set_visible(objects.btn_24h_pressure_info, !graph_mode);
    sync_threshold_dots_visibility();

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
    set_checked(objects.btn_pressure_range_1h, pressure_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_pressure_range_3h, pressure_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_pressure_range_24h, pressure_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

