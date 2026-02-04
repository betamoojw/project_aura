#pragma once
#include <Arduino.h>
#include <lvgl.h>

struct SensorData {
    float temperature = 0.0f;
    float humidity = 0.0f;
    float pm1 = 0.0f;
    float pm25 = 0.0f;
    float pm4 = 0.0f;
    float pm10 = 0.0f;
    float pressure = 0.0f;
    float pressure_delta_3h = 0.0f;
    float pressure_delta_24h = 0.0f;
    float hcho = 0.0f;
    int co2 = 0;
    int voc_index = 0;
    int nox_index = 0;
    bool temp_valid = false;
    bool hum_valid = false;
    bool pm_valid = false;
    bool pm25_valid = false;
    bool pm10_valid = false;
    bool co2_valid = false;
    bool voc_valid = false;
    bool nox_valid = false;
    bool hcho_valid = false;
    bool pressure_valid = false;
    bool pressure_delta_3h_valid = false;
    bool pressure_delta_24h_valid = false;
};

struct AirQuality {
    const char *status;
    int score;
    lv_color_t color;
};

struct ThemeColors {
    lv_color_t screen_bg;
    lv_color_t card_bg;
    lv_color_t card_border;
    lv_color_t text_primary;
    lv_color_t shadow_color;
    bool shadow_enabled;
    bool gradient_enabled;
    lv_color_t gradient_color;
    lv_grad_dir_t gradient_direction;
    bool screen_gradient_enabled;
    lv_color_t screen_gradient_color;
    lv_grad_dir_t screen_gradient_direction;
};

struct ThemeSwatch {
    lv_obj_t *btn;
    lv_obj_t *card;
    lv_obj_t *label;
};

struct TimeZoneEntry {
    const char *name;
    int16_t offset_min;
    const char *posix;
};

extern const TimeZoneEntry kTimeZones[];
extern const size_t TIME_ZONE_COUNT;
