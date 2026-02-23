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

void UiController::ensure_pm05_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_pm05_info,
        pm05_graph_label_min_,
        pm05_graph_label_now_,
        pm05_graph_label_max_);
}

void UiController::update_pm05_graph_overlays(bool has_values,
                                              float min_value,
                                              float max_value,
                                              float latest_value) {
    if (!objects.chart_pm05_info) {
        return;
    }

    ensure_pm05_graph_overlays();
    if (!pm05_graph_label_min_ || !pm05_graph_label_now_ || !pm05_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_pm05_info, pm05_graph_label_min_, pm05_graph_label_now_, pm05_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(pm05_graph_label_min_, "MIN --");
        safe_label_set_text(pm05_graph_label_now_, "NOW --");
        safe_label_set_text(pm05_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[48];
    char now_buf[48];
    char max_buf[48];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f #/cm3", min_value);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f #/cm3", latest_value);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f #/cm3", max_value);
    safe_label_set_text(pm05_graph_label_min_, min_buf);
    safe_label_set_text(pm05_graph_label_now_, now_buf);
    safe_label_set_text(pm05_graph_label_max_, max_buf);
}

void UiController::ensure_pm05_zone_overlay() {
    if (!objects.pm05_info_graph || !objects.chart_pm05_info) {
        return;
    }

    if (!pm05_graph_zone_overlay_ || !lv_obj_is_valid(pm05_graph_zone_overlay_) ||
        lv_obj_get_parent(pm05_graph_zone_overlay_) != objects.pm05_info_graph) {
        pm05_graph_zone_overlay_ = lv_obj_create(objects.pm05_info_graph);
        lv_obj_clear_flag(pm05_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(pm05_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(pm05_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(pm05_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(pm05_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(pm05_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(pm05_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_pm05_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_pm05_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_pm05_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_pm05_info);

    lv_obj_set_pos(pm05_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(pm05_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(pm05_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_pm05_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = pm05_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != pm05_graph_zone_overlay_) {
            band = lv_obj_create(pm05_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    lv_obj_move_background(pm05_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_pm05_info);
}

void UiController::update_pm05_zone_overlay(float y_min_display, float y_max_display) {
    ensure_pm05_zone_overlay();
    if (!pm05_graph_zone_overlay_ || !lv_obj_is_valid(pm05_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(pm05_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(pm05_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (pm05_graph_zone_bands_[i]) {
                lv_obj_add_flag(pm05_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    constexpr float kZoneBounds[] = {-100000.0f, 250.0f, 600.0f, 1200.0f, 100000.0f};
    constexpr GraphZoneTone kZoneTones[] = {GRAPH_ZONE_GREEN, GRAPH_ZONE_YELLOW, GRAPH_ZONE_ORANGE, GRAPH_ZONE_RED};
    constexpr uint8_t kZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_pm05_info, LV_PART_MAIN);
    const float span = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kZoneCount; ++i) {
        lv_obj_t *band = pm05_graph_zone_bands_[i];
        if (!band) {
            continue;
        }

        const float lower = kZoneBounds[i];
        const float upper = kZoneBounds[i + 1];
        const float vis_low = fmaxf(lower, y_min_display);
        const float vis_high = fminf(upper, y_max_display);
        if (vis_high <= vis_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float top_ratio = (y_max_display - vis_high) / span;
        const float bottom_ratio = (y_max_display - vis_low) / span;
        lv_coord_t y1 = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t y2 = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (y1 < 0) {
            y1 = 0;
        }
        if (y2 > height) {
            y2 = height;
        }
        if (y2 <= y1) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, y1);
        lv_obj_set_size(band, width, y2 - y1);
        lv_obj_set_style_bg_color(band, resolve_graph_zone_color(kZoneTones[i], chart_bg), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
    }

    for (uint8_t i = kZoneCount; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = pm05_graph_zone_bands_[i];
        if (band) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void UiController::ensure_pm05_time_labels() {
    ensure_graph_time_labels(
        objects.pm05_info_graph,
        objects.chart_pm05_info,
        pm05_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_pm05_time_labels() {
    update_graph_time_labels(
        objects.pm05_info_graph,
        objects.chart_pm05_info,
        pm05_graph_time_labels_,
        kTempGraphTimeTickCount,
        pm05_graph_points(),
        true,
        true,
        false);
}

void UiController::update_pm05_info_graph() {
    if (!objects.chart_pm05_info) {
        return;
    }

    constexpr float kFallbackValue = 250.0f;
    constexpr float kMinSpan = 200.0f;

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_pm05_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_pm05_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (pm05_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_pm05_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_pm05_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_pm05_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_pm05_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_pm05_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_pm05_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pm05_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pm05_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_pm05_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pm05_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pm05_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_pm05_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = pm05_graph_points();
    lv_chart_set_point_count(objects.chart_pm05_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_pm05_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_pm05_info,
                                     lv_obj_get_style_line_color(objects.chart_pm05_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_pm05_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_pm05_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_value = FLT_MAX;
    float max_value = -FLT_MAX;
    float latest_value = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_PM05, value, valid) &&
                valid && isfinite(value) && value >= 0.0f) {
                if (!has_values) {
                    min_value = value;
                    max_value = value;
                    has_values = true;
                } else {
                    if (value < min_value) {
                        min_value = value;
                    }
                    if (value > max_value) {
                        max_value = value;
                    }
                }
                latest_value = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_pm05_info, series, i, point_value);
    }

    float scale_min = has_values ? min_value : 0.0f;
    float scale_max = has_values ? max_value : kFallbackValue;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < kMinSpan) {
        scale_span = kMinSpan;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 100.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_value) ? latest_value : kFallbackValue;
        y_min_f = center - kMinSpan;
        y_max_f = center + kMinSpan;
    }
    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_pm05_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_pm05_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_pm05_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_value)) {
            latest_value = max_value;
        }
        update_pm05_graph_overlays(true, min_value, max_value, latest_value);
    } else {
        update_pm05_graph_overlays(false, kFallbackValue, kFallbackValue, kFallbackValue);
    }
    update_pm05_time_labels();

    lv_chart_refresh(objects.chart_pm05_info);
}

void UiController::ensure_pm25_4_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_pm25_4_graph,
        pm25_4_graph_label_min_,
        pm25_4_graph_label_now_,
        pm25_4_graph_label_max_);
}

void UiController::update_pm25_4_graph_overlays(bool has_values,
                                                float min_value,
                                                float max_value,
                                                float latest_value) {
    if (!objects.chart_pm25_4_graph) {
        return;
    }

    ensure_pm25_4_graph_overlays();
    if (!pm25_4_graph_label_min_ || !pm25_4_graph_label_now_ || !pm25_4_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_pm25_4_graph, pm25_4_graph_label_min_, pm25_4_graph_label_now_, pm25_4_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(pm25_4_graph_label_min_, "MIN --");
        safe_label_set_text(pm25_4_graph_label_now_, "NOW --");
        safe_label_set_text(pm25_4_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f ug/m3", min_value);
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f ug/m3", latest_value);
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f ug/m3", max_value);
    safe_label_set_text(pm25_4_graph_label_min_, min_buf);
    safe_label_set_text(pm25_4_graph_label_now_, now_buf);
    safe_label_set_text(pm25_4_graph_label_max_, max_buf);
}

void UiController::ensure_pm25_4_zone_overlay() {
    if (!objects.pm25_4_graph || !objects.chart_pm25_4_graph) {
        return;
    }

    if (!pm25_4_graph_zone_overlay_ || !lv_obj_is_valid(pm25_4_graph_zone_overlay_) ||
        lv_obj_get_parent(pm25_4_graph_zone_overlay_) != objects.pm25_4_graph) {
        pm25_4_graph_zone_overlay_ = lv_obj_create(objects.pm25_4_graph);
        lv_obj_clear_flag(pm25_4_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(pm25_4_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(pm25_4_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(pm25_4_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(pm25_4_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(pm25_4_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(pm25_4_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_pm25_4_graph);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_pm25_4_graph);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_pm25_4_graph);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_pm25_4_graph);

    lv_obj_set_pos(pm25_4_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(pm25_4_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(pm25_4_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_pm25_4_graph, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = pm25_4_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != pm25_4_graph_zone_overlay_) {
            band = lv_obj_create(pm25_4_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(pm25_4_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_pm25_4_graph);
}

void UiController::update_pm25_4_zone_overlay(float y_min_display, float y_max_display) {
    ensure_pm25_4_zone_overlay();
    if (!pm25_4_graph_zone_overlay_ || !lv_obj_is_valid(pm25_4_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(pm25_4_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(pm25_4_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (pm25_4_graph_zone_bands_[i]) {
                lv_obj_add_flag(pm25_4_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kPm25ZoneBounds[] = {
        -1000.0f,
        Config::AQ_PM25_GREEN_MAX_UGM3,
        Config::AQ_PM25_YELLOW_MAX_UGM3,
        Config::AQ_PM25_ORANGE_MAX_UGM3,
        100000.0f,
    };
    static const float kPm4ZoneBounds[] = {
        -1000.0f,
        Config::AQ_PM4_GREEN_MAX_UGM3,
        Config::AQ_PM4_YELLOW_MAX_UGM3,
        Config::AQ_PM4_ORANGE_MAX_UGM3,
        100000.0f,
    };
    static const GraphZoneTone kPmZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kPmZoneCount = 4;
    const bool pm4_selected = info_sensor == INFO_PM4;
    const float *zone_bounds = pm4_selected ? kPm4ZoneBounds : kPm25ZoneBounds;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_pm25_4_graph, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = pm25_4_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kPmZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = zone_bounds[i];
        const float zone_high = zone_bounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kPmZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_pm25_4_time_labels() {
    ensure_graph_time_labels(
        objects.pm25_4_graph,
        objects.chart_pm25_4_graph,
        pm25_4_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_pm25_4_time_labels() {
    update_graph_time_labels(
        objects.pm25_4_graph,
        objects.chart_pm25_4_graph,
        pm25_4_graph_time_labels_,
        kTempGraphTimeTickCount,
        pm25_4_graph_points());
}

void UiController::update_pm25_4_info_graph() {
    if (!objects.chart_pm25_4_graph) {
        return;
    }

    const bool pm4_selected = info_sensor == INFO_PM4;
    const ChartsHistory::Metric metric = pm4_selected ? ChartsHistory::METRIC_PM4 : ChartsHistory::METRIC_PM25;
    const float fallback_value = pm4_selected ? Config::AQ_PM4_GREEN_MAX_UGM3 : Config::AQ_PM25_GREEN_MAX_UGM3;
    const float min_span = pm4_selected ? 15.0f : 10.0f;

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_pm25_4_graph, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_pm25_4_graph, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (pm25_4_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_pm25_4_graph, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_pm25_4_graph, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_pm25_4_graph, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_pm25_4_graph, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_pm25_4_graph, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_pm25_4_graph, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pm25_4_graph, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pm25_4_graph, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_pm25_4_graph, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pm25_4_graph, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pm25_4_graph, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_pm25_4_graph, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = pm25_4_graph_points();
    lv_chart_set_point_count(objects.chart_pm25_4_graph, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_pm25_4_graph, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_pm25_4_graph,
                                     lv_obj_get_style_line_color(objects.chart_pm25_4_graph, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_pm25_4_graph, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_pm25_4_graph, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_value = FLT_MAX;
    float max_value = -FLT_MAX;
    float latest_value = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, metric, value, valid) &&
                valid && isfinite(value) && value >= 0.0f) {
                if (!has_values) {
                    min_value = value;
                    max_value = value;
                    has_values = true;
                } else {
                    if (value < min_value) {
                        min_value = value;
                    }
                    if (value > max_value) {
                        max_value = value;
                    }
                }
                latest_value = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_pm25_4_graph, series, i, point_value);
    }

    float scale_min = has_values ? min_value : 0.0f;
    float scale_max = has_values ? max_value : fallback_value;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < min_span) {
        scale_span = min_span;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 5.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_value) ? latest_value : fallback_value;
        y_min_f = center - min_span;
        y_max_f = center + min_span;
    }
    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_pm25_4_graph,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_pm25_4_graph, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_pm25_4_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_value)) {
            latest_value = max_value;
        }
        update_pm25_4_graph_overlays(true, min_value, max_value, latest_value);
    } else {
        update_pm25_4_graph_overlays(false, fallback_value, fallback_value, fallback_value);
    }
    update_pm25_4_time_labels();

    lv_chart_refresh(objects.chart_pm25_4_graph);
}

void UiController::ensure_pm1_10_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_pm1_10_info,
        pm1_10_graph_label_min_,
        pm1_10_graph_label_now_,
        pm1_10_graph_label_max_);
}

void UiController::update_pm1_10_graph_overlays(bool has_values,
                                                float min_value,
                                                float max_value,
                                                float latest_value) {
    if (!objects.chart_pm1_10_info) {
        return;
    }

    ensure_pm1_10_graph_overlays();
    if (!pm1_10_graph_label_min_ || !pm1_10_graph_label_now_ || !pm1_10_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_pm1_10_info, pm1_10_graph_label_min_, pm1_10_graph_label_now_, pm1_10_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(pm1_10_graph_label_min_, "MIN --");
        safe_label_set_text(pm1_10_graph_label_now_, "NOW --");
        safe_label_set_text(pm1_10_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f ug/m3", min_value);
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f ug/m3", latest_value);
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f ug/m3", max_value);
    safe_label_set_text(pm1_10_graph_label_min_, min_buf);
    safe_label_set_text(pm1_10_graph_label_now_, now_buf);
    safe_label_set_text(pm1_10_graph_label_max_, max_buf);
}

void UiController::ensure_pm1_10_zone_overlay() {
    if (!objects.pm1_10_info_graph || !objects.chart_pm1_10_info) {
        return;
    }

    if (!pm1_10_graph_zone_overlay_ || !lv_obj_is_valid(pm1_10_graph_zone_overlay_) ||
        lv_obj_get_parent(pm1_10_graph_zone_overlay_) != objects.pm1_10_info_graph) {
        pm1_10_graph_zone_overlay_ = lv_obj_create(objects.pm1_10_info_graph);
        lv_obj_clear_flag(pm1_10_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(pm1_10_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(pm1_10_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(pm1_10_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(pm1_10_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(pm1_10_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(pm1_10_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_pm1_10_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_pm1_10_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_pm1_10_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_pm1_10_info);

    lv_obj_set_pos(pm1_10_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(pm1_10_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(pm1_10_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_pm1_10_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = pm1_10_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != pm1_10_graph_zone_overlay_) {
            band = lv_obj_create(pm1_10_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(pm1_10_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_pm1_10_info);
}

void UiController::update_pm1_10_zone_overlay(float y_min_display, float y_max_display) {
    ensure_pm1_10_zone_overlay();
    if (!pm1_10_graph_zone_overlay_ || !lv_obj_is_valid(pm1_10_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(pm1_10_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(pm1_10_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (pm1_10_graph_zone_bands_[i]) {
                lv_obj_add_flag(pm1_10_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kPm1ZoneBounds[] = {-1000.0f, 10.0f, 25.0f, 50.0f, 100000.0f};
    static const float kPm10ZoneBounds[] = {-1000.0f, 54.0f, 154.0f, 254.0f, 100000.0f};
    static const GraphZoneTone kPmZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kPmZoneCount = 4;
    const bool pm10_selected = info_sensor == INFO_PM10;
    const float *zone_bounds = pm10_selected ? kPm10ZoneBounds : kPm1ZoneBounds;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_pm1_10_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = pm1_10_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kPmZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = zone_bounds[i];
        const float zone_high = zone_bounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kPmZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_pm1_10_time_labels() {
    ensure_graph_time_labels(
        objects.pm1_10_info_graph,
        objects.chart_pm1_10_info,
        pm1_10_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_pm1_10_time_labels() {
    update_graph_time_labels(
        objects.pm1_10_info_graph,
        objects.chart_pm1_10_info,
        pm1_10_graph_time_labels_,
        kTempGraphTimeTickCount,
        pm1_10_graph_points());
}

void UiController::update_pm1_10_info_graph() {
    if (!objects.chart_pm1_10_info) {
        return;
    }

    const bool pm10_selected = info_sensor == INFO_PM10;
    const ChartsHistory::Metric metric = pm10_selected ? ChartsHistory::METRIC_PM10 : ChartsHistory::METRIC_PM1;
    const float fallback_value = pm10_selected ? 54.0f : 10.0f;
    const float min_span = pm10_selected ? 20.0f : 8.0f;

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_pm1_10_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_pm1_10_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (pm1_10_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_pm1_10_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_pm1_10_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_pm1_10_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_pm1_10_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_pm1_10_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_pm1_10_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pm1_10_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pm1_10_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_pm1_10_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pm1_10_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pm1_10_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_pm1_10_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = pm1_10_graph_points();
    lv_chart_set_point_count(objects.chart_pm1_10_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_pm1_10_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_pm1_10_info,
                                     lv_obj_get_style_line_color(objects.chart_pm1_10_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_pm1_10_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_pm1_10_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_value = FLT_MAX;
    float max_value = -FLT_MAX;
    float latest_value = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, metric, value, valid) &&
                valid && isfinite(value) && value >= 0.0f) {
                if (!has_values) {
                    min_value = value;
                    max_value = value;
                    has_values = true;
                } else {
                    if (value < min_value) {
                        min_value = value;
                    }
                    if (value > max_value) {
                        max_value = value;
                    }
                }
                latest_value = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_pm1_10_info, series, i, point_value);
    }

    float scale_min = has_values ? min_value : 0.0f;
    float scale_max = has_values ? max_value : fallback_value;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < min_span) {
        scale_span = min_span;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = pm10_selected ? 10.0f : 2.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_value) ? latest_value : fallback_value;
        y_min_f = center - min_span;
        y_max_f = center + min_span;
    }
    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_pm1_10_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_pm1_10_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_pm1_10_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_value)) {
            latest_value = max_value;
        }
        update_pm1_10_graph_overlays(true, min_value, max_value, latest_value);
    } else {
        update_pm1_10_graph_overlays(false, fallback_value, fallback_value, fallback_value);
    }
    update_pm1_10_time_labels();

    lv_chart_refresh(objects.chart_pm1_10_info);
}

