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
    const float min_display = pressure_to_display(min_pressure);
    const float now_display = pressure_to_display(latest_pressure);
    const float max_display = pressure_to_display(max_pressure);
    const char *unit = pressure_display_unit();
    if (pressure_display_uses_inhg()) {
        snprintf(min_buf, sizeof(min_buf), "MIN %.2f %s", min_display, unit);
        snprintf(now_buf, sizeof(now_buf), "NOW %.2f %s", now_display, unit);
        snprintf(max_buf, sizeof(max_buf), "MAX %.2f %s", max_display, unit);
    } else {
        snprintf(min_buf, sizeof(min_buf), "MIN %.1f %s", min_display, unit);
        snprintf(now_buf, sizeof(now_buf), "NOW %.1f %s", now_display, unit);
        snprintf(max_buf, sizeof(max_buf), "MAX %.1f %s", max_display, unit);
    }
    safe_label_set_text(pressure_graph_label_min_, min_buf);
    safe_label_set_text(pressure_graph_label_now_, now_buf);
    safe_label_set_text(pressure_graph_label_max_, max_buf);
}

void UiController::ensure_pressure_time_labels() {
    ensure_graph_time_labels(
        objects.pressure_info_graph,
        objects.chart_pressure_info,
        pressure_graph_time_labels_,
        kGraphTimeTickCount);
}

void UiController::update_pressure_time_labels() {
    update_graph_time_labels(
        objects.pressure_info_graph,
        objects.chart_pressure_info,
        pressure_graph_time_labels_,
        kGraphTimeTickCount,
        pressure_graph_points());
}

void UiController::update_pressure_info_graph() {
    if (!objects.chart_pressure_info) {
        return;
    }

    const uint8_t vertical_divisions = graph_vertical_divisions_for_range(pressure_graph_range_);
    apply_standard_info_chart_theme(objects.chart_pressure_info, 5, vertical_divisions);

    const uint16_t points = pressure_graph_points();
    lv_chart_series_t *series = ensure_info_chart_series(objects.chart_pressure_info, points);
    if (!series) {
        return;
    }

    const GraphSeriesStats stats = populate_info_chart_series(objects.chart_pressure_info,
                                                              series,
                                                              points,
                                                              static_cast<int>(ChartsHistory::METRIC_PRESSURE),
                                                              10.0f,
                                                              false);
    const bool has_values = stats.has_values;
    float min_p = stats.min_value;
    float max_p = stats.max_value;
    float latest_p = stats.latest_value;

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
    mark_active_graph_refreshed(info_sensor, pressure_graph_range_, points);
}
