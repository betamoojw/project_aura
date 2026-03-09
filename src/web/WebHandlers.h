// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <Arduino.h>

class WebServer;
class PubSubClient;
class StorageManager;
class ThemeManager;
class FanControl;
class SensorManager;
class ChartsHistory;
class UiController;
struct SensorData;

struct WebHandlerContext {
    WebServer *server = nullptr;
    StorageManager *storage = nullptr;
    ThemeManager *theme_manager = nullptr;
    const String *hostname = nullptr;

    String *wifi_ssid = nullptr;
    String *wifi_pass = nullptr;
    bool *wifi_enabled = nullptr;
    bool *wifi_enabled_dirty = nullptr;
    bool *wifi_ui_dirty = nullptr;
    bool *wifi_scan_in_progress = nullptr;
    String *wifi_scan_options = nullptr;
    bool (*wifi_is_connected)() = nullptr;
    bool (*wifi_is_ap_mode)() = nullptr;
    void (*wifi_start_scan)() = nullptr;
    void (*wifi_stop_scan)() = nullptr;
    void (*wifi_start_sta)() = nullptr;

    PubSubClient *mqtt_client = nullptr;
    bool *mqtt_user_enabled = nullptr;
    uint8_t *mqtt_connect_fail_count = nullptr;
    String *mqtt_host = nullptr;
    uint16_t *mqtt_port = nullptr;
    String *mqtt_user = nullptr;
    String *mqtt_pass = nullptr;
    String *mqtt_device_name = nullptr;
    String *mqtt_base_topic = nullptr;
    String *mqtt_device_id = nullptr;
    bool *mqtt_discovery = nullptr;
    bool *mqtt_anonymous = nullptr;
    void (*mqtt_sync_with_wifi)() = nullptr;
    bool *mqtt_ui_open = nullptr;
    bool *theme_ui_open = nullptr;

    FanControl *fan_control = nullptr;
    SensorManager *sensor_manager = nullptr;
    ChartsHistory *charts_history = nullptr;
    SensorData *sensor_data = nullptr;
    UiController *ui_controller = nullptr;
};

void WebHandlersInit(WebHandlerContext *context);
void WebHandlersPollDeferred();
bool WebHandlersIsOtaBusy();
bool WebHandlersConsumeRestartRequest();

bool wifi_is_ascii_printable(const String &value, size_t max_len);
String wifi_label_safe(const String &value);
void wifi_build_scan_items(int count);

void wifi_handle_root();
void dashboard_handle_root();
void dashboard_handle_styles();
void dashboard_handle_app();
void wifi_handle_save();
void wifi_handle_not_found();
void diag_handle_root();
void mqtt_handle_root();
void mqtt_handle_save();
void theme_handle_root();
void theme_handle_state();
void theme_handle_apply();
void dac_handle_root();
void dac_handle_state();
void dac_handle_action();
void dac_handle_auto();
void charts_handle_data();
void state_handle_data();
void events_handle_data();
void diag_handle_data();
void settings_handle_update();
void ota_handle_update();
void ota_handle_upload();
