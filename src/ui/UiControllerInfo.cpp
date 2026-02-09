// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"

#include <math.h>
#include <string.h>

#include "core/MathUtils.h"
#include "ui/UiText.h"
#include "ui/ui.h"

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
            } else if (objects.label_hcho_unit) {
                unit = lv_label_get_text(objects.label_hcho_unit);
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
            } else if (objects.label_co2_unit) {
                unit = lv_label_get_text(objects.label_co2_unit);
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
            const char *unit = objects.label_hum_unit
                ? lv_label_get_text(objects.label_hum_unit)
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
            const char *unit = objects.label_ah_unit
                ? lv_label_get_text(objects.label_ah_unit)
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
            } else if (objects.label_pm25_unit) {
                unit = lv_label_get_text(objects.label_pm25_unit);
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
            if (objects.label_pm10_unit_pro) {
                unit = lv_label_get_text(objects.label_pm10_unit_pro);
            } else if (objects.label_pm10_unit) {
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
            const bool pm1_available = currentData.pm_valid && isfinite(currentData.pm1) && currentData.pm1 >= 0.0f;
            if (pm1_available) {
                char buf[16];
                if (currentData.pm1 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm1);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm1);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = objects.label_pm1_unit
                ? lv_label_get_text(objects.label_pm1_unit)
                : "ug/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm1_col = pm1_available ? getPM1Color(currentData.pm1) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(pm1_col));
            break;
        }
        case INFO_PM4: {
            const bool pm4_available = currentData.pm_valid && isfinite(currentData.pm4) && currentData.pm4 >= 0.0f;
            if (pm4_available) {
                char buf[16];
                if (currentData.pm4 < 10.0f) snprintf(buf, sizeof(buf), "%.1f", currentData.pm4);
                else snprintf(buf, sizeof(buf), "%.0f", currentData.pm4);
                safe_label_set_text(objects.label_sensor_value, buf);
            } else {
                safe_label_set_text(objects.label_sensor_value, UiText::ValueMissing());
            }
            const char *unit = objects.label_co_unit
                ? lv_label_get_text(objects.label_co_unit)
                : "ug/m3";
            safe_label_set_text(objects.label_sensor_info_unit, unit);
            lv_color_t pm4_col = pm4_available ? getPM4Color(currentData.pm4) : color_inactive();
            set_dot_color(objects.dot_sensor_info, alert_color_for_mode(pm4_col));
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
            safe_label_set_text(objects.label_pressure_value, buf);

            const char *unit = nullptr;
            if (objects.label_pressure_unit_1) {
                unit = lv_label_get_text(objects.label_pressure_unit_1);
            } else if (objects.label_pressure_unit) {
                unit = lv_label_get_text(objects.label_pressure_unit);
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
            safe_label_set_text(objects.label_delta_3h_value, buf);
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
            safe_label_set_text(objects.label_delta_24h_value, buf);
            safe_label_set_text(objects.label_delta_24h_value_1, buf);

            lv_color_t delta_3h_color = night_mode
                ? color_card_border()
                : getPressureDeltaColor(currentData.pressure_delta_3h, currentData.pressure_delta_3h_valid, false);
            lv_color_t delta_24h_color = night_mode
                ? color_card_border()
                : getPressureDeltaColor(currentData.pressure_delta_24h, currentData.pressure_delta_24h_valid, true);
            set_chip_color(objects.chip_delta_3h, delta_3h_color);
            set_chip_color(objects.chip_delta_24h, delta_24h_color);
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
            } else if (objects.label_temp_value) {
                value = lv_label_get_text(objects.label_temp_value);
            } else {
                value = UiText::ValueMissing();
            }
            safe_label_set_text(objects.label_sensor_value, value);
            const char *unit = nullptr;
            if (objects.label_temp_unit_1) {
                unit = lv_label_get_text(objects.label_temp_unit_1);
            } else if (objects.label_temp_unit) {
                unit = lv_label_get_text(objects.label_temp_unit);
            } else {
                unit = temp_units_c ? UiText::UnitC() : UiText::UnitF();
            }
            safe_label_set_text(objects.label_sensor_info_unit, unit);
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
            } else if (objects.label_voc_unit) {
                unit = lv_label_get_text(objects.label_voc_unit);
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
            } else if (objects.label_nox_unit) {
                unit = lv_label_get_text(objects.label_nox_unit);
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
            } else if (objects.label_hcho_unit) {
                unit = lv_label_get_text(objects.label_hcho_unit);
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
            } else if (objects.label_co2_unit) {
                unit = lv_label_get_text(objects.label_co2_unit);
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
        case INFO_PM25:
        case INFO_PM10:
        case INFO_PM1:
        case INFO_PM4:
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
    set_visible(objects.pm_info, true);
    set_visible(objects.pm25_info, sensor == INFO_PM25);
    set_visible(objects.pm10_info, sensor == INFO_PM10);
    set_visible(objects.pm1_info, sensor == INFO_PM1);
    set_visible(objects.pm4_info, sensor == INFO_PM4);

    if (objects.label_sensor_info_title) {
        if (sensor == INFO_PM25) {
            safe_label_set_text(objects.label_sensor_info_title, "PM2.5");
        } else if (sensor == INFO_PM10) {
            safe_label_set_text(objects.label_sensor_info_title, "PM10");
        } else if (sensor == INFO_PM1) {
            safe_label_set_text(objects.label_sensor_info_title, "PM1");
        } else if (sensor == INFO_PM4) {
            safe_label_set_text(objects.label_sensor_info_title, "PM4");
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
    } else if (objects.label_pressure_title) {
        title = lv_label_get_text(objects.label_pressure_title);
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
    set_visible(objects.pm10_info, false);
    set_visible(objects.pm25_info, false);
    set_visible(objects.pm1_info, false);
    set_visible(objects.pm4_info, false);
}

