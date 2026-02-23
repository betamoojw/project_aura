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

void UiController::ensure_voc_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_voc_info,
        voc_graph_label_min_,
        voc_graph_label_now_,
        voc_graph_label_max_);
}

void UiController::update_voc_graph_overlays(bool has_values,
                                             float min_voc,
                                             float max_voc,
                                             float latest_voc) {
    if (!objects.chart_voc_info) {
        return;
    }

    ensure_voc_graph_overlays();
    if (!voc_graph_label_min_ || !voc_graph_label_now_ || !voc_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_voc_info, voc_graph_label_min_, voc_graph_label_now_, voc_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(voc_graph_label_min_, "MIN --");
        safe_label_set_text(voc_graph_label_now_, "NOW --");
        safe_label_set_text(voc_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f idx", min_voc);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f idx", latest_voc);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f idx", max_voc);
    safe_label_set_text(voc_graph_label_min_, min_buf);
    safe_label_set_text(voc_graph_label_now_, now_buf);
    safe_label_set_text(voc_graph_label_max_, max_buf);
}

void UiController::ensure_voc_zone_overlay() {
    if (!objects.voc_info_graph || !objects.chart_voc_info) {
        return;
    }

    if (!voc_graph_zone_overlay_ || !lv_obj_is_valid(voc_graph_zone_overlay_) ||
        lv_obj_get_parent(voc_graph_zone_overlay_) != objects.voc_info_graph) {
        voc_graph_zone_overlay_ = lv_obj_create(objects.voc_info_graph);
        lv_obj_clear_flag(voc_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(voc_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_voc_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_voc_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_voc_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_voc_info);

    lv_obj_set_pos(voc_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(voc_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(voc_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_voc_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = voc_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != voc_graph_zone_overlay_) {
            band = lv_obj_create(voc_graph_zone_overlay_);
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

    lv_obj_move_background(voc_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_voc_info);
}

void UiController::update_voc_zone_overlay(float y_min_display, float y_max_display) {
    ensure_voc_zone_overlay();
    if (!voc_graph_zone_overlay_ || !lv_obj_is_valid(voc_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(voc_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(voc_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (voc_graph_zone_bands_[i]) {
                lv_obj_add_flag(voc_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kVocZoneBounds[] = {
        -1000.0f,
        static_cast<float>(Config::AQ_VOC_GREEN_MAX_INDEX),
        static_cast<float>(Config::AQ_VOC_YELLOW_MAX_INDEX),
        static_cast<float>(Config::AQ_VOC_ORANGE_MAX_INDEX),
        100000.0f};
    static const GraphZoneTone kVocZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kVocZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_voc_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = voc_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kVocZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kVocZoneBounds[i];
        const float zone_high = kVocZoneBounds[i + 1];
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
        const lv_color_t zone_color = resolve_graph_zone_color(kVocZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_voc_time_labels() {
    ensure_graph_time_labels(
        objects.voc_info_graph,
        objects.chart_voc_info,
        voc_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_voc_time_labels() {
    update_graph_time_labels(
        objects.voc_info_graph,
        objects.chart_voc_info,
        voc_graph_time_labels_,
        kTempGraphTimeTickCount,
        voc_graph_points());
}

void UiController::update_voc_info_graph() {
    if (!objects.chart_voc_info) {
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

    lv_chart_set_type(objects.chart_voc_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_voc_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (voc_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_voc_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_voc_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_voc_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_voc_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_voc_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_voc_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_voc_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_voc_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_voc_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_voc_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_voc_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_voc_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = voc_graph_points();
    lv_chart_set_point_count(objects.chart_voc_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_voc_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_voc_info,
                                     lv_obj_get_style_line_color(objects.chart_voc_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_voc_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_voc_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_voc = FLT_MAX;
    float max_voc = -FLT_MAX;
    float latest_voc = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_VOC, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_voc = value;
                    max_voc = value;
                    has_values = true;
                } else {
                    if (value < min_voc) {
                        min_voc = value;
                    }
                    if (value > max_voc) {
                        max_voc = value;
                    }
                }
                latest_voc = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_voc_info, series, i, point_value);
    }

    float scale_min = has_values ? min_voc : 100.0f;
    float scale_max = has_values ? max_voc : 100.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 80.0f) {
        scale_span = 80.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 25.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_voc) ? latest_voc : 100.0f;
        y_min_f = center - 80.0f;
        y_max_f = center + 80.0f;
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

    lv_chart_set_div_line_count(objects.chart_voc_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_voc_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_voc_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_voc)) {
            latest_voc = max_voc;
        }
        update_voc_graph_overlays(true, min_voc, max_voc, latest_voc);
    } else {
        update_voc_graph_overlays(false, 100.0f, 100.0f, 100.0f);
    }
    update_voc_time_labels();

    lv_chart_refresh(objects.chart_voc_info);
}

void UiController::ensure_nox_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_nox_info,
        nox_graph_label_min_,
        nox_graph_label_now_,
        nox_graph_label_max_);
}

void UiController::update_nox_graph_overlays(bool has_values,
                                             float min_nox,
                                             float max_nox,
                                             float latest_nox) {
    if (!objects.chart_nox_info) {
        return;
    }

    ensure_nox_graph_overlays();
    if (!nox_graph_label_min_ || !nox_graph_label_now_ || !nox_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_nox_info, nox_graph_label_min_, nox_graph_label_now_, nox_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(nox_graph_label_min_, "MIN --");
        safe_label_set_text(nox_graph_label_now_, "NOW --");
        safe_label_set_text(nox_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f idx", min_nox);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f idx", latest_nox);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f idx", max_nox);
    safe_label_set_text(nox_graph_label_min_, min_buf);
    safe_label_set_text(nox_graph_label_now_, now_buf);
    safe_label_set_text(nox_graph_label_max_, max_buf);
}

void UiController::ensure_nox_zone_overlay() {
    if (!objects.nox_info_graph || !objects.chart_nox_info) {
        return;
    }

    if (!nox_graph_zone_overlay_ || !lv_obj_is_valid(nox_graph_zone_overlay_) ||
        lv_obj_get_parent(nox_graph_zone_overlay_) != objects.nox_info_graph) {
        nox_graph_zone_overlay_ = lv_obj_create(objects.nox_info_graph);
        lv_obj_clear_flag(nox_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(nox_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_nox_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_nox_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_nox_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_nox_info);

    lv_obj_set_pos(nox_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(nox_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(nox_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_nox_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = nox_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != nox_graph_zone_overlay_) {
            band = lv_obj_create(nox_graph_zone_overlay_);
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

    lv_obj_move_background(nox_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_nox_info);
}

void UiController::update_nox_zone_overlay(float y_min_display, float y_max_display) {
    ensure_nox_zone_overlay();
    if (!nox_graph_zone_overlay_ || !lv_obj_is_valid(nox_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(nox_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(nox_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (nox_graph_zone_bands_[i]) {
                lv_obj_add_flag(nox_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kNoxZoneBounds[] = {
        -1000.0f,
        static_cast<float>(Config::AQ_NOX_GREEN_MAX_INDEX),
        static_cast<float>(Config::AQ_NOX_YELLOW_MAX_INDEX),
        static_cast<float>(Config::AQ_NOX_ORANGE_MAX_INDEX),
        100000.0f};
    static const GraphZoneTone kNoxZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kNoxZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_nox_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = nox_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kNoxZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kNoxZoneBounds[i];
        const float zone_high = kNoxZoneBounds[i + 1];
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
        const lv_color_t zone_color = resolve_graph_zone_color(kNoxZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_nox_time_labels() {
    ensure_graph_time_labels(
        objects.nox_info_graph,
        objects.chart_nox_info,
        nox_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_nox_time_labels() {
    update_graph_time_labels(
        objects.nox_info_graph,
        objects.chart_nox_info,
        nox_graph_time_labels_,
        kTempGraphTimeTickCount,
        nox_graph_points());
}

void UiController::update_nox_info_graph() {
    if (!objects.chart_nox_info) {
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

    lv_chart_set_type(objects.chart_nox_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_nox_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (nox_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_nox_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_nox_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_nox_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_nox_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_nox_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_nox_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_nox_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_nox_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_nox_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_nox_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_nox_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_nox_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = nox_graph_points();
    lv_chart_set_point_count(objects.chart_nox_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_nox_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_nox_info,
                                     lv_obj_get_style_line_color(objects.chart_nox_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_nox_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_nox_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_nox = FLT_MAX;
    float max_nox = -FLT_MAX;
    float latest_nox = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_NOX, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_nox = value;
                    max_nox = value;
                    has_values = true;
                } else {
                    if (value < min_nox) {
                        min_nox = value;
                    }
                    if (value > max_nox) {
                        max_nox = value;
                    }
                }
                latest_nox = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_nox_info, series, i, point_value);
    }

    float scale_min = has_values ? min_nox : 50.0f;
    float scale_max = has_values ? max_nox : 50.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 60.0f) {
        scale_span = 60.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 20.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_nox) ? latest_nox : 50.0f;
        y_min_f = center - 60.0f;
        y_max_f = center + 60.0f;
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

    lv_chart_set_div_line_count(objects.chart_nox_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_nox_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_nox_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_nox)) {
            latest_nox = max_nox;
        }
        update_nox_graph_overlays(true, min_nox, max_nox, latest_nox);
    } else {
        update_nox_graph_overlays(false, 50.0f, 50.0f, 50.0f);
    }
    update_nox_time_labels();

    lv_chart_refresh(objects.chart_nox_info);
}

void UiController::ensure_hcho_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_hcho_info,
        hcho_graph_label_min_,
        hcho_graph_label_now_,
        hcho_graph_label_max_);
}

void UiController::update_hcho_graph_overlays(bool has_values,
                                              float min_hcho,
                                              float max_hcho,
                                              float latest_hcho) {
    if (!objects.chart_hcho_info) {
        return;
    }

    ensure_hcho_graph_overlays();
    if (!hcho_graph_label_min_ || !hcho_graph_label_now_ || !hcho_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_hcho_info, hcho_graph_label_min_, hcho_graph_label_now_, hcho_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(hcho_graph_label_min_, "MIN --");
        safe_label_set_text(hcho_graph_label_now_, "NOW --");
        safe_label_set_text(hcho_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f ppb", min_hcho);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f ppb", latest_hcho);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f ppb", max_hcho);
    safe_label_set_text(hcho_graph_label_min_, min_buf);
    safe_label_set_text(hcho_graph_label_now_, now_buf);
    safe_label_set_text(hcho_graph_label_max_, max_buf);
}

void UiController::ensure_hcho_zone_overlay() {
    if (!objects.hcho_info_graph || !objects.chart_hcho_info) {
        return;
    }

    if (!hcho_graph_zone_overlay_ || !lv_obj_is_valid(hcho_graph_zone_overlay_) ||
        lv_obj_get_parent(hcho_graph_zone_overlay_) != objects.hcho_info_graph) {
        hcho_graph_zone_overlay_ = lv_obj_create(objects.hcho_info_graph);
        lv_obj_clear_flag(hcho_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(hcho_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_hcho_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_hcho_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_hcho_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_hcho_info);

    lv_obj_set_pos(hcho_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(hcho_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(hcho_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_hcho_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = hcho_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != hcho_graph_zone_overlay_) {
            band = lv_obj_create(hcho_graph_zone_overlay_);
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

    lv_obj_move_background(hcho_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_hcho_info);
}

void UiController::update_hcho_zone_overlay(float y_min_display, float y_max_display) {
    ensure_hcho_zone_overlay();
    if (!hcho_graph_zone_overlay_ || !lv_obj_is_valid(hcho_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(hcho_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(hcho_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (hcho_graph_zone_bands_[i]) {
                lv_obj_add_flag(hcho_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kHchoZoneBounds[] = {
        -1000.0f,
        30.0f,
        60.0f,
        100.0f,
        100000.0f};
    static const GraphZoneTone kHchoZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kHchoZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_hcho_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = hcho_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kHchoZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kHchoZoneBounds[i];
        const float zone_high = kHchoZoneBounds[i + 1];
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
        const lv_color_t zone_color = resolve_graph_zone_color(kHchoZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_hcho_time_labels() {
    ensure_graph_time_labels(
        objects.hcho_info_graph,
        objects.chart_hcho_info,
        hcho_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_hcho_time_labels() {
    update_graph_time_labels(
        objects.hcho_info_graph,
        objects.chart_hcho_info,
        hcho_graph_time_labels_,
        kTempGraphTimeTickCount,
        hcho_graph_points());
}

void UiController::update_hcho_info_graph() {
    if (!objects.chart_hcho_info) {
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

    lv_chart_set_type(objects.chart_hcho_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_hcho_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (hcho_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_hcho_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_hcho_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_hcho_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_hcho_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_hcho_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_hcho_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_hcho_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_hcho_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_hcho_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_hcho_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_hcho_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_hcho_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = hcho_graph_points();
    lv_chart_set_point_count(objects.chart_hcho_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_hcho_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_hcho_info,
                                     lv_obj_get_style_line_color(objects.chart_hcho_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_hcho_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_hcho_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_hcho = FLT_MAX;
    float max_hcho = -FLT_MAX;
    float latest_hcho = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_HCHO, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_hcho = value;
                    max_hcho = value;
                    has_values = true;
                } else {
                    if (value < min_hcho) {
                        min_hcho = value;
                    }
                    if (value > max_hcho) {
                        max_hcho = value;
                    }
                }
                latest_hcho = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_hcho_info, series, i, point_value);
    }

    float scale_min = has_values ? min_hcho : 20.0f;
    float scale_max = has_values ? max_hcho : 20.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 40.0f) {
        scale_span = 40.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 10.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_hcho) ? latest_hcho : 20.0f;
        y_min_f = center - 40.0f;
        y_max_f = center + 40.0f;
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

    lv_chart_set_div_line_count(objects.chart_hcho_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_hcho_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_hcho_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_hcho)) {
            latest_hcho = max_hcho;
        }
        update_hcho_graph_overlays(true, min_hcho, max_hcho, latest_hcho);
    } else {
        update_hcho_graph_overlays(false, 20.0f, 20.0f, 20.0f);
    }
    update_hcho_time_labels();

    lv_chart_refresh(objects.chart_hcho_info);
}

void UiController::ensure_co2_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_co2_info,
        co2_graph_label_min_,
        co2_graph_label_now_,
        co2_graph_label_max_);
}

void UiController::update_co2_graph_overlays(bool has_values,
                                             float min_co2,
                                             float max_co2,
                                             float latest_co2) {
    if (!objects.chart_co2_info) {
        return;
    }

    ensure_co2_graph_overlays();
    if (!co2_graph_label_min_ || !co2_graph_label_now_ || !co2_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_co2_info, co2_graph_label_min_, co2_graph_label_now_, co2_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(co2_graph_label_min_, "MIN --");
        safe_label_set_text(co2_graph_label_now_, "NOW --");
        safe_label_set_text(co2_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f ppm", min_co2);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f ppm", latest_co2);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f ppm", max_co2);
    safe_label_set_text(co2_graph_label_min_, min_buf);
    safe_label_set_text(co2_graph_label_now_, now_buf);
    safe_label_set_text(co2_graph_label_max_, max_buf);
}

void UiController::ensure_co2_zone_overlay() {
    if (!objects.co2_info_graph || !objects.chart_co2_info) {
        return;
    }

    if (!co2_graph_zone_overlay_ || !lv_obj_is_valid(co2_graph_zone_overlay_) ||
        lv_obj_get_parent(co2_graph_zone_overlay_) != objects.co2_info_graph) {
        co2_graph_zone_overlay_ = lv_obj_create(objects.co2_info_graph);
        lv_obj_clear_flag(co2_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(co2_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_co2_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_co2_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_co2_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_co2_info);

    lv_obj_set_pos(co2_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(co2_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(co2_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_co2_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = co2_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != co2_graph_zone_overlay_) {
            band = lv_obj_create(co2_graph_zone_overlay_);
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

    lv_obj_move_background(co2_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_co2_info);
}

void UiController::update_co2_zone_overlay(float y_min_display, float y_max_display) {
    ensure_co2_zone_overlay();
    if (!co2_graph_zone_overlay_ || !lv_obj_is_valid(co2_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(co2_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(co2_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (co2_graph_zone_bands_[i]) {
                lv_obj_add_flag(co2_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kCo2ZoneBounds[] = {-1000.0f, 800.0f, 1000.0f, 1500.0f, 100000.0f};
    static const GraphZoneTone kCo2ZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kCo2ZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_co2_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = co2_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kCo2ZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kCo2ZoneBounds[i];
        const float zone_high = kCo2ZoneBounds[i + 1];
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
        const lv_color_t zone_color = resolve_graph_zone_color(kCo2ZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_co2_time_labels() {
    ensure_graph_time_labels(
        objects.co2_info_graph,
        objects.chart_co2_info,
        co2_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_co2_time_labels() {
    update_graph_time_labels(
        objects.co2_info_graph,
        objects.chart_co2_info,
        co2_graph_time_labels_,
        kTempGraphTimeTickCount,
        co2_graph_points());
}

void UiController::update_co2_info_graph() {
    if (!objects.chart_co2_info) {
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

    lv_chart_set_type(objects.chart_co2_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_co2_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (co2_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_co2_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_co2_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_co2_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_co2_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_co2_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_co2_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co2_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co2_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_co2_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co2_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co2_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_co2_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = co2_graph_points();
    lv_chart_set_point_count(objects.chart_co2_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_co2_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_co2_info,
                                     lv_obj_get_style_line_color(objects.chart_co2_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_co2_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_co2_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_co2 = FLT_MAX;
    float max_co2 = -FLT_MAX;
    float latest_co2 = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_CO2, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_co2 = value;
                    max_co2 = value;
                    has_values = true;
                } else {
                    if (value < min_co2) {
                        min_co2 = value;
                    }
                    if (value > max_co2) {
                        max_co2 = value;
                    }
                }
                latest_co2 = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_co2_info, series, i, point_value);
    }

    float scale_min = has_values ? min_co2 : 700.0f;
    float scale_max = has_values ? max_co2 : 700.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 150.0f) {
        scale_span = 150.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 50.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_co2) ? latest_co2 : 700.0f;
        y_min_f = center - 150.0f;
        y_max_f = center + 150.0f;
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

    lv_chart_set_div_line_count(objects.chart_co2_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_co2_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_co2_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_co2)) {
            latest_co2 = max_co2;
        }
        update_co2_graph_overlays(true, min_co2, max_co2, latest_co2);
    } else {
        update_co2_graph_overlays(false, 700.0f, 700.0f, 700.0f);
    }
    update_co2_time_labels();

    lv_chart_refresh(objects.chart_co2_info);
}

void UiController::ensure_co_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_co_info,
        co_graph_label_min_,
        co_graph_label_now_,
        co_graph_label_max_);
}

void UiController::update_co_graph_overlays(bool has_values,
                                            float min_co,
                                            float max_co,
                                            float latest_co) {
    if (!objects.chart_co_info) {
        return;
    }

    ensure_co_graph_overlays();
    if (!co_graph_label_min_ || !co_graph_label_now_ || !co_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(objects.chart_co_info, co_graph_label_min_, co_graph_label_now_, co_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(co_graph_label_min_, "MIN --");
        safe_label_set_text(co_graph_label_now_, "NOW --");
        safe_label_set_text(co_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f ppm", min_co);
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f ppm", latest_co);
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f ppm", max_co);
    safe_label_set_text(co_graph_label_min_, min_buf);
    safe_label_set_text(co_graph_label_now_, now_buf);
    safe_label_set_text(co_graph_label_max_, max_buf);
}

void UiController::ensure_co_zone_overlay() {
    if (!objects.co_info_graph || !objects.chart_co_info) {
        return;
    }

    if (!co_graph_zone_overlay_ || !lv_obj_is_valid(co_graph_zone_overlay_) ||
        lv_obj_get_parent(co_graph_zone_overlay_) != objects.co_info_graph) {
        co_graph_zone_overlay_ = lv_obj_create(objects.co_info_graph);
        lv_obj_clear_flag(co_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(co_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_co_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_co_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_co_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_co_info);

    lv_obj_set_pos(co_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(co_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(co_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_co_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = co_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != co_graph_zone_overlay_) {
            band = lv_obj_create(co_graph_zone_overlay_);
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

    lv_obj_move_background(co_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_co_info);
}

void UiController::update_co_zone_overlay(float y_min_display, float y_max_display) {
    ensure_co_zone_overlay();
    if (!co_graph_zone_overlay_ || !lv_obj_is_valid(co_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(co_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(co_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (co_graph_zone_bands_[i]) {
                lv_obj_add_flag(co_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kCoZoneBounds[] = {
        -1000.0f,
        Config::AQ_CO_GREEN_MAX_PPM,
        Config::AQ_CO_YELLOW_MAX_PPM,
        Config::AQ_CO_ORANGE_MAX_PPM,
        100000.0f,
    };
    static const GraphZoneTone kCoZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kCoZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_co_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = co_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kCoZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kCoZoneBounds[i];
        const float zone_high = kCoZoneBounds[i + 1];
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
        const lv_color_t zone_color = resolve_graph_zone_color(kCoZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_co_time_labels() {
    ensure_graph_time_labels(
        objects.co_info_graph,
        objects.chart_co_info,
        co_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_co_time_labels() {
    update_graph_time_labels(
        objects.co_info_graph,
        objects.chart_co_info,
        co_graph_time_labels_,
        kTempGraphTimeTickCount,
        co_graph_points());
}

void UiController::update_co_info_graph() {
    if (!objects.chart_co_info) {
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

    lv_chart_set_type(objects.chart_co_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_co_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (co_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_co_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_co_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_co_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_co_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_co_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_co_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_co_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_co_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = co_graph_points();
    lv_chart_set_point_count(objects.chart_co_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_co_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_co_info,
                                     lv_obj_get_style_line_color(objects.chart_co_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_co_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_co_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_co = FLT_MAX;
    float max_co = -FLT_MAX;
    float latest_co = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_CO, value, valid) &&
                valid && isfinite(value) && value >= 0.0f) {
                if (!has_values) {
                    min_co = value;
                    max_co = value;
                    has_values = true;
                } else {
                    if (value < min_co) {
                        min_co = value;
                    }
                    if (value > max_co) {
                        max_co = value;
                    }
                }
                latest_co = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_co_info, series, i, point_value);
    }

    float scale_min = has_values ? min_co : 0.0f;
    float scale_max = has_values ? max_co : Config::AQ_CO_GREEN_MAX_PPM;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 5.0f) {
        scale_span = 5.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 1.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_co) ? latest_co : (Config::AQ_CO_GREEN_MAX_PPM * 0.5f);
        y_min_f = center - 5.0f;
        y_max_f = center + 5.0f;
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

    lv_chart_set_div_line_count(objects.chart_co_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_co_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_co_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_co)) {
            latest_co = max_co;
        }
        update_co_graph_overlays(true, min_co, max_co, latest_co);
    } else {
        const float fallback = Config::AQ_CO_GREEN_MAX_PPM * 0.5f;
        update_co_graph_overlays(false, fallback, fallback, fallback);
    }
    update_co_time_labels();

    lv_chart_refresh(objects.chart_co_info);
}

