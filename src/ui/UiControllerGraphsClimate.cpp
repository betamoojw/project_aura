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

void UiController::apply_temperature_graph_theme(const SensorGraphProfile &profile) {
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
    const uint8_t initial_horizontal = (profile.horizontal_divisions_min > 0) ? profile.horizontal_divisions_min : 3;
    const uint8_t initial_vertical = (profile.vertical_divisions > 0) ? profile.vertical_divisions : 15;
    lv_chart_set_div_line_count(objects.chart_temp_info, initial_horizontal, initial_vertical);

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
    ensure_graph_stat_overlays(
        objects.chart_temp_info,
        temp_graph_label_min_,
        temp_graph_label_now_,
        temp_graph_label_max_);
}

void UiController::update_temperature_graph_overlays(const SensorGraphProfile &profile,
                                                     bool has_values,
                                                     float min_temp,
                                                     float max_temp,
                                                     float latest_temp) {
    if (!objects.chart_temp_info) {
        return;
    }

    ensure_temperature_graph_overlays();
    if (!temp_graph_label_min_ || !temp_graph_label_now_ || !temp_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(
        objects.chart_temp_info,
        temp_graph_label_min_,
        temp_graph_label_now_,
        temp_graph_label_max_);

    if (!has_values) {
        char min_empty[24];
        char now_empty[24];
        char max_empty[24];
        snprintf(min_empty, sizeof(min_empty), "%s --", profile.label_min ? profile.label_min : "MIN");
        snprintf(now_empty, sizeof(now_empty), "%s --", profile.label_now ? profile.label_now : "NOW");
        snprintf(max_empty, sizeof(max_empty), "%s --", profile.label_max ? profile.label_max : "MAX");
        safe_label_set_text(temp_graph_label_min_, min_empty);
        safe_label_set_text(temp_graph_label_now_, now_empty);
        safe_label_set_text(temp_graph_label_max_, max_empty);
        return;
    }

    const char *unit = profile.unit;
    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "%s %.1f%s", profile.label_min ? profile.label_min : "MIN", min_temp, unit ? unit : "");
    snprintf(now_buf, sizeof(now_buf), "%s %.1f%s", profile.label_now ? profile.label_now : "NOW", latest_temp, unit ? unit : "");
    snprintf(max_buf, sizeof(max_buf), "%s %.1f%s", profile.label_max ? profile.label_max : "MAX", max_temp, unit ? unit : "");
    safe_label_set_text(temp_graph_label_min_, min_buf);
    safe_label_set_text(temp_graph_label_now_, now_buf);
    safe_label_set_text(temp_graph_label_max_, max_buf);
}

void UiController::ensure_temperature_zone_overlay() {
    if (!objects.temperature_info_graph || !objects.chart_temp_info) {
        return;
    }

    if (!temp_graph_zone_overlay_ || !lv_obj_is_valid(temp_graph_zone_overlay_) ||
        lv_obj_get_parent(temp_graph_zone_overlay_) != objects.temperature_info_graph) {
        temp_graph_zone_overlay_ = lv_obj_create(objects.temperature_info_graph);
        lv_obj_clear_flag(temp_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(temp_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_temp_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_temp_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_temp_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_temp_info);

    lv_obj_set_pos(temp_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(temp_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(temp_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_temp_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = temp_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != temp_graph_zone_overlay_) {
            band = lv_obj_create(temp_graph_zone_overlay_);
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

    lv_obj_move_background(temp_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_temp_info);
}

void UiController::update_temperature_zone_overlay(const SensorGraphProfile &profile,
                                                   float y_min_display,
                                                   float y_max_display) {
    ensure_temperature_zone_overlay();
    if (!temp_graph_zone_overlay_ || !lv_obj_is_valid(temp_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(temp_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(temp_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (temp_graph_zone_bands_[i]) {
                lv_obj_add_flag(temp_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_temp_info, LV_PART_MAIN);
    uint8_t zone_count = profile.zone_count;
    if (zone_count > kMaxGraphZoneBands) {
        zone_count = kMaxGraphZoneBands;
    }
    if (zone_count == 0) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (temp_graph_zone_bands_[i]) {
                lv_obj_add_flag(temp_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    const float denom = y_max_display - y_min_display;
    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = temp_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= zone_count) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = profile.zone_bounds[i];
        const float zone_high = profile.zone_bounds[i + 1];
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
        lv_color_t zone_color = resolve_graph_zone_color(profile.zone_tones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_temperature_time_labels() {
    ensure_graph_time_labels(
        objects.temperature_info_graph,
        objects.chart_temp_info,
        temp_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_temperature_time_labels() {
    update_graph_time_labels(
        objects.temperature_info_graph,
        objects.chart_temp_info,
        temp_graph_time_labels_,
        kTempGraphTimeTickCount,
        temperature_graph_points());
}

void UiController::ensure_humidity_graph_overlays() {
    ensure_graph_stat_overlays(
        objects.chart_rh_info,
        rh_graph_label_min_,
        rh_graph_label_now_,
        rh_graph_label_max_);
}

void UiController::update_humidity_graph_overlays(bool has_values,
                                                  float min_humidity,
                                                  float max_humidity,
                                                  float latest_humidity) {
    if (!objects.chart_rh_info) {
        return;
    }

    ensure_humidity_graph_overlays();
    if (!rh_graph_label_min_ || !rh_graph_label_now_ || !rh_graph_label_max_) {
        return;
    }

    style_graph_stat_overlays(
        objects.chart_rh_info,
        rh_graph_label_min_,
        rh_graph_label_now_,
        rh_graph_label_max_);

    if (!has_values) {
        safe_label_set_text(rh_graph_label_min_, "MIN --");
        safe_label_set_text(rh_graph_label_now_, "NOW --");
        safe_label_set_text(rh_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f%%", min_humidity);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f%%", latest_humidity);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f%%", max_humidity);
    safe_label_set_text(rh_graph_label_min_, min_buf);
    safe_label_set_text(rh_graph_label_now_, now_buf);
    safe_label_set_text(rh_graph_label_max_, max_buf);
}

void UiController::ensure_humidity_zone_overlay() {
    if (!objects.rh_info_graph || !objects.chart_rh_info) {
        return;
    }

    if (!rh_graph_zone_overlay_ || !lv_obj_is_valid(rh_graph_zone_overlay_) ||
        lv_obj_get_parent(rh_graph_zone_overlay_) != objects.rh_info_graph) {
        rh_graph_zone_overlay_ = lv_obj_create(objects.rh_info_graph);
        lv_obj_clear_flag(rh_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(rh_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_rh_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_rh_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_rh_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_rh_info);

    lv_obj_set_pos(rh_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(rh_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(rh_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_rh_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = rh_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != rh_graph_zone_overlay_) {
            band = lv_obj_create(rh_graph_zone_overlay_);
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

    lv_obj_move_background(rh_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_rh_info);
}

void UiController::update_humidity_zone_overlay(float y_min_display, float y_max_display) {
    ensure_humidity_zone_overlay();
    if (!rh_graph_zone_overlay_ || !lv_obj_is_valid(rh_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(rh_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(rh_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (rh_graph_zone_bands_[i]) {
                lv_obj_add_flag(rh_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kRhZoneBounds[kMaxGraphZoneBounds] = {-1000.0f, 20.0f, 30.0f, 40.0f, 60.0f, 65.0f, 70.0f, 1000.0f};
    static const GraphZoneTone kRhZoneTones[kMaxGraphZoneBands] = {
        GRAPH_ZONE_RED,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };

    const float denom = y_max_display - y_min_display;
    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_rh_info, LV_PART_MAIN);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = rh_graph_zone_bands_[i];
        if (!band) {
            continue;
        }

        const float zone_low = kRhZoneBounds[i];
        const float zone_high = kRhZoneBounds[i + 1];
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
        lv_color_t zone_color = resolve_graph_zone_color(kRhZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_humidity_time_labels() {
    ensure_graph_time_labels(
        objects.rh_info_graph,
        objects.chart_rh_info,
        rh_graph_time_labels_,
        kTempGraphTimeTickCount);
}

void UiController::update_humidity_time_labels() {
    update_graph_time_labels(
        objects.rh_info_graph,
        objects.chart_rh_info,
        rh_graph_time_labels_,
        kTempGraphTimeTickCount,
        humidity_graph_points());
}

void UiController::update_temperature_info_graph() {
    if (!objects.chart_temp_info) {
        return;
    }

    const SensorGraphProfile profile = build_temperature_graph_profile();
    apply_temperature_graph_theme(profile);

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

    const float fallback = profile.fallback_value;

    float scale_min = min_temp;
    float scale_max = max_temp;
    if (has_values) {
        scale_min = min_temp;
        scale_max = max_temp;
    } else {
        scale_min = fallback;
        scale_max = fallback;
        latest_temp = fallback;
    }

    const float min_span = profile.min_span;
    const float span = scale_max - scale_min;
    float scale_span = span;
    if (!isfinite(scale_span) || scale_span < min_span) {
        scale_span = min_span;
    }
    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = temp_units_c ? 0.5f : 1.0f;
    }
    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_temp) ? latest_temp : fallback;
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
    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    const int32_t horizontal_min = (profile.horizontal_divisions_min > 0)
        ? static_cast<int32_t>(profile.horizontal_divisions_min)
        : 3;
    const int32_t horizontal_max = (profile.horizontal_divisions_max >= profile.horizontal_divisions_min &&
                                    profile.horizontal_divisions_max > 0)
        ? static_cast<int32_t>(profile.horizontal_divisions_max)
        : 12;
    if (horizontal_divisions < horizontal_min) {
        horizontal_divisions = horizontal_min;
    }
    if (horizontal_divisions > horizontal_max) {
        horizontal_divisions = horizontal_max;
    }
    const uint8_t vertical_divisions = (profile.vertical_divisions > 0) ? profile.vertical_divisions : 15;
    lv_chart_set_div_line_count(objects.chart_temp_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_temp_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_temperature_zone_overlay(profile, y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_temp)) {
            latest_temp = max_temp;
        }
        update_temperature_graph_overlays(profile, true, min_temp, max_temp, latest_temp);
    } else {
        update_temperature_graph_overlays(profile, false, fallback, fallback, fallback);
    }
    update_temperature_time_labels();

    lv_chart_refresh(objects.chart_temp_info);
}

void UiController::update_humidity_info_graph() {
    if (!objects.chart_rh_info) {
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

    lv_chart_set_type(objects.chart_rh_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_rh_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (rh_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }
    lv_chart_set_div_line_count(objects.chart_rh_info, 5, vertical_divisions);

    lv_obj_set_style_bg_color(objects.chart_rh_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_rh_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_rh_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_rh_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_rh_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_rh_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_rh_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_rh_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_rh_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_rh_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_rh_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_rh_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = humidity_graph_points();
    lv_chart_set_point_count(objects.chart_rh_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_rh_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_rh_info,
                                     lv_obj_get_style_line_color(objects.chart_rh_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_rh_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_rh_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_h = FLT_MAX;
    float max_h = -FLT_MAX;
    float latest_h = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_HUMIDITY, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_h = value;
                    max_h = value;
                    has_values = true;
                } else {
                    if (value < min_h) {
                        min_h = value;
                    }
                    if (value > max_h) {
                        max_h = value;
                    }
                }
                latest_h = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_rh_info, series, i, point_value);
    }

    float scale_min = has_values ? min_h : 50.0f;
    float scale_max = has_values ? max_h : 50.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 10.0f) {
        scale_span = 10.0f;
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
        y_min_f = 0.0f;
        y_max_f = 100.0f;
    }

    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }
    if (y_max_f > 100.0f) {
        y_max_f = 100.0f;
    }
    if (y_max_f <= y_min_f) {
        y_min_f = 0.0f;
        y_max_f = 100.0f;
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

    lv_chart_set_div_line_count(objects.chart_rh_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_rh_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_humidity_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_h)) {
            latest_h = max_h;
        }
        update_humidity_graph_overlays(true, min_h, max_h, latest_h);
    } else {
        update_humidity_graph_overlays(false, 50.0f, 50.0f, 50.0f);
    }
    update_humidity_time_labels();

    lv_chart_refresh(objects.chart_rh_info);
}

