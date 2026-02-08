// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"
#include "ui/UiText.h"
#include "ui/ui.h"
#include "core/MathUtils.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

// CO sensor integration is not wired into SensorData yet.
// Keep PM4 fallback active until dedicated CO data/valid flags are added.
bool has_co_sensor_data(const SensorData &) {
    return false;
}

float get_co_ppm_value(const SensorData &) {
    return NAN;
}

bool mold_inputs_valid(float temp_c, float rh) {
    return isfinite(temp_c) && isfinite(rh) && rh >= 0.0f && rh <= 100.0f;
}

int compute_mold_risk_index(float temp_c, float rh) {
    // Practical 0..10 indoor mold risk heuristic driven by RH + temperature.
    // RH is primary driver; warmer air slightly increases risk.
    if (!mold_inputs_valid(temp_c, rh)) {
        return -1;
    }
    float risk = ((rh - 55.0f) / 4.0f) + ((temp_c - 18.0f) / 7.0f);
    risk = constrain(risk, 0.0f, 10.0f);
    return static_cast<int>(lroundf(risk));
}

} // namespace

void UiController::update_sensor_cards(const AirQuality &aq, bool gas_warmup, bool show_co2_bar) {
    char buf[16];

    if (currentData.co2_valid) {
        snprintf(buf, sizeof(buf), "%d", currentData.co2);
        safe_label_set_text(objects.label_co2_value, buf);
        safe_label_set_text(objects.label_co2_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_co2_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_co2_value_1, UiText::ValueMissing());
    }
    if (objects.co2_bar_wrap) {
        show_co2_bar ? lv_obj_clear_flag(objects.co2_bar_wrap, LV_OBJ_FLAG_HIDDEN)
                     : lv_obj_add_flag(objects.co2_bar_wrap, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.co2_bar_wrap_1) {
        show_co2_bar ? lv_obj_clear_flag(objects.co2_bar_wrap_1, LV_OBJ_FLAG_HIDDEN)
                     : lv_obj_add_flag(objects.co2_bar_wrap_1, LV_OBJ_FLAG_HIDDEN);
    }
    lv_color_t co2_col = currentData.co2_valid ? getCO2Color(currentData.co2) : color_inactive();
    set_dot_color(objects.dot_co2, alert_color_for_mode(co2_col));
    set_dot_color(objects.dot_co2_1, alert_color_for_mode(co2_col));
    if (show_co2_bar) {
        set_dot_color(objects.co2_marker, co2_col);
        update_co2_bar(currentData.co2, currentData.co2_valid);

        if (objects.co2_bar_fill_1 && objects.co2_marker_1) {
            if (!currentData.co2_valid) {
                if (objects.co2_bar_mask_1) {
                    lv_obj_set_width(objects.co2_bar_mask_1, 0);
                } else {
                    lv_obj_set_width(objects.co2_bar_fill_1, 0);
                }
                lv_obj_set_x(objects.co2_marker_1, 2);
            } else {
                int bar_max = 330;
                int fill_w = lv_obj_get_width(objects.co2_bar_fill_1);
                if (fill_w > 0) {
                    bar_max = fill_w;
                }
                int clamped = constrain(currentData.co2, 400, 2000);
                int w = map(clamped, 400, 2000, 0, bar_max);
                w = constrain(w, 0, bar_max);
                if (objects.co2_bar_mask_1) {
                    lv_obj_set_width(objects.co2_bar_mask_1, w);
                } else {
                    lv_obj_set_width(objects.co2_bar_fill_1, w);
                }

                const int marker_w = 14;
                int center = 4 + w;
                int x = center - (marker_w / 2);
                int track_w = objects.co2_bar_track_1 ? lv_obj_get_width(objects.co2_bar_track_1) : 0;
                int max_x = (track_w > 0) ? (track_w - marker_w - 2) : (340 - marker_w - 2);
                x = constrain(x, 2, max_x);
                lv_obj_set_x(objects.co2_marker_1, x);
            }
            set_dot_color(objects.co2_marker_1, co2_col);
        }
    }

    if (currentData.temp_valid) {
        float temp_display = currentData.temperature;
        if (!temp_units_c) {
            temp_display = (temp_display * 9.0f / 5.0f) + 32.0f;
        }
        snprintf(buf, sizeof(buf), "%.1f", temp_display);
        safe_label_set_text(objects.label_temp_value, buf);
        safe_label_set_text(objects.label_temp_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_temp_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_temp_value_1, UiText::ValueMissing());
    }
    if (objects.label_temp_unit) {
        safe_label_set_text_static(objects.label_temp_unit, temp_units_c ? UiText::UnitC() : UiText::UnitF());
    }
    if (objects.label_temp_unit_1) {
        safe_label_set_text_static(objects.label_temp_unit_1, temp_units_c ? UiText::UnitC() : UiText::UnitF());
    }
    lv_color_t temp_col = currentData.temp_valid ? getTempColor(currentData.temperature) : color_inactive();
    set_dot_color(objects.dot_temp, alert_color_for_mode(temp_col));
    set_dot_color(objects.dot_temp_1, alert_color_for_mode(temp_col));

    if (currentData.hum_valid) {
        snprintf(buf, sizeof(buf), "%.0f", currentData.humidity);
        safe_label_set_text(objects.label_hum_value, buf);
        safe_label_set_text(objects.label_hum_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_hum_value, UiText::ValueMissingShort());
        safe_label_set_text_static(objects.label_hum_value_1, UiText::ValueMissingShort());
    }
    lv_color_t hum_col = currentData.hum_valid ? getHumidityColor(currentData.humidity) : color_inactive();
    set_dot_color(objects.dot_hum, alert_color_for_mode(hum_col));
    set_dot_color(objects.dot_hum_1, alert_color_for_mode(hum_col));

    float dew_c = NAN;
    float dew_c_rounded = NAN;
    float ah_gm3 = NAN;
    if (currentData.temp_valid && currentData.hum_valid) {
        dew_c = MathUtils::compute_dew_point_c(currentData.temperature, currentData.humidity);
        if (isfinite(dew_c)) {
            dew_c_rounded = roundf(dew_c);
        }
        ah_gm3 = MathUtils::compute_absolute_humidity_gm3(currentData.temperature, currentData.humidity);
    }
    if (isfinite(dew_c)) {
        float dew_display = dew_c;
        if (!temp_units_c) {
            dew_display = (dew_display * 9.0f / 5.0f) + 32.0f;
        } else if (isfinite(dew_c_rounded)) {
            dew_display = dew_c_rounded;
        }
        snprintf(buf, sizeof(buf), "%.0f", dew_display);
        safe_label_set_text(objects.label_dew_value, buf);
        safe_label_set_text(objects.label_dew_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_dew_value, UiText::ValueMissingShort());
        safe_label_set_text_static(objects.label_dew_value_1, UiText::ValueMissingShort());
    }
    if (objects.label_dew_unit) {
        safe_label_set_text_static(objects.label_dew_unit, temp_units_c ? UiText::UnitC() : UiText::UnitF());
    }
    if (objects.label_dew_unit_1) {
        safe_label_set_text_static(objects.label_dew_unit_1, temp_units_c ? UiText::UnitC() : UiText::UnitF());
    }
    if (objects.dot_dp) {
        float dp_color_c = isfinite(dew_c_rounded) ? dew_c_rounded : dew_c;
        lv_color_t dp_col = getDewPointColor(dp_color_c);
        set_dot_color(objects.dot_dp, alert_color_for_mode(dp_col));
        set_dot_color(objects.dot_dp_1, alert_color_for_mode(dp_col));
    }
    if (isfinite(ah_gm3)) {
        snprintf(buf, sizeof(buf), "%.0f", ah_gm3);
        safe_label_set_text(objects.label_ah_value, buf);
        safe_label_set_text(objects.label_ah_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_ah_value, UiText::ValueMissingShort());
        safe_label_set_text_static(objects.label_ah_value_1, UiText::ValueMissingShort());
    }
    if (objects.dot_ah) {
        lv_color_t ah_col = getAbsoluteHumidityColor(ah_gm3);
        set_dot_color(objects.dot_ah, alert_color_for_mode(ah_col));
        set_dot_color(objects.dot_ah_1, alert_color_for_mode(ah_col));
    }

    const int mold_risk =
        (currentData.temp_valid && currentData.hum_valid)
            ? compute_mold_risk_index(currentData.temperature, currentData.humidity)
            : -1;
    if (objects.label_mr_value) {
        if (mold_risk >= 0) {
            snprintf(buf, sizeof(buf), "%d", mold_risk);
            safe_label_set_text(objects.label_mr_value, buf);
        } else {
            safe_label_set_text_static(objects.label_mr_value, UiText::ValueMissingShort());
        }
    }
    if (objects.dot_mr) {
        lv_color_t mr_col = color_inactive();
        if (mold_risk >= 0) {
            if (mold_risk <= 2) mr_col = color_green();
            else if (mold_risk <= 4) mr_col = color_yellow();
            else if (mold_risk <= 7) mr_col = color_orange();
            else mr_col = color_red();
        }
        set_dot_color(objects.dot_mr, alert_color_for_mode(mr_col));
    }

    if (currentData.pm25_valid) {
        if (currentData.pm25 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm25);
        else snprintf(buf, sizeof(buf), "%.0f", currentData.pm25);
        safe_label_set_text(objects.label_pm25_value, buf);
        safe_label_set_text(objects.label_pm25_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_pm25_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_pm25_value_1, UiText::ValueMissing());
    }
    lv_color_t pm25_col = currentData.pm25_valid ? getPM25Color(currentData.pm25) : color_inactive();
    set_dot_color(objects.dot_pm25, alert_color_for_mode(pm25_col));
    set_dot_color(objects.dot_pm25_1, alert_color_for_mode(pm25_col));

    if (currentData.pm10_valid) {
        if (currentData.pm10 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm10);
        else snprintf(buf, sizeof(buf), "%.0f", currentData.pm10);
        safe_label_set_text(objects.label_pm10_value, buf);
        safe_label_set_text(objects.label_pm10_value_pro, buf);
    } else {
        safe_label_set_text_static(objects.label_pm10_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_pm10_value_pro, UiText::ValueMissing());
    }
    lv_color_t pm10_col = currentData.pm10_valid ? getPM10Color(currentData.pm10) : color_inactive();
    set_dot_color(objects.dot_pm10, alert_color_for_mode(pm10_col));
    set_dot_color(objects.dot_pm10_pro, alert_color_for_mode(pm10_col));

    const bool pm1_available = currentData.pm_valid && isfinite(currentData.pm1) && currentData.pm1 >= 0.0f;
    if (objects.label_pm1_value) {
        if (pm1_available) {
            if (currentData.pm1 < 10.0f) {
                snprintf(buf, sizeof(buf), "%.1f", currentData.pm1);
            } else {
                snprintf(buf, sizeof(buf), "%.0f", currentData.pm1);
            }
            safe_label_set_text(objects.label_pm1_value, buf);
        } else {
            safe_label_set_text_static(objects.label_pm1_value, UiText::ValueMissing());
        }
    }
    if (objects.dot_pm1) {
        lv_color_t pm1_col = pm1_available ? getPM1Color(currentData.pm1) : color_inactive();
        set_dot_color(objects.dot_pm1, alert_color_for_mode(pm1_col));
    }

    if (currentData.voc_valid) {
        snprintf(buf, sizeof(buf), "%d", currentData.voc_index);
        safe_label_set_text(objects.label_voc_value, buf);
        safe_label_set_text(objects.label_voc_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_voc_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_voc_value_1, UiText::ValueMissing());
    }
    if (objects.label_voc_warmup) {
        gas_warmup ? lv_obj_clear_flag(objects.label_voc_warmup, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_add_flag(objects.label_voc_warmup, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_voc_warmup_1) {
        gas_warmup ? lv_obj_clear_flag(objects.label_voc_warmup_1, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_add_flag(objects.label_voc_warmup_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_voc_value) {
        gas_warmup ? lv_obj_add_flag(objects.label_voc_value, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_voc_value, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_voc_value_1) {
        gas_warmup ? lv_obj_add_flag(objects.label_voc_value_1, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_voc_value_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_voc_unit) {
        gas_warmup ? lv_obj_add_flag(objects.label_voc_unit, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_voc_unit, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_voc_unit_1) {
        gas_warmup ? lv_obj_add_flag(objects.label_voc_unit_1, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_voc_unit_1, LV_OBJ_FLAG_HIDDEN);
    }
    lv_color_t voc_col = gas_warmup ? color_blue()
                                    : (currentData.voc_valid ? getVOCColor(currentData.voc_index) : color_inactive());
    set_dot_color(objects.dot_voc, gas_warmup ? voc_col : alert_color_for_mode(voc_col));
    set_dot_color(objects.dot_voc_1, gas_warmup ? voc_col : alert_color_for_mode(voc_col));

    if (currentData.nox_valid) {
        snprintf(buf, sizeof(buf), "%d", currentData.nox_index);
        safe_label_set_text(objects.label_nox_value, buf);
        safe_label_set_text(objects.label_nox_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_nox_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_nox_value_1, UiText::ValueMissing());
    }
    if (objects.label_nox_warmup) {
        gas_warmup ? lv_obj_clear_flag(objects.label_nox_warmup, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_add_flag(objects.label_nox_warmup, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_nox_warmup_1) {
        gas_warmup ? lv_obj_clear_flag(objects.label_nox_warmup_1, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_add_flag(objects.label_nox_warmup_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_nox_value) {
        gas_warmup ? lv_obj_add_flag(objects.label_nox_value, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_nox_value, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_nox_value_1) {
        gas_warmup ? lv_obj_add_flag(objects.label_nox_value_1, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_nox_value_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_nox_unit) {
        gas_warmup ? lv_obj_add_flag(objects.label_nox_unit, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_nox_unit, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.label_nox_unit_1) {
        gas_warmup ? lv_obj_add_flag(objects.label_nox_unit_1, LV_OBJ_FLAG_HIDDEN)
                   : lv_obj_clear_flag(objects.label_nox_unit_1, LV_OBJ_FLAG_HIDDEN);
    }
    lv_color_t nox_col = gas_warmup ? color_blue()
                                    : (currentData.nox_valid ? getNOxColor(currentData.nox_index) : color_inactive());
    set_dot_color(objects.dot_nox, gas_warmup ? nox_col : alert_color_for_mode(nox_col));
    set_dot_color(objects.dot_nox_1, gas_warmup ? nox_col : alert_color_for_mode(nox_col));

    const bool hcho_available = currentData.hcho_valid;
    if (objects.label_hcho_title) {
        safe_label_set_text_static(objects.label_hcho_title, hcho_available ? UiText::LabelHcho() : UiText::LabelAqi());
    }
    if (objects.label_hcho_unit) {
        safe_label_set_text_static(objects.label_hcho_unit, hcho_available ? UiText::UnitPpb() : UiText::UnitIndex());
    }
    if (objects.label_hcho_value) {
        if (hcho_available) {
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(lroundf(currentData.hcho)));
        } else {
            snprintf(buf, sizeof(buf), "%d", aq.score);
        }
        safe_label_set_text(objects.label_hcho_value, buf);
    }
    if (objects.dot_hcho) {
        lv_color_t hcho_col = hcho_available ? getHCHOColor(currentData.hcho, true) : aq.color;
        set_dot_color(objects.dot_hcho, alert_color_for_mode(hcho_col));
    }

    // PRO card fallback: HCHO if available, otherwise AQI.
    if (objects.label_hcho_title_1) {
        safe_label_set_text_static(objects.label_hcho_title_1, hcho_available ? UiText::LabelHcho() : UiText::LabelAqi());
    }
    if (objects.label_hcho_unit_1) {
        safe_label_set_text_static(objects.label_hcho_unit_1, hcho_available ? UiText::UnitPpb() : UiText::UnitIndex());
    }
    if (objects.label_hcho_value_1) {
        if (hcho_available) {
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(lroundf(currentData.hcho)));
        } else {
            snprintf(buf, sizeof(buf), "%d", aq.score);
        }
        safe_label_set_text(objects.label_hcho_value_1, buf);
    }
    if (objects.dot_hcho_1) {
        lv_color_t hcho_col = hcho_available ? getHCHOColor(currentData.hcho, true) : aq.color;
        set_dot_color(objects.dot_hcho_1, alert_color_for_mode(hcho_col));
    }

    // PRO card fallback: PM4 until dedicated CO sensor data is available.
    const bool co_available = has_co_sensor_data(currentData);
    const bool pm4_available = currentData.pm_valid && isfinite(currentData.pm4) && currentData.pm4 >= 0.0f;
    if (objects.label_co_title) {
        safe_label_set_text_static(objects.label_co_title, co_available ? "CO" : "PM4");
    }
    if (objects.label_co_unit) {
        safe_label_set_text_static(objects.label_co_unit, co_available ? "ppm" : "ug/m3");
    }
    if (objects.label_co_value) {
        if (co_available) {
            float co_ppm = get_co_ppm_value(currentData);
            if (isfinite(co_ppm) && co_ppm >= 0.0f) {
                snprintf(buf, sizeof(buf), "%.0f", co_ppm);
            } else {
                strcpy(buf, UiText::ValueMissing());
            }
        } else if (pm4_available) {
            if (currentData.pm4 < 10.0f) {
                snprintf(buf, sizeof(buf), "%.1f", currentData.pm4);
            } else {
                snprintf(buf, sizeof(buf), "%.0f", currentData.pm4);
            }
        } else {
            strcpy(buf, UiText::ValueMissing());
        }
        safe_label_set_text(objects.label_co_value, buf);
    }
    if (objects.dot_co) {
        lv_color_t co_card_col = color_inactive();
        if (co_available) {
            const float co_ppm = get_co_ppm_value(currentData);
            if (isfinite(co_ppm) && co_ppm >= 0.0f) {
                // Placeholder mapping until dedicated CO thresholds are added.
                co_card_col = getPM10Color(co_ppm);
            }
        } else if (pm4_available) {
            co_card_col = getPM4Color(currentData.pm4);
        }
        set_dot_color(objects.dot_co, alert_color_for_mode(co_card_col));
    }

    // PRO divider lines follow active theme border color, no shadow.
    const lv_color_t divider_col = color_card_border();
    if (objects.line_1) {
        lv_obj_set_style_line_color(objects.line_1, divider_col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(objects.line_1, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (objects.line_2) {
        lv_obj_set_style_line_color(objects.line_2, divider_col, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(objects.line_2, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (currentData.pressure_valid) {
        snprintf(buf, sizeof(buf), "%.0f", currentData.pressure);
        safe_label_set_text(objects.label_pressure_value, buf);
        safe_label_set_text(objects.label_pressure_value_1, buf);
    } else {
        safe_label_set_text_static(objects.label_pressure_value, UiText::ValueMissing());
        safe_label_set_text_static(objects.label_pressure_value_1, UiText::ValueMissing());
    }

    if (currentData.pressure_delta_3h_valid) {
        if (currentData.pressure_delta_3h > 0.05f) {
            snprintf(buf, sizeof(buf), "+%.1f", currentData.pressure_delta_3h);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", currentData.pressure_delta_3h);
        }
        safe_label_set_text(objects.label_delta_3h_value, buf);
        safe_label_set_text(objects.label_delta_5, buf);
    } else {
        safe_label_set_text_static(objects.label_delta_3h_value, UiText::ValueMissingShort());
        safe_label_set_text_static(objects.label_delta_5, UiText::ValueMissingShort());
    }

    if (currentData.pressure_delta_24h_valid) {
        if (currentData.pressure_delta_24h > 0.05f) {
            snprintf(buf, sizeof(buf), "+%.1f", currentData.pressure_delta_24h);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", currentData.pressure_delta_24h);
        }
        safe_label_set_text(objects.label_delta_24h_value, buf);
        safe_label_set_text(objects.label_delta_26, buf);
    } else {
        safe_label_set_text_static(objects.label_delta_24h_value, UiText::ValueMissingShort());
        safe_label_set_text_static(objects.label_delta_26, UiText::ValueMissingShort());
    }

    lv_color_t delta_3h_color = night_mode
        ? color_card_border()
        : getPressureDeltaColor(currentData.pressure_delta_3h, currentData.pressure_delta_3h_valid, false);
    lv_color_t delta_24h_color = night_mode
        ? color_card_border()
        : getPressureDeltaColor(currentData.pressure_delta_24h, currentData.pressure_delta_24h_valid, true);
    set_chip_color(objects.chip_delta_3h, delta_3h_color);
    set_chip_color(objects.chip_delta_24h, delta_24h_color);
    set_chip_color(objects.chip_delta_4, delta_3h_color);
    set_chip_color(objects.chip_delta_25, delta_24h_color);
}
