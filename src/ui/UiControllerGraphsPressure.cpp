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

void UiController::ensure_pressure_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_pressure_info,
        pressure_graph_label_min_,
        pressure_graph_label_now_,
        pressure_graph_label_max_);
}

void UiController::update_pressure_graph_overlays(bool has_values,
                                                  float min_pressure,
                                                  float max_pressure,
                                                  float latest_pressure) {
    if (!objects.chart_pressure_info) {
        return;
    }

    ensure_pressure_graph_overlays();
    if (!pressure_graph_label_min_ || !pressure_graph_label_now_ || !pressure_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_pressure_info, pressure_graph_label_min_, pressure_graph_label_now_, pressure_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(pressure_graph_label_min_, "MIN --");
        safe_label_set_text(pressure_graph_label_now_, "NOW --");
        safe_label_set_text(pressure_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f hPa", min_pressure);
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f hPa", latest_pressure);
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f hPa", max_pressure);
    safe_label_set_text(pressure_graph_label_min_, min_buf);
    safe_label_set_text(pressure_graph_label_now_, now_buf);
    safe_label_set_text(pressure_graph_label_max_, max_buf);
}

void UiController::ensure_pressure_time_labels() {
    ensure_graph_time_labels(
        objects.pressure_info_graph,
        objects.chart_pressure_info,
        pressure_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_pressure_time_labels() {
    update_graph_time_labels(
        objects.pressure_info_graph,
        objects.chart_pressure_info,
        pressure_graph_time_labels_,
        kTempGraphTimeTickCount,
        pressure_graph_points());
}

void UiController::update_pressure_info_graph() {
    if (!objects.chart_pressure_info) {
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

    lv_chart_set_type(objects.chart_pressure_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_pressure_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (pressure_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_pressure_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_pressure_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_pressure_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_pressure_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_pressure_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_pressure_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pressure_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pressure_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_pressure_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pressure_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pressure_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_pressure_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = pressure_graph_points();
    lv_chart_set_point_count(objects.chart_pressure_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_pressure_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_pressure_info,
                                     lv_obj_get_style_line_color(objects.chart_pressure_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_pressure_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_pressure_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_p = FLT_MAX;
    float max_p = -FLT_MAX;
    float latest_p = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_PRESSURE, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_p = value;
                    max_p = value;
                    has_values = true;
                } else {
                    if (value < min_p) {
                        min_p = value;
                    }
                    if (value > max_p) {
                        max_p = value;
                    }
                }
                latest_p = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_pressure_info, series, i, point_value);
    }

    float scale_min = has_values ? min_p : 1013.0f;
    float scale_max = has_values ? max_p : 1013.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 2.0f) {
        scale_span = 2.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 0.5f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_p) ? latest_p : 1013.0f;
        y_min_f = center - 2.0f;
        y_max_f = center + 2.0f;
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

    lv_chart_set_div_line_count(objects.chart_pressure_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_pressure_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);

    if (has_values) {
        if (!isfinite(latest_p)) {
            latest_p = max_p;
        }
        update_pressure_graph_overlays(true, min_p, max_p, latest_p);
    } else {
        update_pressure_graph_overlays(false, 1013.0f, 1013.0f, 1013.0f);
    }
    update_pressure_time_labels();

    lv_chart_refresh(objects.chart_pressure_info);
}
