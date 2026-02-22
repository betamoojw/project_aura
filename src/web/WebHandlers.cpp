// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebHandlers.h"

#include <math.h>
#include <time.h>
#include <WiFi.h>
#include <Update.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "config/AppConfig.h"
#include "config/AppData.h"
#include "core/MathUtils.h"
#include "core/Logger.h"
#include "modules/ChartsHistory.h"
#include "modules/DacAutoConfig.h"
#include "modules/FanControl.h"
#include "modules/SensorManager.h"
#include "modules/StorageManager.h"
#include "web/WebTemplates.h"
#include "ui/UiController.h"
#include "ui/ThemeManager.h"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

WebHandlerContext *g_ctx = nullptr;
constexpr uint32_t kDeferredActionDelayMs = 200;
bool g_deferred_wifi_start_sta = false;
bool g_deferred_mqtt_sync = false;
uint32_t g_deferred_wifi_start_sta_due_ms = 0;
uint32_t g_deferred_mqtt_sync_due_ms = 0;
constexpr uint32_t kChartStepS = Config::CHART_HISTORY_STEP_MS / 1000UL;
constexpr size_t kEventsApiMaxEntries = 48;
Logger::RecentEntry g_events_snapshot[kEventsApiMaxEntries];
bool g_ota_upload_seen = false;
bool g_ota_upload_active = false;
bool g_ota_upload_success = false;
bool g_ota_size_known = false;
size_t g_ota_expected_size = 0;
size_t g_ota_slot_size = 0;
size_t g_ota_written_size = 0;
String g_ota_error;

struct ChartMetricSpec {
    const char *key;
    const char *unit;
    ChartsHistory::Metric metric;
};

constexpr ChartMetricSpec kChartCoreMetrics[] = {
    {"co2", "ppm", ChartsHistory::METRIC_CO2},
    {"temperature", "C", ChartsHistory::METRIC_TEMPERATURE},
    {"humidity", "%", ChartsHistory::METRIC_HUMIDITY},
    {"pressure", "hPa", ChartsHistory::METRIC_PRESSURE},
};

constexpr ChartMetricSpec kChartGasMetrics[] = {
    {"co", "ppm", ChartsHistory::METRIC_CO},
    {"voc", "idx", ChartsHistory::METRIC_VOC},
    {"nox", "idx", ChartsHistory::METRIC_NOX},
    {"hcho", "ppb", ChartsHistory::METRIC_HCHO},
};

constexpr ChartMetricSpec kChartPmMetrics[] = {
    {"pm05", "#/cm3", ChartsHistory::METRIC_PM05},
    {"pm1", "ug/m3", ChartsHistory::METRIC_PM1},
    {"pm25", "ug/m3", ChartsHistory::METRIC_PM25},
    {"pm4", "ug/m3", ChartsHistory::METRIC_PM4},
    {"pm10", "ug/m3", ChartsHistory::METRIC_PM10},
};

WebHandlerContext *ctx() {
    return g_ctx;
}

uint16_t chart_window_points(const String &window_arg, const char *&window_name) {
    String window = window_arg;
    window.trim();
    window.toLowerCase();

    if (window == "1h") {
        window_name = "1h";
        return Config::CHART_HISTORY_1H_STEPS;
    }
    if (window == "24h") {
        window_name = "24h";
        return Config::CHART_HISTORY_24H_SAMPLES;
    }
    window_name = "3h";
    return Config::CHART_HISTORY_3H_STEPS;
}

void chart_group_metrics(const String &group_arg,
                         const char *&group_name,
                         const ChartMetricSpec *&metrics,
                         size_t &metric_count) {
    String group = group_arg;
    group.trim();
    group.toLowerCase();

    if (group == "gases" || group == "gas") {
        group_name = "gases";
        metrics = kChartGasMetrics;
        metric_count = sizeof(kChartGasMetrics) / sizeof(kChartGasMetrics[0]);
        return;
    }
    if (group == "pm") {
        group_name = "pm";
        metrics = kChartPmMetrics;
        metric_count = sizeof(kChartPmMetrics) / sizeof(kChartPmMetrics[0]);
        return;
    }
    group_name = "core";
    metrics = kChartCoreMetrics;
    metric_count = sizeof(kChartCoreMetrics) / sizeof(kChartCoreMetrics[0]);
}

bool chart_latest_metric(const ChartsHistory &history,
                         ChartsHistory::Metric metric,
                         float &out_value) {
    uint16_t total_count = history.count();
    if (total_count == 0) {
        return false;
    }
    for (int offset = static_cast<int>(total_count) - 1; offset >= 0; --offset) {
        float value = 0.0f;
        bool valid = false;
        if (history.metricValueFromOldest(static_cast<uint16_t>(offset), metric, value, valid) &&
            valid && isfinite(value)) {
            out_value = value;
            return true;
        }
    }
    return false;
}

bool deadline_reached(uint32_t now_ms, uint32_t due_ms) {
    return static_cast<int32_t>(now_ms - due_ms) >= 0;
}

void ota_reset_state() {
    g_ota_upload_seen = false;
    g_ota_upload_active = false;
    g_ota_upload_success = false;
    g_ota_size_known = false;
    g_ota_expected_size = 0;
    g_ota_slot_size = 0;
    g_ota_written_size = 0;
    g_ota_error = "";
}

void ota_set_ui_screen(bool active) {
    WebHandlerContext *context = ctx();
    if (!context || !context->ui_controller) {
        return;
    }
    context->ui_controller->webSetFirmwareUpdateScreen(active);
}

bool parse_size_arg(const String &value, size_t &out) {
    String trimmed = value;
    trimmed.trim();
    if (trimmed.isEmpty()) {
        return false;
    }
    char *end = nullptr;
    const unsigned long long parsed = strtoull(trimmed.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }
    if (parsed == 0 || parsed > static_cast<unsigned long long>(SIZE_MAX)) {
        return false;
    }
    out = static_cast<size_t>(parsed);
    return true;
}

String ota_error_prefixed(const char *prefix) {
    String error = prefix;
    error += ": ";
    error += Update.errorString();
    return error;
}

void ota_set_error(const String &error) {
    if (g_ota_error.isEmpty()) {
        g_ota_error = error;
    }
    g_ota_upload_success = false;
    g_ota_upload_active = false;
    ota_set_ui_screen(false);
}

String html_escape(const String &input) {
    String out;
    out.reserve(input.length());
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

int wifi_rssi_to_quality(int rssi) {
    if (rssi <= -100) {
        return 0;
    }
    if (rssi >= -50) {
        return 100;
    }
    return 2 * (rssi + 100);
}

String theme_color_to_hex(lv_color_t color) {
    lv_color32_t c32;
    c32.full = lv_color_to32(color);
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", c32.ch.red, c32.ch.green, c32.ch.blue);
    return String(buf);
}

bool parse_hex_color(const String &value, lv_color_t &out) {
    String s = value;
    s.trim();
    if (s.startsWith("#")) {
        s = s.substring(1);
    }
    if (s.length() != 6) {
        return false;
    }
    char *end = nullptr;
    long rgb = strtol(s.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    out = lv_color_hex((r << 16) | (g << 8) | b);
    return true;
}

bool has_control_chars(const String &value) {
    for (size_t i = 0; i < value.length(); i++) {
        uint8_t c = static_cast<uint8_t>(value[i]);
        if (c < 32 || c == 127) {
            return true;
        }
    }
    return false;
}

bool mqtt_topic_has_wildcards(const String &topic) {
    return topic.indexOf('#') >= 0 || topic.indexOf('+') >= 0;
}

const char *dac_status_text(const FanControl &fan) {
    if (fan.isFaulted()) {
        return "FAULT";
    }
    if (!fan.isAvailable()) {
        return "OFFLINE";
    }
    return fan.isRunning() ? "RUNNING" : "STOPPED";
}

const char *event_level_text(Logger::Level level) {
    switch (level) {
        case Logger::Error: return "E";
        case Logger::Warn: return "W";
        case Logger::Info: return "I";
        case Logger::Debug: return "D";
        default: return "?";
    }
}

const char *event_severity_text(Logger::Level level) {
    switch (level) {
        case Logger::Error: return "critical";
        case Logger::Warn: return "warning";
        case Logger::Info: return "info";
        case Logger::Debug: return "info";
        default: return "info";
    }
}

void json_set_float_or_null(ArduinoJson::JsonObject obj, const char *key, bool valid, float value) {
    if (valid && isfinite(value)) {
        obj[key] = value;
    } else {
        obj[key] = nullptr;
    }
}

void json_set_int_or_null(ArduinoJson::JsonObject obj, const char *key, bool valid, int value) {
    if (valid) {
        obj[key] = value;
    } else {
        obj[key] = nullptr;
    }
}

String format_uptime_human(uint32_t uptime_seconds) {
    uint32_t days = uptime_seconds / 86400UL;
    uptime_seconds %= 86400UL;
    uint32_t hours = uptime_seconds / 3600UL;
    uptime_seconds %= 3600UL;
    uint32_t minutes = uptime_seconds / 60UL;

    char buf[32];
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %luh %lum",
                 static_cast<unsigned long>(days),
                 static_cast<unsigned long>(hours),
                 static_cast<unsigned long>(minutes));
    } else {
        snprintf(buf, sizeof(buf), "%luh %lum",
                 static_cast<unsigned long>(hours),
                 static_cast<unsigned long>(minutes));
    }
    return String(buf);
}

void fill_web_settings_json(ArduinoJson::JsonObject settings, const WebHandlerContext &context) {
    if (context.ui_controller) {
        settings["night_mode"] = context.ui_controller->webNightModeEnabled();
        settings["night_mode_locked"] = context.ui_controller->webNightModeLocked();
        settings["backlight_on"] = context.ui_controller->webBacklightOn();
        settings["units_c"] = context.ui_controller->webUnitsC();
        settings["temp_offset"] = context.ui_controller->webTempOffset();
        settings["hum_offset"] = context.ui_controller->webHumOffset();
        return;
    }

    if (context.storage) {
        const auto &cfg = context.storage->config();
        settings["night_mode"] = cfg.night_mode;
        settings["night_mode_locked"] = cfg.auto_night_enabled;
        settings["backlight_on"] = nullptr;
        settings["units_c"] = cfg.units_c;
        settings["temp_offset"] = cfg.temp_offset;
        settings["hum_offset"] = cfg.hum_offset;
        return;
    }

    settings["night_mode"] = nullptr;
    settings["night_mode_locked"] = nullptr;
    settings["backlight_on"] = nullptr;
    settings["units_c"] = nullptr;
    settings["temp_offset"] = nullptr;
    settings["hum_offset"] = nullptr;
}

} // namespace

void WebHandlersInit(WebHandlerContext *context) {
    g_ctx = context;
    g_deferred_wifi_start_sta = false;
    g_deferred_mqtt_sync = false;
    g_deferred_wifi_start_sta_due_ms = 0;
    g_deferred_mqtt_sync_due_ms = 0;
    ota_reset_state();
}

void WebHandlersPollDeferred() {
    WebHandlerContext *context = ctx();
    if (!context) {
        return;
    }
    const uint32_t now_ms = millis();

    if (g_deferred_wifi_start_sta && deadline_reached(now_ms, g_deferred_wifi_start_sta_due_ms)) {
        g_deferred_wifi_start_sta = false;
        g_deferred_wifi_start_sta_due_ms = 0;
        if (context->wifi_start_sta) {
            context->wifi_start_sta();
        }
    }

    if (g_deferred_mqtt_sync && deadline_reached(now_ms, g_deferred_mqtt_sync_due_ms)) {
        g_deferred_mqtt_sync = false;
        g_deferred_mqtt_sync_due_ms = 0;
        if (context->mqtt_sync_with_wifi) {
            context->mqtt_sync_with_wifi();
        }
    }
}

bool wifi_is_ascii_printable(const String &value, size_t max_len) {
    size_t len = value.length();
    if (len == 0 || len > max_len) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        uint8_t c = static_cast<uint8_t>(value[i]);
        if (c < 32 || c > 126) {
            return false;
        }
    }
    return true;
}

String wifi_label_safe(const String &value) {
    if (value.isEmpty()) {
        return "---";
    }
    String out;
    out.reserve(value.length());
    for (size_t i = 0; i < value.length(); i++) {
        uint8_t c = static_cast<uint8_t>(value[i]);
        if (c >= 32 && c <= 126) {
            out += static_cast<char>(c);
        } else {
            out += '?';
        }
    }
    return out;
}

void wifi_build_scan_items(int count) {
    WebHandlerContext *context = ctx();
    if (!context || !context->wifi_scan_options) {
        return;
    }
    context->wifi_scan_options->clear();
    if (count <= 0) {
        return;
    }
    context->wifi_scan_options->reserve(count * 220);
    for (int i = 0; i < count; i++) {
        String ssid_raw = WiFi.SSID(i);
        if (ssid_raw.isEmpty()) {
            continue;
        }
        String ssid_label = wifi_label_safe(ssid_raw);
        String ssid_html = html_escape(ssid_label);
        int quality = wifi_rssi_to_quality(WiFi.RSSI(i));
        bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        const char *security = open ? "Open" : "Secure";

        (*context->wifi_scan_options) += "<div class=\"network-item\" data-ssid=\"";
        (*context->wifi_scan_options) += ssid_html;
        (*context->wifi_scan_options) += "\"><div class=\"network-icon\">";
        (*context->wifi_scan_options) += FPSTR(WebTemplates::kWifiIconSvg);
        (*context->wifi_scan_options) += "</div><div class=\"network-info\"><span class=\"network-name\">";
        (*context->wifi_scan_options) += ssid_html;
        (*context->wifi_scan_options) += "</span><span class=\"network-meta\">";
        (*context->wifi_scan_options) += security;
        (*context->wifi_scan_options) += "</span></div><div class=\"network-signal\">";
        (*context->wifi_scan_options) += String(quality);
        (*context->wifi_scan_options) += "%</div></div>";
    }
}

void wifi_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (!context->wifi_is_ap_mode || !context->wifi_is_ap_mode()) {
        context->server->send(404, "text/plain", "Not found");
        return;
    }
    WebServer &server = *context->server;
    if (server.hasArg("scan") && context->wifi_start_scan) {
        context->wifi_start_scan();
    }
    String list_items;
    if (context->wifi_scan_in_progress && *context->wifi_scan_in_progress) {
        list_items = FPSTR(WebTemplates::kWifiListScanning);
    } else if (context->wifi_scan_options && !context->wifi_scan_options->isEmpty()) {
        list_items = *context->wifi_scan_options;
    } else {
        list_items = FPSTR(WebTemplates::kWifiListEmpty);
    }
    String html = FPSTR(WebTemplates::kWifiPageTemplate);
    html.reserve(html.length() + list_items.length() + 64);
    html.replace("{{SSID_ITEMS}}", list_items);
    html.replace("{{SCAN_IN_PROGRESS}}",
                 (context->wifi_scan_in_progress && *context->wifi_scan_in_progress) ? "1" : "0");
    server.send(200, "text/html", html);
}

void dashboard_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }

    const bool ap_mode = context->wifi_is_ap_mode && context->wifi_is_ap_mode();
    const String uri = context->server->uri();

    // Keep captive-portal setup flow intact on AP root.
    if (ap_mode && uri == "/") {
        wifi_handle_root();
        return;
    }

    if (!ap_mode && context->wifi_is_connected && !context->wifi_is_connected()) {
        context->server->send(404, "text/plain", "Not found");
        return;
    }

    context->server->send_P(200, "text/html", WebTemplates::kDashboardPageTemplate);
}

void wifi_handle_save() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->storage) {
        return;
    }
    WebServer &server = *context->server;
    if (!context->wifi_is_ap_mode || !context->wifi_is_ap_mode()) {
        server.send(409, "text/plain", "WiFi save allowed only in AP setup mode");
        return;
    }
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    ssid.trim();
    pass.trim();
    if (ssid.isEmpty()) {
        server.send(400, "text/plain", "SSID required");
        return;
    }
    if (!wifi_is_ascii_printable(ssid, 32)) {
        server.send(400, "text/plain", "SSID must be ASCII (32 chars max)");
        return;
    }

    context->storage->saveWiFiSettings(ssid, pass, true);
    if (context->wifi_ssid) *context->wifi_ssid = ssid;
    if (context->wifi_pass) *context->wifi_pass = pass;
    if (context->wifi_enabled) *context->wifi_enabled = true;
    if (context->wifi_enabled_dirty) *context->wifi_enabled_dirty = false;
    if (context->wifi_ui_dirty) *context->wifi_ui_dirty = true;

    String html = FPSTR(WebTemplates::kWifiSavePage);
    server.send(200, "text/html", html);
    g_deferred_wifi_start_sta = true;
    g_deferred_wifi_start_sta_due_ms = millis() + kDeferredActionDelayMs;
}

void wifi_handle_not_found() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    context->server->send(404, "text/plain", "Not found");
}

void mqtt_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->mqtt_client) {
        return;
    }
    if (!context->wifi_is_connected || !context->wifi_is_connected()) {
        context->server->send(404, "text/plain", "Not found");
        return;
    }
    if (!context->mqtt_ui_open || !*context->mqtt_ui_open) {
        String html = FPSTR(WebTemplates::kMqttLockedPage);
        context->server->send(200, "text/html", html);
        return;
    }
    WebServer &server = *context->server;
    PubSubClient &client = *context->mqtt_client;

    bool wifi_connected = context->wifi_is_connected ? context->wifi_is_connected() : false;
    bool wifi_enabled = context->wifi_enabled ? *context->wifi_enabled : false;
    bool mqtt_enabled = context->mqtt_user_enabled ? *context->mqtt_user_enabled : false;
    uint8_t mqtt_fail_count = context->mqtt_connect_fail_count ? *context->mqtt_connect_fail_count : 0;

    String status_text;
    String status_class;
    if (!mqtt_enabled) {
        status_text = "Disabled";
        status_class = "status-disconnected";
    } else if (!wifi_enabled || !wifi_connected) {
        status_text = "No WiFi";
        status_class = "status-error";
    } else if (client.connected()) {
        status_text = "Connected";
        status_class = "status-connected";
    } else if (mqtt_fail_count < Config::MQTT_CONNECT_MAX_FAILS) {
        status_text = "Connecting";
        status_class = "status-error";
    } else {
        status_text = "Error";
        status_class = "status-error";
    }

    String device_ip = "---";
    if (wifi_connected) {
        device_ip = WiFi.localIP().toString();
    }

    String mqtt_user = context->mqtt_user ? *context->mqtt_user : String();
    String mqtt_pass = context->mqtt_pass ? *context->mqtt_pass : String();
    bool is_anonymous = false;
    if (context->mqtt_anonymous) {
        is_anonymous = *context->mqtt_anonymous;
    } else {
        is_anonymous = mqtt_user.isEmpty() && mqtt_pass.isEmpty();
    }
    String anonymous_checked = is_anonymous ? "checked" : "";
    bool discovery = context->mqtt_discovery ? *context->mqtt_discovery : false;
    String discovery_checked = discovery ? "checked" : "";

    String html = FPSTR(WebTemplates::kMqttPageTemplate);
    html.reserve(html.length() + 512);

    html.replace("{{STATUS}}", status_text);
    html.replace("{{STATUS_CLASS}}", status_class);
    html.replace("{{DEVICE_ID}}", html_escape(context->mqtt_device_id ? *context->mqtt_device_id : String()));
    html.replace("{{DEVICE_IP}}", html_escape(device_ip));
    html.replace("{{MQTT_HOST}}", html_escape(context->mqtt_host ? *context->mqtt_host : String()));
    html.replace("{{MQTT_PORT}}",
                 String(context->mqtt_port ? *context->mqtt_port : Config::MQTT_DEFAULT_PORT));
    html.replace("{{MQTT_USER}}", html_escape(mqtt_user));
    html.replace("{{MQTT_PASS}}", html_escape(mqtt_pass));
    html.replace("{{MQTT_NAME}}", html_escape(context->mqtt_device_name ? *context->mqtt_device_name : String()));
    html.replace("{{MQTT_TOPIC}}", html_escape(context->mqtt_base_topic ? *context->mqtt_base_topic : String()));
    html.replace("{{ANONYMOUS_CHECKED}}", anonymous_checked);
    html.replace("{{DISCOVERY_CHECKED}}", discovery_checked);

    server.send(200, "text/html", html);
}

void mqtt_handle_save() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->storage || !context->mqtt_client) {
        return;
    }
    WebServer &server = *context->server;
    if (!context->wifi_is_connected || !context->wifi_is_connected()) {
        server.send(404, "text/plain", "Not found");
        return;
    }
    if (!context->mqtt_ui_open || !*context->mqtt_ui_open) {
        server.send(409, "text/plain", "Open MQTT screen to enable");
        return;
    }

    String host = server.arg("host");
    String port_str = server.arg("port");
    String user = server.arg("user");
    String pass = server.arg("pass");
    String name = server.arg("name");
    String topic = server.arg("topic");
    bool anonymous = server.hasArg("anonymous");
    bool discovery = server.hasArg("discovery");

    host.trim();
    port_str.trim();
    user.trim();
    pass.trim();
    name.trim();
    topic.trim();

    if (host.isEmpty()) {
        server.send(400, "text/plain", "Broker address required");
        return;
    }

    if (name.isEmpty()) {
        server.send(400, "text/plain", "Device name required");
        return;
    }

    if (topic.isEmpty()) {
        server.send(400, "text/plain", "Base topic required");
        return;
    }

    if (has_control_chars(host) || has_control_chars(user) ||
        has_control_chars(pass) || has_control_chars(name) ||
        has_control_chars(topic)) {
        server.send(400, "text/plain", "Fields contain unsupported control characters");
        return;
    }

    if (mqtt_topic_has_wildcards(topic)) {
        server.send(400, "text/plain", "Base topic must not include MQTT wildcards (+ or #)");
        return;
    }

    uint16_t port = port_str.toInt();
    if (port == 0 || port > 65535) {
        port = Config::MQTT_DEFAULT_PORT;
    }

    if (!anonymous && (user.isEmpty() || pass.isEmpty())) {
        server.send(400, "text/plain",
                    "Username and password are required when anonymous mode is disabled");
        return;
    }

    if (anonymous) {
        if (user.isEmpty() && context->mqtt_user) {
            user = *context->mqtt_user;
        }
        if (pass.isEmpty() && context->mqtt_pass) {
            pass = *context->mqtt_pass;
        }
    }

    if (topic.endsWith("/")) {
        topic.remove(topic.length() - 1);
    }

    context->storage->saveMqttSettings(host, port, user, pass, topic, name, discovery, anonymous);

    if (context->mqtt_host) *context->mqtt_host = host;
    if (context->mqtt_port) *context->mqtt_port = port;
    if (context->mqtt_user) *context->mqtt_user = user;
    if (context->mqtt_pass) *context->mqtt_pass = pass;
    if (context->mqtt_device_name) *context->mqtt_device_name = name;
    if (context->mqtt_base_topic) *context->mqtt_base_topic = topic;
    if (context->mqtt_discovery) *context->mqtt_discovery = discovery;
    if (context->mqtt_anonymous) *context->mqtt_anonymous = anonymous;

    String html = FPSTR(WebTemplates::kMqttSavePage);
    server.send(200, "text/html", html);
    g_deferred_mqtt_sync = true;
    g_deferred_mqtt_sync_due_ms = millis() + kDeferredActionDelayMs;
}

void theme_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->theme_manager) {
        return;
    }
    bool wifi_ready = context->wifi_is_connected && context->wifi_is_connected();
    if (!wifi_ready) {
        context->server->send(403, "text/plain", "WiFi required");
        return;
    }
    if (!context->theme_ui_open || !*context->theme_ui_open) {
        String html = FPSTR(WebTemplates::kThemeLockedPage);
        context->server->send(200, "text/html", html);
        return;
    }
    if (!context->theme_manager->isCustomScreenOpen()) {
        String html = FPSTR(WebTemplates::kThemeLockedPage);
        context->server->send(200, "text/html", html);
        return;
    }
    ThemeColors colors = context->theme_manager->previewOrCurrent();
    String html = FPSTR(WebTemplates::kThemePageTemplate);
    html.replace("{{BG_COLOR}}", theme_color_to_hex(colors.screen_bg));
    html.replace("{{CARD_TOP}}", theme_color_to_hex(colors.card_bg));
    html.replace("{{CARD_BOTTOM}}", theme_color_to_hex(colors.gradient_color));
    html.replace("{{CARD_BORDER}}", theme_color_to_hex(colors.card_border));
    html.replace("{{SHADOW_COLOR}}", theme_color_to_hex(colors.shadow_color));
    html.replace("{{TEXT_COLOR}}", theme_color_to_hex(colors.text_primary));
    html.replace("{{CARD_GRADIENT_BOOL}}", colors.gradient_enabled ? "true" : "false");
    context->server->send(200, "text/html", html);
}

void theme_handle_apply() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->theme_manager) {
        return;
    }
    WebServer &server = *context->server;
    bool wifi_ready = context->wifi_is_connected && context->wifi_is_connected();
    if (!wifi_ready) {
        server.send(403, "text/plain", "WiFi required");
        return;
    }
    if (!context->theme_ui_open || !*context->theme_ui_open) {
        server.send(409, "text/plain", "Open Theme screen to enable");
        return;
    }
    if (!context->theme_manager->isCustomScreenOpen()) {
        server.send(409, "text/plain", "Open Custom Theme screen to enable");
        return;
    }
    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    ThemeColors colors = context->theme_manager->previewOrCurrent();

    lv_color_t parsed = {};
    const char *bg = doc["bg"] | "";
    if (parse_hex_color(String(bg), parsed)) {
        colors.screen_bg = parsed;
        colors.screen_gradient_enabled = false;
        colors.screen_gradient_color = parsed;
        colors.screen_gradient_direction = LV_GRAD_DIR_NONE;
    }
    const char *card_top = doc["card_top"] | "";
    if (parse_hex_color(String(card_top), parsed)) {
        colors.card_bg = parsed;
    }
    const char *card_bottom = doc["card_bottom"] | "";
    if (parse_hex_color(String(card_bottom), parsed)) {
        colors.gradient_color = parsed;
    }
    const char *border = doc["border"] | "";
    if (parse_hex_color(String(border), parsed)) {
        colors.card_border = parsed;
    }
    const char *shadow = doc["shadow"] | "";
    if (parse_hex_color(String(shadow), parsed)) {
        colors.shadow_color = parsed;
    }
    const char *text = doc["text"] | "";
    if (parse_hex_color(String(text), parsed)) {
        colors.text_primary = parsed;
    }

    int gradient_enabled = doc["card_gradient"] | (colors.gradient_enabled ? 1 : 0);
    colors.gradient_enabled = gradient_enabled != 0;
    colors.gradient_direction = colors.gradient_enabled ? LV_GRAD_DIR_VER : LV_GRAD_DIR_NONE;
    colors.shadow_enabled = true;

    if (!lvgl_port_lock(-1)) {
        server.send(503, "text/plain", "LVGL unavailable");
        return;
    }
    context->theme_manager->applyPreviewCustom(colors);
    if (!lvgl_port_unlock()) {
        server.send(500, "text/plain", "LVGL unlock failed");
        return;
    }

    server.send(200, "text/plain", "OK");
}

void dac_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control) {
        return;
    }
    String html = FPSTR(WebTemplates::kDacPageTemplate);
    context->server->send(200, "text/html", html);
}

void dac_handle_state() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control || !context->sensor_data) {
        return;
    }

    const FanControl &fan = *context->fan_control;
    const SensorData &data = *context->sensor_data;
    const bool gas_warmup = context->sensor_manager ? context->sensor_manager->isWarmupActive() : false;

    ArduinoJson::JsonDocument doc;
    doc["success"] = true;

    ArduinoJson::JsonObject dac = doc["dac"].to<ArduinoJson::JsonObject>();
    dac["available"] = fan.isAvailable();
    dac["faulted"] = fan.isFaulted();
    dac["running"] = fan.isRunning();
    dac["manual_override"] = fan.isManualOverrideActive();
    dac["auto_resume_blocked"] = fan.isAutoResumeBlocked();
    dac["output_known"] = fan.isOutputKnown();
    dac["mode"] = (fan.mode() == FanControl::Mode::Manual) ? "manual" : "auto";
    dac["manual_step"] = fan.manualStep();
    dac["selected_timer_s"] = fan.selectedTimerSeconds();
    dac["remaining_s"] = fan.remainingSeconds(millis());
    dac["output_mv"] = fan.outputMillivolts();
    dac["output_percent"] = fan.outputPercent();
    dac["status"] = dac_status_text(fan);

    ArduinoJson::JsonObject auto_cfg = doc["auto"].to<ArduinoJson::JsonObject>();
    DacAutoConfigJson::writeJson(auto_cfg, fan.autoConfig());

    ArduinoJson::JsonObject sensors = doc["sensors"].to<ArduinoJson::JsonObject>();
    sensors["gas_warmup"] = gas_warmup;
    sensors["co2"] = data.co2;
    sensors["co2_valid"] = data.co2_valid;
    sensors["co_ppm"] = data.co_ppm;
    sensors["co_valid"] = data.co_valid && data.co_sensor_present;
    sensors["pm05"] = data.pm05;
    sensors["pm05_valid"] = data.pm05_valid;
    sensors["pm1"] = data.pm1;
    sensors["pm1_valid"] = data.pm1_valid;
    sensors["pm4"] = data.pm4;
    sensors["pm4_valid"] = data.pm4_valid;
    sensors["pm25"] = data.pm25;
    sensors["pm25_valid"] = data.pm25_valid;
    sensors["pm10"] = data.pm10;
    sensors["pm10_valid"] = data.pm10_valid;
    sensors["voc_index"] = data.voc_index;
    sensors["voc_valid"] = data.voc_valid;
    sensors["nox_index"] = data.nox_index;
    sensors["nox_valid"] = data.nox_valid;

    String json;
    serializeJson(doc, json);
    context->server->send(200, "application/json", json);
}

void dac_handle_action() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control || !context->storage) {
        return;
    }
    WebServer &server = *context->server;
    FanControl &fan = *context->fan_control;

    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    const String action = String(doc["action"] | "");
    if (action == "set_mode") {
        const String mode = String(doc["mode"] | "");
        if (mode == "manual") {
            fan.setMode(FanControl::Mode::Manual);
            context->storage->config().dac_auto_mode = false;
            context->storage->saveConfig(true);
        } else if (mode == "auto") {
            fan.setMode(FanControl::Mode::Auto);
            context->storage->config().dac_auto_mode = true;
            context->storage->saveConfig(true);
        } else {
            server.send(400, "text/plain", "Invalid mode");
            return;
        }
    } else if (action == "set_manual_step") {
        fan.setManualStep(doc["step"] | 1);
    } else if (action == "set_timer") {
        fan.setTimerSeconds(doc["seconds"] | 0);
    } else if (action == "start") {
        fan.requestStart();
    } else if (action == "stop") {
        fan.requestStop();
    } else if (action == "start_auto") {
        fan.requestAutoStart();
        context->storage->config().dac_auto_mode = true;
        context->storage->saveConfig(true);
    } else {
        server.send(400, "text/plain", "Unsupported action");
        return;
    }

    server.send(200, "application/json", "{\"success\":true}");
}

void dac_handle_auto() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control || !context->storage) {
        return;
    }
    WebServer &server = *context->server;
    FanControl &fan = *context->fan_control;

    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    DacAutoConfig config = fan.autoConfig();
    ArduinoJson::JsonObjectConst root = doc.as<ArduinoJson::JsonObjectConst>();
    ArduinoJson::JsonObjectConst source = root;
    if (root["auto"].is<ArduinoJson::JsonObjectConst>()) {
        source = root["auto"].as<ArduinoJson::JsonObjectConst>();
    }
    if (!DacAutoConfigJson::readJson(source, config)) {
        server.send(400, "text/plain", "Invalid auto payload");
        return;
    }

    const bool rearm = root["rearm"] | false;
    fan.setAutoConfig(config);
    String serialized = DacAutoConfigJson::serialize(config);
    if (!context->storage->saveTextAtomic(StorageManager::kDacAutoPath, serialized)) {
        server.send(500, "text/plain", "Failed to persist auto config");
        return;
    }
    if (rearm) {
        fan.requestAutoStart();
        if (!context->storage->config().dac_auto_mode) {
            context->storage->config().dac_auto_mode = true;
            context->storage->saveConfig(true);
        }
    }
    server.send(200, "application/json", "{\"success\":true}");
}

void charts_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->charts_history) {
        return;
    }

    WebServer &server = *context->server;
    const ChartsHistory &history = *context->charts_history;

    const char *window_name = "3h";
    uint16_t window_points = chart_window_points(server.arg("window"), window_name);

    const char *group_name = "core";
    const ChartMetricSpec *metrics = nullptr;
    size_t metric_count = 0;
    chart_group_metrics(server.arg("group"), group_name, metrics, metric_count);

    uint16_t total_count = history.count();
    uint16_t available = (total_count < window_points) ? total_count : window_points;
    uint16_t missing_prefix = window_points - available;
    uint16_t start_offset = total_count - available;

    uint32_t latest_epoch = history.latestEpoch();
    bool has_epoch = latest_epoch > Config::TIME_VALID_EPOCH;

    ArduinoJson::JsonDocument doc;
    doc["success"] = true;
    doc["group"] = group_name;
    doc["window"] = window_name;
    doc["step_s"] = kChartStepS;
    doc["points"] = window_points;
    doc["available"] = available;

    ArduinoJson::JsonArray timestamps = doc["timestamps"].to<ArduinoJson::JsonArray>();
    for (uint16_t i = 0; i < window_points; ++i) {
        if (!has_epoch) {
            timestamps.add(nullptr);
            continue;
        }
        uint32_t back_steps = static_cast<uint32_t>(window_points - 1 - i);
        timestamps.add(latest_epoch - back_steps * kChartStepS);
    }

    ArduinoJson::JsonArray series = doc["series"].to<ArduinoJson::JsonArray>();
    for (size_t i = 0; i < metric_count; ++i) {
        const ChartMetricSpec &spec = metrics[i];
        ArduinoJson::JsonObject entry = series.add<ArduinoJson::JsonObject>();
        entry["key"] = spec.key;
        entry["unit"] = spec.unit;

        float latest_value = 0.0f;
        if (chart_latest_metric(history, spec.metric, latest_value)) {
            entry["latest"] = latest_value;
        } else {
            entry["latest"] = nullptr;
        }

        ArduinoJson::JsonArray values = entry["values"].to<ArduinoJson::JsonArray>();
        for (uint16_t slot = 0; slot < window_points; ++slot) {
            if (slot < missing_prefix) {
                values.add(nullptr);
                continue;
            }

            uint16_t offset = start_offset + (slot - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (!history.metricValueFromOldest(offset, spec.metric, value, valid) ||
                !valid || !isfinite(value)) {
                values.add(nullptr);
                continue;
            }
            values.add(value);
        }
    }

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void state_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->sensor_data) {
        return;
    }

    const SensorData &data = *context->sensor_data;
    WebServer &server = *context->server;
    const uint32_t uptime_s = millis() / 1000UL;
    const time_t now_epoch = time(nullptr);

    ArduinoJson::JsonDocument doc;
    doc["success"] = true;
    doc["uptime_s"] = uptime_s;
    doc["timestamp_ms"] = millis();
    if (now_epoch > 0) {
        doc["time_epoch_s"] = static_cast<int64_t>(now_epoch);
    } else {
        doc["time_epoch_s"] = nullptr;
    }

    ArduinoJson::JsonObject sensors = doc["sensors"].to<ArduinoJson::JsonObject>();
    json_set_float_or_null(sensors, "temp", data.temp_valid, data.temperature);
    json_set_float_or_null(sensors, "rh", data.hum_valid, data.humidity);
    json_set_float_or_null(sensors, "pressure", data.pressure_valid, data.pressure);
    json_set_float_or_null(sensors, "pm05", data.pm05_valid, data.pm05);
    json_set_float_or_null(sensors, "pm1", data.pm1_valid, data.pm1);
    json_set_float_or_null(sensors, "pm25", data.pm25_valid, data.pm25);
    json_set_float_or_null(sensors, "pm4", data.pm4_valid, data.pm4);
    json_set_float_or_null(sensors, "pm10", data.pm10_valid, data.pm10);
    json_set_int_or_null(sensors, "co2", data.co2_valid, data.co2);
    json_set_int_or_null(sensors, "voc", data.voc_valid, data.voc_index);
    json_set_int_or_null(sensors, "nox", data.nox_valid, data.nox_index);
    json_set_float_or_null(sensors, "hcho", data.hcho_valid, data.hcho);
    json_set_float_or_null(sensors, "co", data.co_valid && data.co_sensor_present, data.co_ppm);
    sensors["co_sensor_present"] = data.co_sensor_present;
    sensors["co_warmup"] = data.co_warmup;

    ArduinoJson::JsonObject derived = doc["derived"].to<ArduinoJson::JsonObject>();
    const bool climate_valid = data.temp_valid && data.hum_valid;
    const float dew_point = climate_valid ? MathUtils::compute_dew_point_c(data.temperature, data.humidity) : NAN;
    const float abs_humidity = climate_valid ? MathUtils::compute_absolute_humidity_gm3(data.temperature, data.humidity) : NAN;
    const int mold_risk = climate_valid ? MathUtils::compute_mold_risk_index(data.temperature, data.humidity) : -1;
    json_set_float_or_null(derived, "dew_point", climate_valid, dew_point);
    json_set_float_or_null(derived, "ah", climate_valid, abs_humidity);
    if (mold_risk >= 0) {
        derived["mold"] = mold_risk;
    } else {
        derived["mold"] = nullptr;
    }
    json_set_float_or_null(derived, "pressure_delta_3h", data.pressure_delta_3h_valid, data.pressure_delta_3h);
    json_set_float_or_null(derived, "pressure_delta_24h", data.pressure_delta_24h_valid, data.pressure_delta_24h);
    derived["uptime"] = format_uptime_human(uptime_s);

    ArduinoJson::JsonObject network = doc["network"].to<ArduinoJson::JsonObject>();
    const bool wifi_enabled = context->wifi_enabled ? *context->wifi_enabled : false;
    const bool ap_mode = context->wifi_is_ap_mode && context->wifi_is_ap_mode();
    const bool sta_connected = context->wifi_is_connected && context->wifi_is_connected();
    network["wifi_enabled"] = wifi_enabled;
    network["mode"] = ap_mode ? "ap" : (sta_connected ? "sta" : "off");

    String wifi_ssid;
    if (ap_mode) {
        wifi_ssid = WiFi.softAPSSID();
    } else if (sta_connected) {
        wifi_ssid = WiFi.SSID();
    } else if (context->wifi_ssid) {
        wifi_ssid = *context->wifi_ssid;
    }
    network["wifi_ssid"] = wifi_ssid;

    String ip = ap_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    network["ip"] = ip;

    const int rssi = sta_connected ? WiFi.RSSI() : 0;
    if (sta_connected && rssi < 0) {
        network["rssi"] = rssi;
    } else {
        network["rssi"] = nullptr;
    }

    if (context->hostname) {
        network["hostname"] = *context->hostname;
    } else {
        network["hostname"] = "aura";
    }

    if (context->mqtt_host) {
        network["mqtt_broker"] = *context->mqtt_host;
    } else {
        network["mqtt_broker"] = "";
    }
    if (context->mqtt_user_enabled) {
        network["mqtt_enabled"] = *context->mqtt_user_enabled;
    } else {
        network["mqtt_enabled"] = false;
    }
    network["mqtt_connected"] = context->mqtt_client && context->mqtt_client->connected();

    ArduinoJson::JsonObject system = doc["system"].to<ArduinoJson::JsonObject>();
    system["firmware"] = APP_VERSION;
    system["build_date"] = __DATE__;
    system["build_time"] = __TIME__;
    system["uptime"] = format_uptime_human(uptime_s);
    system["dac_available"] = context->fan_control && context->fan_control->isAvailable();

    ArduinoJson::JsonObject settings = doc["settings"].to<ArduinoJson::JsonObject>();
    fill_web_settings_json(settings, *context);

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void settings_handle_update() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }

    WebServer &server = *context->server;
    if (!context->ui_controller) {
        server.send(503, "text/plain", "UI controller unavailable");
        return;
    }
    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    UiController &ui = *context->ui_controller;

    ArduinoJson::JsonVariantConst night_mode_var = doc["night_mode"];
    if (!night_mode_var.isNull()) {
        if (!night_mode_var.is<bool>()) {
            server.send(400, "text/plain", "night_mode must be bool");
            return;
        }
        if (!ui.webSetNightMode(night_mode_var.as<bool>())) {
            server.send(409, "text/plain", "night_mode is locked by auto mode");
            return;
        }
    }

    ArduinoJson::JsonVariantConst backlight_var = doc["backlight_on"];
    if (!backlight_var.isNull()) {
        if (!backlight_var.is<bool>()) {
            server.send(400, "text/plain", "backlight_on must be bool");
            return;
        }
        ui.webSetBacklight(backlight_var.as<bool>());
    }

    ArduinoJson::JsonVariantConst units_c_var = doc["units_c"];
    if (!units_c_var.isNull()) {
        if (!units_c_var.is<bool>()) {
            server.send(400, "text/plain", "units_c must be bool");
            return;
        }
        ui.webSetUnitsC(units_c_var.as<bool>());
    }

    const ArduinoJson::JsonVariantConst temp_offset_var = doc["temp_offset"];
    const ArduinoJson::JsonVariantConst hum_offset_var = doc["hum_offset"];
    const bool has_temp_offset = !temp_offset_var.isNull();
    const bool has_hum_offset = !hum_offset_var.isNull();
    if (has_temp_offset || has_hum_offset) {
        float temp_offset = ui.webTempOffset();
        float hum_offset = ui.webHumOffset();

        if (has_temp_offset) {
            if (!temp_offset_var.is<float>() && !temp_offset_var.is<int>()) {
                server.send(400, "text/plain", "temp_offset must be number");
                return;
            }
            temp_offset = temp_offset_var.as<float>();
        }
        if (has_hum_offset) {
            if (!hum_offset_var.is<float>() && !hum_offset_var.is<int>()) {
                server.send(400, "text/plain", "hum_offset must be number");
                return;
            }
            hum_offset = hum_offset_var.as<float>();
        }

        ui.webSetOffsets(temp_offset, hum_offset);
    }

    bool restart_requested = false;
    ArduinoJson::JsonVariantConst restart_var = doc["restart"];
    if (!restart_var.isNull()) {
        if (!restart_var.is<bool>()) {
            server.send(400, "text/plain", "restart must be bool");
            return;
        }
        restart_requested = restart_var.as<bool>();
    }

    ArduinoJson::JsonDocument response_doc;
    response_doc["success"] = true;
    response_doc["restart"] = restart_requested;
    ArduinoJson::JsonObject response_settings = response_doc["settings"].to<ArduinoJson::JsonObject>();
    fill_web_settings_json(response_settings, *context);

    String json;
    serializeJson(response_doc, json);
    server.send(200, "application/json", json);

    if (restart_requested) {
        ui.webRequestRestart();
    }
}

void ota_handle_upload() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }

    WebServer &server = *context->server;
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        ota_reset_state();
        g_ota_upload_seen = true;
        g_ota_upload_active = true;
        ota_set_ui_screen(true);

        const esp_partition_t *target_partition = esp_ota_get_next_update_partition(nullptr);
        if (!target_partition) {
            ota_set_error("OTA partition unavailable");
            LOGE("OTA", "no target partition");
            return;
        }
        g_ota_slot_size = target_partition->size;

        size_t expected_size = 0;
        g_ota_size_known = parse_size_arg(server.arg("ota_size"), expected_size);
        if (g_ota_size_known) {
            g_ota_expected_size = expected_size;
            if (g_ota_expected_size > g_ota_slot_size) {
                ota_set_error(String("Firmware too large for OTA slot: ") +
                              String(g_ota_expected_size) + " > " + String(g_ota_slot_size));
                LOGW("OTA", "reject oversized image: %u > %u",
                     static_cast<unsigned>(g_ota_expected_size),
                     static_cast<unsigned>(g_ota_slot_size));
                return;
            }
            if (!Update.begin(g_ota_expected_size, U_FLASH)) {
                ota_set_error(ota_error_prefixed("Update begin failed"));
                LOGE("OTA", "%s", g_ota_error.c_str());
                return;
            }
        } else {
            g_ota_expected_size = 0;
            if (!Update.begin(g_ota_slot_size, U_FLASH)) {
                ota_set_error(ota_error_prefixed("Update begin failed"));
                LOGE("OTA", "%s", g_ota_error.c_str());
                return;
            }
        }

        LOGI("OTA", "upload started (slot=%u, expected=%u, known=%s)",
             static_cast<unsigned>(g_ota_slot_size),
             static_cast<unsigned>(g_ota_expected_size),
             g_ota_size_known ? "YES" : "NO");
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (!g_ota_upload_active || !g_ota_error.isEmpty()) {
            return;
        }
        if (upload.currentSize == 0) {
            return;
        }
        if (g_ota_written_size + upload.currentSize > g_ota_slot_size) {
            if (Update.isRunning()) {
                Update.abort();
            }
            ota_set_error(String("Firmware too large for OTA slot: ") +
                          String(g_ota_written_size + upload.currentSize) +
                          " > " + String(g_ota_slot_size));
            LOGW("OTA", "upload exceeded slot size");
            return;
        }
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            if (Update.isRunning()) {
                Update.abort();
            }
            ota_set_error(ota_error_prefixed("Update write failed"));
            LOGE("OTA", "%s", g_ota_error.c_str());
            return;
        }
        g_ota_written_size += upload.currentSize;
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        if (!g_ota_upload_active || !g_ota_error.isEmpty()) {
            if (Update.isRunning()) {
                Update.abort();
            }
            return;
        }
        if (g_ota_size_known && g_ota_expected_size > 0 && g_ota_written_size != g_ota_expected_size) {
            if (Update.isRunning()) {
                Update.abort();
            }
            ota_set_error(String("Firmware size mismatch: expected ") +
                          String(g_ota_expected_size) + ", got " + String(g_ota_written_size));
            LOGW("OTA", "size mismatch");
            return;
        }
        if (!Update.end(true)) {
            ota_set_error(ota_error_prefixed("Update finalize failed"));
            LOGE("OTA", "%s", g_ota_error.c_str());
            return;
        }
        g_ota_upload_success = true;
        g_ota_upload_active = false;
        LOGI("OTA", "upload complete, written=%u bytes", static_cast<unsigned>(g_ota_written_size));
        return;
    }

    if (upload.status == UPLOAD_FILE_ABORTED) {
        if (Update.isRunning()) {
            Update.abort();
        }
        ota_set_error("Upload aborted");
        LOGW("OTA", "upload aborted");
    }
}

void ota_handle_update() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }

    WebServer &server = *context->server;
    const bool has_upload = g_ota_upload_seen;
    const bool success = has_upload && g_ota_upload_success && g_ota_error.isEmpty();
    const size_t written_size = g_ota_written_size;
    const size_t slot_size = g_ota_slot_size;
    const bool size_known = g_ota_size_known;
    const size_t expected_size = g_ota_expected_size;
    String error = g_ota_error;

    if (!has_upload && error.isEmpty()) {
        error = "Firmware file is missing";
    } else if (!success && error.isEmpty()) {
        error = "OTA update failed";
    }

    int status_code = 200;
    if (!success) {
        if (error.indexOf("too large") >= 0) {
            status_code = 413;
        } else if (error.indexOf("missing") >= 0 ||
                   error.indexOf("aborted") >= 0 ||
                   error.indexOf("mismatch") >= 0) {
            status_code = 400;
        } else {
            status_code = 500;
        }
    }

    ArduinoJson::JsonDocument doc;
    doc["success"] = success;
    doc["written"] = static_cast<uint32_t>(written_size);
    doc["slot_size"] = static_cast<uint32_t>(slot_size);
    if (size_known) {
        doc["expected"] = static_cast<uint32_t>(expected_size);
    } else {
        doc["expected"] = nullptr;
    }

    if (success) {
        doc["message"] = "Firmware uploaded. Device will reboot.";
        doc["rebooting"] = true;
    } else {
        doc["error"] = error;
        doc["rebooting"] = false;
    }

    String json;
    serializeJson(doc, json);
    if (success) {
        server.client().setNoDelay(true);
    }
    server.send(status_code, "application/json", json);

    if (success) {
        delay(300);
        server.client().stop();
        ESP.restart();
    } else {
        ota_set_ui_screen(false);
    }
    ota_reset_state();
}

void events_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }

    WebServer &server = *context->server;
    const size_t count = Logger::copyRecent(g_events_snapshot, kEventsApiMaxEntries);

    ArduinoJson::JsonDocument doc;
    doc["success"] = true;
    uint32_t emitted_count = 0;
    doc["count"] = static_cast<uint32_t>(count);
    doc["uptime_s"] = millis() / 1000UL;

    ArduinoJson::JsonArray events = doc["events"].to<ArduinoJson::JsonArray>();
    for (size_t i = 0; i < count; ++i) {
        const Logger::RecentEntry &entry = g_events_snapshot[i];
        // Keep memory telemetry in serial monitor, but hide it from web Events feed.
        if (strcmp(entry.tag, "Mem") == 0) {
            continue;
        }
        ArduinoJson::JsonObject e = events.add<ArduinoJson::JsonObject>();
        e["ts_ms"] = entry.ms;
        e["level"] = event_level_text(entry.level);
        e["severity"] = event_severity_text(entry.level);
        e["type"] = entry.tag[0] ? entry.tag : "SYSTEM";
        e["message"] = entry.message[0] ? entry.message : "Event";
        emitted_count++;
    }
    doc["count"] = emitted_count;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}
