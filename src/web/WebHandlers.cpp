// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebHandlers.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <WiFi.h>
#include <Update.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "config/AppConfig.h"
#include "config/AppData.h"
#include "core/MathUtils.h"
#include "core/Logger.h"
#include "core/WifiPowerSaveGuard.h"
#include "core/Watchdog.h"
#include "modules/ChartsHistory.h"
#include "modules/DacAutoConfig.h"
#include "modules/FanControl.h"
#include "modules/SensorManager.h"
#include "modules/StorageManager.h"
#include "web/WebInputValidation.h"
#include "web/OtaDeferredRestart.h"
#include "web/WebTemplates.h"
#include "ui/UiController.h"
#include "ui/ThemeManager.h"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

WebHandlerContext *g_ctx = nullptr;
constexpr uint32_t kDeferredActionDelayMs = 200;
constexpr uint32_t kDeferredRestartDelayMs = 1500;
constexpr uint32_t kTaskWdtDefaultMs = 180000;
constexpr uint32_t kTaskWdtOtaMs = 10UL * 60UL * 1000UL;
bool g_deferred_wifi_start_sta = false;
bool g_deferred_mqtt_sync = false;
OtaDeferredRestart::Controller g_restart_controller;
uint32_t g_deferred_wifi_start_sta_due_ms = 0;
uint32_t g_deferred_mqtt_sync_due_ms = 0;
constexpr uint32_t kChartStepS = Config::CHART_HISTORY_STEP_MS / 1000UL;
constexpr size_t kEventsApiMaxEntries = 48;
constexpr size_t kWebDisplayNameMaxLen = 32;
constexpr size_t kWifiScanMaxItems = 15;
constexpr size_t kDiagMaxErrorItems = 12;
constexpr uint32_t kHttpStreamSlowWriteWarnMs = 200;

struct StreamProfile {
    size_t chunk_size;
    uint16_t max_zero_writes;
    uint8_t yield_ms;
    uint16_t retry_delay_fast_ms;
    uint16_t retry_delay_medium_ms;
    uint16_t retry_delay_slow_ms;
    uint16_t retry_delay_very_slow_ms;
    uint16_t retry_delay_max_ms;
    uint32_t max_duration_ms;
    uint32_t no_progress_timeout_ms;
    bool disable_wifi_power_save;
};

constexpr StreamProfile kHtmlStreamProfile = {
    1460,
    2048,
    1,
    2,
    12,
    50,
    100,
    150,
    45000,
    10000,
    false
};

constexpr StreamProfile kImmutableAssetStreamProfile = {
    1024,
    2048,
    1,
    2,
    12,
    50,
    100,
    150,
    60000,
    15000,
    true
};
constexpr const char kApiErrorOtaBusyJson[] =
    "{\"success\":false,\"error\":\"OTA upload in progress\","
    "\"error_code\":\"OTA_BUSY\",\"ota_busy\":true}";
Logger::RecentEntry g_events_snapshot[kEventsApiMaxEntries];
bool g_ota_upload_seen = false;
bool g_ota_upload_active = false;
bool g_ota_upload_success = false;
bool g_ota_size_known = false;
size_t g_ota_expected_size = 0;
size_t g_ota_slot_size = 0;
size_t g_ota_written_size = 0;
String g_ota_error;
bool g_ota_wdt_extended = false;
uint32_t g_ota_upload_start_ms = 0;
uint32_t g_ota_first_chunk_ms = 0;
uint32_t g_ota_last_chunk_ms = 0;
uint32_t g_ota_finalize_ms = 0;
uint32_t g_ota_chunk_count = 0;
size_t g_ota_chunk_min_size = 0;
size_t g_ota_chunk_max_size = 0;
size_t g_ota_chunk_sum_size = 0;
bool g_ota_first_chunk_seen = false;
bool g_ota_start_rssi_valid = false;
int g_ota_start_rssi = 0;
bool g_ota_wifi_ps_saved = false;
wifi_ps_type_t g_ota_wifi_ps_prev = WIFI_PS_NONE;

void ota_disable_wifi_power_save_for_upload();
void ota_restore_wifi_power_save();

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

void send_no_store_headers(WebServer &server);

bool diag_access_allowed(const WebHandlerContext *context) {
    if (!context) {
        return false;
    }
    const bool ap_mode = context->wifi_is_ap_mode && context->wifi_is_ap_mode();
    const bool sta_connected = context->wifi_is_connected && context->wifi_is_connected();
    return ap_mode || sta_connected;
}

void send_ota_busy_json(WebServer &server) {
    send_no_store_headers(server);
    server.send(503, "application/json", kApiErrorOtaBusyJson);
}

bool persist_dac_auto_mode(StorageManager &storage, bool enabled) {
    const bool previous = storage.config().dac_auto_mode;
    if (previous == enabled) {
        return true;
    }
    storage.config().dac_auto_mode = enabled;
    if (!storage.saveConfig(true)) {
        storage.config().dac_auto_mode = previous;
        return false;
    }
    return true;
}

bool parse_dac_timer_seconds(const ArduinoJson::JsonVariantConst &value, uint32_t &out_seconds) {
    if (!value.is<int>() && !value.is<unsigned int>()) {
        return false;
    }
    const int32_t raw_seconds = value.as<int32_t>();
    if (raw_seconds < 0) {
        return false;
    }
    const uint32_t seconds = static_cast<uint32_t>(raw_seconds);
    if (Config::isDacTimerPresetSeconds(seconds)) {
        out_seconds = seconds;
        return true;
    }
    return false;
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

uint32_t ota_upload_timeout_ms(size_t image_size_bytes) {
    constexpr uint32_t kMinTimeoutMs = 180000;
    constexpr uint32_t kMaxTimeoutMs = 900000;
    constexpr uint32_t kMinUploadBytesPerSec = 20 * 1024;
    constexpr uint32_t kOverheadMs = 120000;

    if (image_size_bytes == 0) {
        return kMaxTimeoutMs;
    }

    const uint64_t transfer_ms =
        (static_cast<uint64_t>(image_size_bytes) * 1000ULL + kMinUploadBytesPerSec - 1) /
        kMinUploadBytesPerSec;
    const uint64_t timeout_ms = transfer_ms + kOverheadMs;
    if (timeout_ms <= kMinTimeoutMs) {
        return kMinTimeoutMs;
    }
    if (timeout_ms >= kMaxTimeoutMs) {
        return kMaxTimeoutMs;
    }
    return static_cast<uint32_t>(timeout_ms);
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
    g_ota_upload_start_ms = 0;
    g_ota_first_chunk_ms = 0;
    g_ota_last_chunk_ms = 0;
    g_ota_finalize_ms = 0;
    g_ota_chunk_count = 0;
    g_ota_chunk_min_size = 0;
    g_ota_chunk_max_size = 0;
    g_ota_chunk_sum_size = 0;
    g_ota_first_chunk_seen = false;
    g_ota_start_rssi_valid = false;
    g_ota_start_rssi = 0;
}

void ota_extend_task_wdt() {
    if (g_ota_wdt_extended) {
        return;
    }
    if (Watchdog::setup(kTaskWdtOtaMs)) {
        g_ota_wdt_extended = true;
        LOGI("OTA", "Task WDT extended to %u ms for upload",
             static_cast<unsigned>(kTaskWdtOtaMs));
    } else {
        LOGW("OTA", "failed to extend Task WDT for upload");
    }
}

void ota_restore_task_wdt() {
    if (!g_ota_wdt_extended) {
        return;
    }
    if (Watchdog::setup(kTaskWdtDefaultMs)) {
        g_ota_wdt_extended = false;
        LOGI("OTA", "Task WDT restored to %u ms",
             static_cast<unsigned>(kTaskWdtDefaultMs));
    } else {
        LOGW("OTA", "failed to restore Task WDT");
    }
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
    ota_restore_wifi_power_save();
    ota_restore_task_wdt();
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

void send_no_store_headers(WebServer &server) {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
}

void send_no_store_text(WebServer &server, int status_code, const char *message) {
    send_no_store_headers(server);
    server.send(status_code, "text/plain", message);
}

void send_immutable_headers(WebServer &server) {
    server.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
}

enum class AssetCacheMode : uint8_t {
    NoStore = 0,
    Immutable,
};

enum class StreamAbortReason : uint8_t {
    None = 0,
    Disconnected,
    InvalidSocket,
    ZeroWriteLimit,
    NoProgressTimeout,
    TotalTimeout,
    SocketWriteError
};

const char *stream_abort_reason_text(StreamAbortReason reason) {
    switch (reason) {
        case StreamAbortReason::Disconnected:
            return "client_disconnected";
        case StreamAbortReason::InvalidSocket:
            return "invalid_socket";
        case StreamAbortReason::ZeroWriteLimit:
            return "zero_write_limit";
        case StreamAbortReason::NoProgressTimeout:
            return "no_progress_timeout";
        case StreamAbortReason::TotalTimeout:
            return "total_timeout";
        case StreamAbortReason::SocketWriteError:
            return "socket_write_error";
        case StreamAbortReason::None:
        default:
            return "none";
    }
}

struct WebStreamStats {
    uint32_t ok_count = 0;
    uint32_t abort_count = 0;
    uint32_t slow_count = 0;
    StreamAbortReason last_abort_reason = StreamAbortReason::None;
    int last_errno = 0;
    size_t last_sent = 0;
    size_t last_total = 0;
    uint32_t last_max_write_ms = 0;
    char last_uri[96] = {};
};

WebStreamStats g_web_stream_stats;

void apply_asset_cache_headers(WebServer &server, AssetCacheMode cache_mode) {
    if (cache_mode == AssetCacheMode::Immutable) {
        send_immutable_headers(server);
        return;
    }
    send_no_store_headers(server);
}

void record_web_stream_result(const String &uri,
                              size_t total_size,
                              size_t sent,
                              bool ok,
                              StreamAbortReason abort_reason,
                              uint32_t max_write_ms,
                              int last_socket_errno) {
    WebStreamStats &stats = g_web_stream_stats;
    if (ok) {
        stats.ok_count++;
    } else {
        stats.abort_count++;
    }
    if (max_write_ms >= kHttpStreamSlowWriteWarnMs) {
        stats.slow_count++;
    }

    stats.last_abort_reason = abort_reason;
    stats.last_errno = last_socket_errno;
    stats.last_sent = sent;
    stats.last_total = total_size;
    stats.last_max_write_ms = max_write_ms;

    const size_t copy_len = uri.length() < (sizeof(stats.last_uri) - 1)
        ? uri.length()
        : (sizeof(stats.last_uri) - 1);
    memcpy(stats.last_uri, uri.c_str(), copy_len);
    stats.last_uri[copy_len] = '\0';
}

void ota_disable_wifi_power_save_for_upload() {
    const wifi_mode_t mode = WiFi.getMode();
    if ((mode & WIFI_MODE_STA) == 0) {
        g_ota_wifi_ps_saved = false;
        g_ota_wifi_ps_prev = WIFI_PS_NONE;
        return;
    }

    wifi_ps_type_t current_ps = WIFI_PS_NONE;
    const esp_err_t get_err = esp_wifi_get_ps(&current_ps);
    if (get_err != ESP_OK) {
        LOGW("OTA", "failed to read WiFi power-save mode: %s", esp_err_to_name(get_err));
        g_ota_wifi_ps_saved = false;
        g_ota_wifi_ps_prev = WIFI_PS_NONE;
        return;
    }

    g_ota_wifi_ps_prev = current_ps;
    g_ota_wifi_ps_saved = true;
    if (current_ps == WIFI_PS_NONE) {
        return;
    }

    const esp_err_t set_err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (set_err == ESP_OK) {
        LOGI("OTA", "WiFi power-save disabled for OTA (prev=%d)", static_cast<int>(current_ps));
        return;
    }

    LOGW("OTA", "failed to disable WiFi power-save for OTA: %s", esp_err_to_name(set_err));
    g_ota_wifi_ps_saved = false;
    g_ota_wifi_ps_prev = WIFI_PS_NONE;
}

void ota_restore_wifi_power_save() {
    if (!g_ota_wifi_ps_saved) {
        return;
    }

    const wifi_ps_type_t restore_mode = g_ota_wifi_ps_prev;
    const esp_err_t set_err = esp_wifi_set_ps(restore_mode);
    if (set_err == ESP_OK) {
        LOGI("OTA", "WiFi power-save restored after OTA (mode=%d)", static_cast<int>(restore_mode));
    } else {
        LOGW("OTA", "failed to restore WiFi power-save mode=%d: %s",
             static_cast<int>(restore_mode),
             esp_err_to_name(set_err));
    }
    g_ota_wifi_ps_saved = false;
    g_ota_wifi_ps_prev = WIFI_PS_NONE;
}

uint16_t stream_retry_delay_ms(const StreamProfile &profile, uint16_t zero_writes) {
    if (zero_writes <= 3) {
        return profile.retry_delay_fast_ms;
    }
    if (zero_writes <= 10) {
        return profile.retry_delay_medium_ms;
    }
    if (zero_writes <= 40) {
        return profile.retry_delay_slow_ms;
    }
    if (zero_writes <= 120) {
        return profile.retry_delay_very_slow_ms;
    }
    return profile.retry_delay_max_ms;
}

bool wait_for_socket_writable(int socket_fd, uint16_t wait_ms, int &last_socket_errno) {
    if (wait_ms == 0) {
        return true;
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(socket_fd, &write_fds);

    struct timeval timeout = {};
    timeout.tv_sec = static_cast<long>(wait_ms / 1000U);
    timeout.tv_usec = static_cast<long>((wait_ms % 1000U) * 1000U);

    errno = 0;
    const int select_result = ::select(socket_fd + 1, nullptr, &write_fds, nullptr, &timeout);
    if (select_result > 0) {
        last_socket_errno = 0;
        return FD_ISSET(socket_fd, &write_fds) != 0;
    }
    if (select_result == 0) {
        last_socket_errno = EAGAIN;
        return false;
    }

    last_socket_errno = errno;
    if (last_socket_errno == EINTR) {
        return false;
    }
    return false;
}

bool stream_client_bytes(NetworkClient &client,
                         const uint8_t *data,
                         size_t size,
                         const StreamProfile &profile,
                         size_t &sent,
                         StreamAbortReason &abort_reason,
                         uint32_t &max_write_ms,
                         int &last_socket_errno) {
    sent = 0;
    abort_reason = StreamAbortReason::None;
    max_write_ms = 0;
    last_socket_errno = 0;
    if (!data || size == 0) {
        return true;
    }

    const int socket_fd = client.fd();
    if (socket_fd < 0) {
        abort_reason = StreamAbortReason::InvalidSocket;
        last_socket_errno = EBADF;
        client.stop();
        return false;
    }

    uint16_t zero_writes = 0;
    const uint32_t start_ms = millis();
    uint32_t last_progress_ms = start_ms;
    while (sent < size) {
        Watchdog::kick();

        size_t to_send = size - sent;
        if (to_send > profile.chunk_size) {
            to_send = profile.chunk_size;
        }

        const uint32_t write_start_ms = millis();
        errno = 0;
        const ssize_t written = ::send(
            socket_fd,
            reinterpret_cast<const char *>(data + sent),
            static_cast<int>(to_send),
            MSG_DONTWAIT
        );
        const uint32_t write_elapsed_ms = millis() - write_start_ms;
        if (write_elapsed_ms > max_write_ms) {
            max_write_ms = write_elapsed_ms;
        }

        const uint32_t now_ms = millis();
        if (written > 0) {
            sent += static_cast<size_t>(written);
            zero_writes = 0;
            last_progress_ms = now_ms;
            last_socket_errno = 0;
            if (deadline_reached(now_ms, start_ms + profile.max_duration_ms)) {
                abort_reason = StreamAbortReason::TotalTimeout;
                client.stop();
                return false;
            }
            if (sent < size) {
                delay(profile.yield_ms);
                Watchdog::kick();
            }
            continue;
        }

        if (written == 0) {
            if (!client.connected()) {
                abort_reason = StreamAbortReason::Disconnected;
                client.stop();
                return false;
            }
            last_socket_errno = 0;
        } else {
            last_socket_errno = errno;
            if (last_socket_errno == ENOTCONN
#ifdef EPIPE
                || last_socket_errno == EPIPE
#endif
#ifdef ECONNRESET
                || last_socket_errno == ECONNRESET
#endif
                ) {
                abort_reason = StreamAbortReason::Disconnected;
                client.stop();
                return false;
            }
            if (last_socket_errno != EAGAIN
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
                && last_socket_errno != EWOULDBLOCK
#endif
#ifdef ENOBUFS
                && last_socket_errno != ENOBUFS
#endif
                && last_socket_errno != EINTR) {
                abort_reason = StreamAbortReason::SocketWriteError;
                client.stop();
                return false;
            }
        }

        if (++zero_writes > profile.max_zero_writes) {
            abort_reason = StreamAbortReason::ZeroWriteLimit;
            client.stop();
            return false;
        }
        if (deadline_reached(now_ms, start_ms + profile.max_duration_ms)) {
            abort_reason = StreamAbortReason::TotalTimeout;
            client.stop();
            return false;
        }
        if (deadline_reached(now_ms, last_progress_ms + profile.no_progress_timeout_ms)) {
            abort_reason = StreamAbortReason::NoProgressTimeout;
            client.stop();
            return false;
        }
        const uint16_t retry_wait_ms = stream_retry_delay_ms(profile, zero_writes);
        const bool writable = wait_for_socket_writable(socket_fd, retry_wait_ms, last_socket_errno);
        if (!writable
            && last_socket_errno != 0
            && last_socket_errno != EAGAIN
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
            && last_socket_errno != EWOULDBLOCK
#endif
            && last_socket_errno != EINTR) {
            abort_reason = StreamAbortReason::SocketWriteError;
            client.stop();
            return false;
        }
        if (!writable && last_socket_errno == EINTR) {
            last_socket_errno = 0;
        }
        if (!writable) {
            delay(profile.yield_ms);
        }
        Watchdog::kick();
    }
    return true;
}

bool send_html_stream(WebServer &server, const String &html) {
    const size_t body_size = html.length();
    send_no_store_headers(server);
    server.setContentLength(body_size);
    server.send(200, "text/html; charset=utf-8", "");
    if (body_size == 0) {
        record_web_stream_result(server.uri(), body_size, 0, true, StreamAbortReason::None, 0, 0);
        return true;
    }

    size_t sent = 0;
    StreamAbortReason abort_reason = StreamAbortReason::None;
    uint32_t max_write_ms = 0;
    int last_socket_errno = 0;
    const bool ok = stream_client_bytes(
        server.client(),
        reinterpret_cast<const uint8_t *>(html.c_str()),
        body_size,
        kHtmlStreamProfile,
        sent,
        abort_reason,
        max_write_ms,
        last_socket_errno
    );
    record_web_stream_result(server.uri(), body_size, sent, ok, abort_reason, max_write_ms, last_socket_errno);
    if (!ok) {
        Logger::log(Logger::Warn, "Web",
                    "HTML stream interrupted: uri=%s sent=%u/%u reason=%s max_write_ms=%u err=%d",
                    server.uri().c_str(),
                    static_cast<unsigned>(sent),
                    static_cast<unsigned>(body_size),
                    stream_abort_reason_text(abort_reason),
                    static_cast<unsigned>(max_write_ms),
                    last_socket_errno);
    } else if (max_write_ms >= kHttpStreamSlowWriteWarnMs) {
        Logger::log(Logger::Warn, "Web",
                    "HTML stream slow write: uri=%s size=%u max_write_ms=%u",
                    server.uri().c_str(),
                    static_cast<unsigned>(body_size),
                    static_cast<unsigned>(max_write_ms));
    }
    return ok;
}

bool send_progmem_asset(WebServer &server,
                        const char *content_type,
                        const uint8_t *content,
                        size_t content_size,
                        bool gzip_encoded,
                        AssetCacheMode cache_mode) {
    const StreamProfile &profile =
        (cache_mode == AssetCacheMode::Immutable) ? kImmutableAssetStreamProfile : kHtmlStreamProfile;
    WifiPowerSaveGuard wifi_ps_guard;
    if (profile.disable_wifi_power_save) {
        wifi_ps_guard.suspend();
    }
    apply_asset_cache_headers(server, cache_mode);
    server.setContentLength(content_size);
    if (gzip_encoded) {
        server.sendHeader("Content-Encoding", "gzip");
    }
    server.send(200, content_type, "");
    if (content_size == 0) {
        record_web_stream_result(server.uri(), content_size, 0, true, StreamAbortReason::None, 0, 0);
        return true;
    }

    size_t sent = 0;
    StreamAbortReason abort_reason = StreamAbortReason::None;
    uint32_t max_write_ms = 0;
    int last_socket_errno = 0;
    const bool ok = stream_client_bytes(server.client(),
                                        content,
                                        content_size,
                                        profile,
                                        sent,
                                        abort_reason,
                                        max_write_ms,
                                        last_socket_errno);
    record_web_stream_result(server.uri(), content_size, sent, ok, abort_reason, max_write_ms, last_socket_errno);
    if (!ok) {
        Logger::log(Logger::Warn, "Web",
                    "PROGMEM asset stream interrupted: uri=%s sent=%u/%u reason=%s max_write_ms=%u err=%d",
                    server.uri().c_str(),
                    static_cast<unsigned>(sent),
                    static_cast<unsigned>(content_size),
                    stream_abort_reason_text(abort_reason),
                    static_cast<unsigned>(max_write_ms),
                    last_socket_errno);
    } else if (max_write_ms >= kHttpStreamSlowWriteWarnMs) {
        Logger::log(Logger::Warn, "Web",
                    "PROGMEM asset stream slow write: uri=%s size=%u max_write_ms=%u",
                    server.uri().c_str(),
                    static_cast<unsigned>(content_size),
                    static_cast<unsigned>(max_write_ms));
    }
    return ok;
}

bool send_html_stream_progmem(WebServer &server, const uint8_t *content, size_t content_size, bool gzip_encoded) {
    return send_progmem_asset(server,
                              "text/html; charset=utf-8",
                              content,
                              content_size,
                              gzip_encoded,
                              AssetCacheMode::NoStore);
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

bool event_tag_in_list(const char *tag, const char *const *list, size_t count) {
    if (!tag || tag[0] == '\0' || !list || count == 0) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        if (list[i] && strcmp(tag, list[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool should_emit_web_event(const Logger::RecentEntry &entry) {
    // Keep memory telemetry in serial monitor only.
    if (strcmp(entry.tag, "Mem") == 0) {
        return false;
    }

    // Dashboard Events should not include debug chatter.
    if (entry.level == Logger::Debug) {
        return false;
    }

    // Warnings/errors are always relevant for dashboard visibility.
    if (entry.level == Logger::Error || entry.level == Logger::Warn) {
        return true;
    }

    // Curated info-level tags for dashboard Events.
    static const char *const kInfoTags[] = {
        "OTA",
        "WiFi",
        "mDNS",
        "Time",
        "MQTT",
        "Storage",
        "Main",
        "Sensors",
        "PressureHistory",
        "ChartsHistory",
        "UI",
    };
    return event_tag_in_list(entry.tag, kInfoTags, sizeof(kInfoTags) / sizeof(kInfoTags[0]));
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
    } else if (context.storage) {
        const auto &cfg = context.storage->config();
        settings["night_mode"] = cfg.night_mode;
        settings["night_mode_locked"] = cfg.auto_night_enabled;
        settings["backlight_on"] = nullptr;
        settings["units_c"] = cfg.units_c;
        settings["temp_offset"] = cfg.temp_offset;
        settings["hum_offset"] = cfg.hum_offset;
    } else {
        settings["night_mode"] = nullptr;
        settings["night_mode_locked"] = nullptr;
        settings["backlight_on"] = nullptr;
        settings["units_c"] = nullptr;
        settings["temp_offset"] = nullptr;
        settings["hum_offset"] = nullptr;
    }

    if (context.storage) {
        settings["display_name"] = context.storage->config().web_display_name;
    } else {
        settings["display_name"] = nullptr;
    }
}

} // namespace

void WebHandlersInit(WebHandlerContext *context) {
    g_ctx = context;
    g_deferred_wifi_start_sta = false;
    g_deferred_mqtt_sync = false;
    g_restart_controller.reset();
    g_deferred_wifi_start_sta_due_ms = 0;
    g_deferred_mqtt_sync_due_ms = 0;
    ota_reset_state();
}

bool WebHandlersIsOtaBusy() {
    return g_restart_controller.is_busy(g_ota_upload_active);
}

bool WebHandlersConsumeRestartRequest() {
    return g_restart_controller.consume_request();
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

    if (g_restart_controller.poll(now_ms)) {
        LOGI("OTA", "deferred reboot: restart requested");
    }
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
    struct WifiScanRow {
        String ssid;
        int rssi;
        int quality;
        bool open;
    };
    WifiScanRow rows[kWifiScanMaxItems];
    int row_count = 0;

    for (int i = 0; i < count; i++) {
        String ssid_raw = WiFi.SSID(i);
        if (ssid_raw.isEmpty()) {
            continue;
        }

        const int rssi = WiFi.RSSI(i);
        const int quality = wifi_rssi_to_quality(rssi);
        const bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

        int duplicate_index = -1;
        for (int j = 0; j < row_count; j++) {
            if (rows[j].ssid == ssid_raw) {
                duplicate_index = j;
                break;
            }
        }
        if (duplicate_index >= 0) {
            if (rssi > rows[duplicate_index].rssi) {
                rows[duplicate_index].rssi = rssi;
                rows[duplicate_index].quality = quality;
                rows[duplicate_index].open = open;
            }
            continue;
        }

        if (row_count < static_cast<int>(kWifiScanMaxItems)) {
            rows[row_count].ssid = ssid_raw;
            rows[row_count].rssi = rssi;
            rows[row_count].quality = quality;
            rows[row_count].open = open;
            row_count++;
            continue;
        }

        int weakest_index = 0;
        for (int j = 1; j < row_count; j++) {
            if (rows[j].rssi < rows[weakest_index].rssi) {
                weakest_index = j;
            }
        }
        if (rssi <= rows[weakest_index].rssi) {
            continue;
        }
        rows[weakest_index].ssid = ssid_raw;
        rows[weakest_index].rssi = rssi;
        rows[weakest_index].quality = quality;
        rows[weakest_index].open = open;
    }

    for (int i = 1; i < row_count; i++) {
        int j = i;
        while (j > 0 && rows[j].rssi > rows[j - 1].rssi) {
            WifiScanRow tmp = rows[j - 1];
            rows[j - 1] = rows[j];
            rows[j] = tmp;
            j--;
        }
    }

    context->wifi_scan_options->reserve(row_count * 170);
    for (int i = 0; i < row_count; i++) {
        String ssid_label = wifi_label_safe(rows[i].ssid);
        String ssid_html = html_escape(ssid_label);
        const char *security = rows[i].open ? "Open" : "Secure";
        const String rssi_text = String(rows[i].rssi) + " dBm";

        (*context->wifi_scan_options) += "<div class=\"network-item\" data-ssid=\"";
        (*context->wifi_scan_options) += ssid_html;
        (*context->wifi_scan_options) += "\"><div class=\"network-icon\" aria-hidden=\"true\"></div>";
        (*context->wifi_scan_options) += "<div class=\"network-info\"><span class=\"network-name\">";
        (*context->wifi_scan_options) += ssid_html;
        (*context->wifi_scan_options) += "</span><span class=\"network-meta\">";
        (*context->wifi_scan_options) += security;
        (*context->wifi_scan_options) += " | ";
        (*context->wifi_scan_options) += rssi_text;
        (*context->wifi_scan_options) += "</span></div><div class=\"network-signal\">";
        (*context->wifi_scan_options) += String(rows[i].quality);
        (*context->wifi_scan_options) += "%</div></div>";
    }
}

void wifi_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (!context->wifi_is_ap_mode || !context->wifi_is_ap_mode()) {
        send_no_store_headers(*context->server);
        context->server->send(404, "text/plain", "Not found");
        return;
    }
    WebServer &server = *context->server;

    if (server.hasArg("scan_status")) {
        ArduinoJson::JsonDocument doc;
        doc["success"] = true;
        doc["scan_in_progress"] = context->wifi_scan_in_progress && *context->wifi_scan_in_progress;
        String json;
        serializeJson(doc, json);
        send_no_store_headers(server);
        server.send(200, "application/json", json);
        return;
    }

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
    send_html_stream(server, html);
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

    send_html_stream_progmem(
        *context->server,
        WebTemplates::kDashboardShellHtmlGzip,
        WebTemplates::kDashboardShellHtmlGzipSize,
        true
    );
}

void dashboard_handle_styles() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    send_progmem_asset(*context->server,
                       "text/css; charset=utf-8",
                       WebTemplates::kDashboardStylesCssGzip,
                       WebTemplates::kDashboardStylesCssGzipSize,
                       true,
                       AssetCacheMode::Immutable);
}

void dashboard_handle_app() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    send_progmem_asset(*context->server,
                       "application/javascript; charset=utf-8",
                       WebTemplates::kDashboardAppJsGzip,
                       WebTemplates::kDashboardAppJsGzipSize,
                       true,
                       AssetCacheMode::Immutable);
}

void wifi_handle_save() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->storage) {
        return;
    }
    WebServer &server = *context->server;
    if (!context->wifi_is_ap_mode || !context->wifi_is_ap_mode()) {
        send_no_store_text(server, 409, "WiFi save allowed only in AP setup mode");
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_no_store_text(server, 503, "OTA in progress");
        return;
    }
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    if (ssid.isEmpty()) {
        send_no_store_text(server, 400, "SSID required");
        return;
    }
    if (!WebInputValidation::isWifiSsidValid(ssid, WebInputValidation::kWifiSsidMaxBytes)) {
        send_no_store_text(server, 400, "SSID must be 1-32 bytes");
        return;
    }
    if (WebInputValidation::hasControlChars(pass)) {
        send_no_store_text(server, 400, "Password contains unsupported control characters");
        return;
    }

    if (!context->storage->saveWiFiSettings(ssid, pass, true)) {
        send_no_store_text(server, 500, "Failed to persist WiFi settings");
        return;
    }
    if (context->wifi_ssid) *context->wifi_ssid = ssid;
    if (context->wifi_pass) *context->wifi_pass = pass;
    if (context->wifi_enabled) *context->wifi_enabled = true;
    if (context->wifi_enabled_dirty) *context->wifi_enabled_dirty = false;
    if (context->wifi_ui_dirty) *context->wifi_ui_dirty = true;

    String html = FPSTR(WebTemplates::kWifiSavePage);
    String hostname = "aura";
    if (context->hostname && !context->hostname->isEmpty()) {
        hostname = *context->hostname;
    }
    String hostname_url = "http://";
    hostname_url += hostname;
    hostname_url += ".local/dashboard";
    html.replace("{{HOSTNAME_DASHBOARD_URL}}", html_escape(hostname_url));
    html.replace("{{WAIT_SECONDS}}", "15");
    send_html_stream(server, html);
    g_deferred_wifi_start_sta = true;
    g_deferred_wifi_start_sta_due_ms = millis() + kDeferredActionDelayMs;
}

void wifi_handle_not_found() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (context->wifi_is_ap_mode && context->wifi_is_ap_mode()) {
        IPAddress ap_ip = WiFi.softAPIP();
        String portal_url = "http://";
        if (ap_ip[0] != 0 || ap_ip[1] != 0 || ap_ip[2] != 0 || ap_ip[3] != 0) {
            portal_url += ap_ip.toString();
        } else {
            portal_url += "192.168.4.1";
        }
        portal_url += "/";

        context->server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        context->server->sendHeader("Pragma", "no-cache");
        context->server->sendHeader("Location", portal_url, true);
        context->server->send(302, "text/plain", "Redirecting to captive portal");
        return;
    }

    context->server->send(404, "text/plain", "Not found");
}

void diag_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (!diag_access_allowed(context)) {
        context->server->send(404, "text/plain", "Not found");
        return;
    }
    send_no_store_headers(*context->server);
    context->server->send_P(200, "text/html", WebTemplates::kDiagPageTemplate);
}

void diag_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (!diag_access_allowed(context)) {
        context->server->send(404, "text/plain", "Not found");
        return;
    }

    const bool ap_mode = context->wifi_is_ap_mode && context->wifi_is_ap_mode();
    const bool sta_connected = context->wifi_is_connected && context->wifi_is_connected();
    const bool wifi_enabled = context->wifi_enabled ? *context->wifi_enabled : false;

    ArduinoJson::JsonDocument doc;
    doc["success"] = true;
    doc["uptime_s"] = millis() / 1000UL;
    doc["ota_busy"] = WebHandlersIsOtaBusy();

    ArduinoJson::JsonObject heap = doc["heap"].to<ArduinoJson::JsonObject>();
    heap["free"] = ESP.getFreeHeap();
    heap["min_free"] = ESP.getMinFreeHeap();

    ArduinoJson::JsonObject network = doc["network"].to<ArduinoJson::JsonObject>();
    network["wifi_enabled"] = wifi_enabled;
    network["mode"] = ap_mode ? "ap" : (sta_connected ? "sta" : "off");
    network["sta_status"] = static_cast<int>(WiFi.status());
    network["scan_in_progress"] = context->wifi_scan_in_progress && *context->wifi_scan_in_progress;

    String wifi_ssid;
    if (ap_mode) {
        wifi_ssid = WiFi.softAPSSID();
    } else if (sta_connected) {
        wifi_ssid = WiFi.SSID();
    } else if (context->wifi_ssid) {
        wifi_ssid = *context->wifi_ssid;
    }
    network["wifi_ssid"] = wifi_ssid;

    network["ip"] = ap_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    if (context->hostname && !context->hostname->isEmpty()) {
        network["hostname"] = *context->hostname;
    } else {
        network["hostname"] = "aura";
    }
    const int rssi = sta_connected ? WiFi.RSSI() : 0;
    if (sta_connected && rssi < 0) {
        network["rssi"] = rssi;
    } else {
        network["rssi"] = nullptr;
    }

    size_t event_count = Logger::copyRecent(g_events_snapshot, kEventsApiMaxEntries);
    ArduinoJson::JsonArray last_errors = doc["last_errors"].to<ArduinoJson::JsonArray>();
    size_t added = 0;
    for (size_t i = 0; i < event_count && added < kDiagMaxErrorItems; ++i) {
        const Logger::RecentEntry &entry = g_events_snapshot[i];
        if (entry.level != Logger::Error && entry.level != Logger::Warn) {
            continue;
        }
        ArduinoJson::JsonObject item = last_errors.add<ArduinoJson::JsonObject>();
        item["ts_ms"] = entry.ms;
        item["level"] = event_level_text(entry.level);
        item["tag"] = entry.tag[0] ? entry.tag : "SYSTEM";
        item["message"] = entry.message[0] ? entry.message : "";
        added++;
    }
    doc["error_count"] = added;

    ArduinoJson::JsonObject web_stream = doc["web_stream"].to<ArduinoJson::JsonObject>();
    web_stream["ok_count"] = g_web_stream_stats.ok_count;
    web_stream["abort_count"] = g_web_stream_stats.abort_count;
    web_stream["slow_count"] = g_web_stream_stats.slow_count;
    web_stream["last_abort_reason"] = stream_abort_reason_text(g_web_stream_stats.last_abort_reason);
    web_stream["last_errno"] = g_web_stream_stats.last_errno;
    web_stream["last_sent"] = static_cast<uint32_t>(g_web_stream_stats.last_sent);
    web_stream["last_total"] = static_cast<uint32_t>(g_web_stream_stats.last_total);
    if (g_web_stream_stats.last_total > 0) {
        web_stream["last_sent_ratio"] =
            static_cast<float>(g_web_stream_stats.last_sent) /
            static_cast<float>(g_web_stream_stats.last_total);
    } else {
        web_stream["last_sent_ratio"] = 1.0f;
    }
    web_stream["last_max_write_ms"] = g_web_stream_stats.last_max_write_ms;
    web_stream["last_uri"] = g_web_stream_stats.last_uri;

    String json;
    serializeJson(doc, json);
    send_no_store_headers(*context->server);
    context->server->send(200, "application/json", json);
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
        send_html_stream(*context->server, html);
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

    send_html_stream(server, html);
}

void mqtt_handle_save() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->storage || !context->mqtt_client) {
        return;
    }
    WebServer &server = *context->server;
    if (!context->wifi_is_connected || !context->wifi_is_connected()) {
        send_no_store_text(server, 404, "Not found");
        return;
    }
    if (!context->mqtt_ui_open || !*context->mqtt_ui_open) {
        send_no_store_text(server, 409, "Open MQTT screen to enable");
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_no_store_text(server, 503, "OTA in progress");
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
    name.trim();
    topic.trim();

    if (host.isEmpty()) {
        send_no_store_text(server, 400, "Broker address required");
        return;
    }

    if (name.isEmpty()) {
        send_no_store_text(server, 400, "Device name required");
        return;
    }

    if (topic.isEmpty()) {
        send_no_store_text(server, 400, "Base topic required");
        return;
    }

    if (has_control_chars(host) || has_control_chars(user) ||
        has_control_chars(pass) || has_control_chars(name) ||
        has_control_chars(topic)) {
        send_no_store_text(server, 400, "Fields contain unsupported control characters");
        return;
    }

    if (mqtt_topic_has_wildcards(topic)) {
        send_no_store_text(server, 400, "Base topic must not include MQTT wildcards (+ or #)");
        return;
    }

    uint16_t port = Config::MQTT_DEFAULT_PORT;
    if (!WebInputValidation::parsePortOrDefault(port_str, Config::MQTT_DEFAULT_PORT, port)) {
        send_no_store_text(server, 400, "Port must be in range 1-65535");
        return;
    }

    if (!anonymous && (user.isEmpty() || pass.isEmpty())) {
        send_no_store_text(server, 400,
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

    if (!context->storage->saveMqttSettings(host, port, user, pass, topic, name, discovery, anonymous)) {
        send_no_store_text(server, 500, "Failed to persist MQTT settings");
        return;
    }

    if (context->mqtt_host) *context->mqtt_host = host;
    if (context->mqtt_port) *context->mqtt_port = port;
    if (context->mqtt_user) *context->mqtt_user = user;
    if (context->mqtt_pass) *context->mqtt_pass = pass;
    if (context->mqtt_device_name) *context->mqtt_device_name = name;
    if (context->mqtt_base_topic) *context->mqtt_base_topic = topic;
    if (context->mqtt_discovery) *context->mqtt_discovery = discovery;
    if (context->mqtt_anonymous) *context->mqtt_anonymous = anonymous;

    String html = FPSTR(WebTemplates::kMqttSavePage);
    send_html_stream(server, html);
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
        send_html_stream(*context->server, html);
        return;
    }
    if (!context->theme_manager->isCustomScreenOpen()) {
        String html = FPSTR(WebTemplates::kThemeLockedPage);
        send_html_stream(*context->server, html);
        return;
    }
    send_html_stream_progmem(
        *context->server,
        WebTemplates::kThemeShellHtmlGzip,
        WebTemplates::kThemeShellHtmlGzipSize,
        true
    );
}

void theme_handle_styles() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    send_progmem_asset(*context->server,
                       "text/css; charset=utf-8",
                       WebTemplates::kThemeStylesCssGzip,
                       WebTemplates::kThemeStylesCssGzipSize,
                       true,
                       AssetCacheMode::Immutable);
}

void theme_handle_app() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    send_progmem_asset(*context->server,
                       "application/javascript; charset=utf-8",
                       WebTemplates::kThemeAppJsGzip,
                       WebTemplates::kThemeAppJsGzipSize,
                       true,
                       AssetCacheMode::Immutable);
}

void theme_handle_state() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->theme_manager) {
        return;
    }

    WebServer &server = *context->server;
    auto send_theme_error = [&server](int status_code, const char *message) {
        ArduinoJson::JsonDocument doc;
        doc["success"] = false;
        doc["error"] = message;
        String json;
        serializeJson(doc, json);
        send_no_store_headers(server);
        server.send(status_code, "application/json", json);
    };

    bool wifi_ready = context->wifi_is_connected && context->wifi_is_connected();
    if (!wifi_ready) {
        send_theme_error(403, "WiFi required");
        return;
    }
    if (!context->theme_ui_open || !*context->theme_ui_open) {
        send_theme_error(409, "Open Theme screen to enable");
        return;
    }
    if (!context->theme_manager->isCustomScreenOpen()) {
        send_theme_error(409, "Open Custom Theme screen to enable");
        return;
    }

    ThemeColors colors = context->theme_manager->previewOrCurrent();
    ArduinoJson::JsonDocument doc;
    doc["success"] = true;
    doc["bg_color"] = theme_color_to_hex(colors.screen_bg);
    doc["card_top"] = theme_color_to_hex(colors.card_bg);
    doc["card_bottom"] = theme_color_to_hex(colors.gradient_color);
    doc["card_gradient"] = colors.gradient_enabled;
    doc["card_border"] = theme_color_to_hex(colors.card_border);
    doc["shadow_color"] = theme_color_to_hex(colors.shadow_color);
    doc["text_color"] = theme_color_to_hex(colors.text_primary);

    String json;
    serializeJson(doc, json);
    send_no_store_headers(server);
    server.send(200, "application/json", json);
}

void theme_handle_apply() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->theme_manager) {
        return;
    }
    WebServer &server = *context->server;
    auto send_theme_apply_error = [&server](int status_code, const char *message) {
        send_no_store_text(server, status_code, message);
    };
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(server);
        return;
    }
    bool wifi_ready = context->wifi_is_connected && context->wifi_is_connected();
    if (!wifi_ready) {
        send_theme_apply_error(403, "WiFi required");
        return;
    }
    if (!context->theme_ui_open || !*context->theme_ui_open) {
        send_theme_apply_error(409, "Open Theme screen to enable");
        return;
    }
    if (!context->theme_manager->isCustomScreenOpen()) {
        send_theme_apply_error(409, "Open Custom Theme screen to enable");
        return;
    }
    String body = server.arg("plain");
    if (body.isEmpty()) {
        send_theme_apply_error(400, "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        send_theme_apply_error(400, "Invalid JSON");
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
        send_theme_apply_error(503, "LVGL unavailable");
        return;
    }
    context->theme_manager->applyPreviewCustom(colors);
    if (!lvgl_port_unlock()) {
        send_theme_apply_error(500, "LVGL unlock failed");
        return;
    }

    send_no_store_headers(server);
    server.send(200, "text/plain", "OK");
}

void dac_handle_root() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control) {
        return;
    }
    send_html_stream_progmem(
        *context->server,
        WebTemplates::kDacShellHtmlGzip,
        WebTemplates::kDacShellHtmlGzipSize,
        true
    );
}

void dac_handle_styles() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    send_progmem_asset(*context->server,
                       "text/css; charset=utf-8",
                       WebTemplates::kDacStylesCssGzip,
                       WebTemplates::kDacStylesCssGzipSize,
                       true,
                       AssetCacheMode::Immutable);
}

void dac_handle_app() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    send_progmem_asset(*context->server,
                       "application/javascript; charset=utf-8",
                       WebTemplates::kDacAppJsGzip,
                       WebTemplates::kDacAppJsGzipSize,
                       true,
                       AssetCacheMode::Immutable);
}

void dac_handle_state() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control || !context->sensor_data) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
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
    sensors["hcho"] = data.hcho;
    sensors["hcho_valid"] = data.hcho_valid;
    sensors["voc_index"] = data.voc_index;
    sensors["voc_valid"] = data.voc_valid;
    sensors["nox_index"] = data.nox_index;
    sensors["nox_valid"] = data.nox_valid;

    String json;
    serializeJson(doc, json);
    send_no_store_headers(*context->server);
    context->server->send(200, "application/json", json);
}

void dac_handle_action() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control || !context->storage) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
        return;
    }
    WebServer &server = *context->server;
    FanControl &fan = *context->fan_control;
    auto send_dac_action_error = [&server](int status_code, const char *message) {
        send_no_store_text(server, status_code, message);
    };

    String body = server.arg("plain");
    if (body.isEmpty()) {
        send_dac_action_error(400, "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        send_dac_action_error(400, "Invalid JSON");
        return;
    }

    const String action = String(doc["action"] | "");
    if (action == "set_mode") {
        const String mode = String(doc["mode"] | "");
        bool auto_mode = false;
        if (mode == "manual") {
            auto_mode = false;
        } else if (mode == "auto") {
            auto_mode = true;
        } else {
            send_dac_action_error(400, "Invalid mode");
            return;
        }
        if (!persist_dac_auto_mode(*context->storage, auto_mode)) {
            send_dac_action_error(500, "Failed to persist DAC mode");
            return;
        }
        fan.setMode(auto_mode ? FanControl::Mode::Auto : FanControl::Mode::Manual);
    } else if (action == "set_manual_step") {
        fan.setManualStep(doc["step"] | 1);
    } else if (action == "set_timer") {
        uint32_t timer_seconds = 0;
        if (!parse_dac_timer_seconds(doc["seconds"], timer_seconds)) {
            send_dac_action_error(400, "Invalid timer value");
            return;
        }
        fan.setTimerSeconds(timer_seconds);
    } else if (action == "start") {
        fan.requestStart();
    } else if (action == "stop") {
        fan.requestStop();
    } else if (action == "start_auto") {
        if (!persist_dac_auto_mode(*context->storage, true)) {
            send_dac_action_error(500, "Failed to persist DAC auto mode");
            return;
        }
        fan.requestAutoStart();
    } else {
        send_dac_action_error(400, "Unsupported action");
        return;
    }

    send_no_store_headers(server);
    server.send(200, "application/json", "{\"success\":true}");
}

void dac_handle_auto() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->fan_control || !context->storage) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
        return;
    }
    WebServer &server = *context->server;
    FanControl &fan = *context->fan_control;
    auto send_dac_auto_error = [&server](int status_code, const char *message) {
        send_no_store_text(server, status_code, message);
    };

    String body = server.arg("plain");
    if (body.isEmpty()) {
        send_dac_auto_error(400, "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        send_dac_auto_error(400, "Invalid JSON");
        return;
    }

    DacAutoConfig config = fan.autoConfig();
    ArduinoJson::JsonObjectConst root = doc.as<ArduinoJson::JsonObjectConst>();
    ArduinoJson::JsonObjectConst source = root;
    if (root["auto"].is<ArduinoJson::JsonObjectConst>()) {
        source = root["auto"].as<ArduinoJson::JsonObjectConst>();
    }
    if (!DacAutoConfigJson::readJson(source, config)) {
        send_dac_auto_error(400, "Invalid auto payload");
        return;
    }

    const bool rearm = root["rearm"] | false;
    String serialized = DacAutoConfigJson::serialize(config);
    if (!context->storage->saveTextAtomic(StorageManager::kDacAutoPath, serialized)) {
        send_dac_auto_error(500, "Failed to persist auto config");
        return;
    }
    if (rearm && !persist_dac_auto_mode(*context->storage, true)) {
        send_dac_auto_error(500, "Failed to persist DAC auto mode");
        return;
    }

    fan.setAutoConfig(config);
    if (rearm) {
        fan.requestAutoStart();
    }
    send_no_store_headers(server);
    server.send(200, "application/json", "{\"success\":true}");
}

void charts_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->charts_history) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
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
    send_no_store_headers(server);
    server.send(200, "application/json", json);
}

void state_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server || !context->sensor_data) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
        return;
    }

    const SensorData &data = *context->sensor_data;
    WebServer &server = *context->server;
    const uint32_t uptime_s = millis() / 1000UL;
    const time_t now_epoch = time(nullptr);

    ArduinoJson::JsonDocument doc;
    doc["success"] = true;
    doc["ota_busy"] = false;
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
    send_no_store_headers(server);
    server.send(200, "application/json", json);
}

void settings_handle_update() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
        return;
    }

    WebServer &server = *context->server;
    auto send_settings_error = [&server](int status_code, const char *message) {
        send_no_store_text(server, status_code, message);
    };
    if (!context->ui_controller) {
        send_settings_error(503, "UI controller unavailable");
        return;
    }
    String body = server.arg("plain");
    if (body.isEmpty()) {
        send_settings_error(400, "Missing body");
        return;
    }

    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, body);
    if (err) {
        send_settings_error(400, "Invalid JSON");
        return;
    }

    UiController &ui = *context->ui_controller;

    // Stage all inputs first to avoid partial apply on late validation errors.
    bool has_night_mode = false;
    bool requested_night_mode = false;
    ArduinoJson::JsonVariantConst night_mode_var = doc["night_mode"];
    if (!night_mode_var.isNull()) {
        if (!night_mode_var.is<bool>()) {
            send_settings_error(400, "night_mode must be bool");
            return;
        }
        has_night_mode = true;
        requested_night_mode = night_mode_var.as<bool>();
        if (ui.webNightModeLocked()) {
            send_settings_error(409, "night_mode is locked by auto mode");
            return;
        }
    }

    bool has_backlight = false;
    bool requested_backlight = false;
    ArduinoJson::JsonVariantConst backlight_var = doc["backlight_on"];
    if (!backlight_var.isNull()) {
        if (!backlight_var.is<bool>()) {
            send_settings_error(400, "backlight_on must be bool");
            return;
        }
        has_backlight = true;
        requested_backlight = backlight_var.as<bool>();
    }

    bool has_units_c = false;
    bool requested_units_c = false;
    ArduinoJson::JsonVariantConst units_c_var = doc["units_c"];
    if (!units_c_var.isNull()) {
        if (!units_c_var.is<bool>()) {
            send_settings_error(400, "units_c must be bool");
            return;
        }
        has_units_c = true;
        requested_units_c = units_c_var.as<bool>();
    }

    const ArduinoJson::JsonVariantConst temp_offset_var = doc["temp_offset"];
    const ArduinoJson::JsonVariantConst hum_offset_var = doc["hum_offset"];
    const bool has_temp_offset = !temp_offset_var.isNull();
    const bool has_hum_offset = !hum_offset_var.isNull();
    const bool has_offsets = has_temp_offset || has_hum_offset;
    float requested_temp_offset = ui.webTempOffset();
    float requested_hum_offset = ui.webHumOffset();
    if (has_temp_offset) {
        if (!temp_offset_var.is<float>() && !temp_offset_var.is<int>()) {
            send_settings_error(400, "temp_offset must be number");
            return;
        }
        requested_temp_offset = temp_offset_var.as<float>();
    }
    if (has_hum_offset) {
        if (!hum_offset_var.is<float>() && !hum_offset_var.is<int>()) {
            send_settings_error(400, "hum_offset must be number");
            return;
        }
        requested_hum_offset = hum_offset_var.as<float>();
    }

    bool has_display_name = false;
    String requested_display_name;
    ArduinoJson::JsonVariantConst display_name_var = doc["display_name"];
    if (!display_name_var.isNull()) {
        if (!display_name_var.is<const char *>()) {
            send_settings_error(400, "display_name must be string");
            return;
        }
        if (!context->storage) {
            send_settings_error(503, "Storage unavailable");
            return;
        }

        requested_display_name = display_name_var.as<String>();
        requested_display_name.trim();
        if (requested_display_name.length() > kWebDisplayNameMaxLen) {
            send_settings_error(400, "display_name is too long");
            return;
        }
        if (has_control_chars(requested_display_name)) {
            send_settings_error(400, "display_name contains invalid characters");
            return;
        }
        has_display_name = true;
    }

    bool restart_requested = false;
    ArduinoJson::JsonVariantConst restart_var = doc["restart"];
    if (!restart_var.isNull()) {
        if (!restart_var.is<bool>()) {
            send_settings_error(400, "restart must be bool");
            return;
        }
        restart_requested = restart_var.as<bool>();
    }

    const bool previous_backlight = ui.webBacklightOn();
    const bool previous_night_mode = ui.webNightModeEnabled();
    const bool previous_units_c = ui.webUnitsC();
    const float previous_temp_offset = ui.webTempOffset();
    const float previous_hum_offset = ui.webHumOffset();
    const String previous_display_name =
        (has_display_name && context->storage) ? context->storage->config().web_display_name : String();

    bool applied_backlight = false;
    bool applied_night_mode = false;
    bool applied_units = false;
    bool applied_offsets = false;
    bool applied_display_name = false;

    auto rollback = [&]() -> bool {
        bool rollback_failed = false;

        if (applied_backlight && ui.webBacklightOn() != previous_backlight) {
            if (!ui.webSetBacklight(previous_backlight)) {
                rollback_failed = true;
            }
        }
        if (applied_night_mode && ui.webNightModeEnabled() != previous_night_mode) {
            if (!ui.webSetNightMode(previous_night_mode)) {
                rollback_failed = true;
            }
        }
        if (applied_units && ui.webUnitsC() != previous_units_c) {
            if (!ui.webSetUnitsC(previous_units_c)) {
                rollback_failed = true;
            }
        }
        if (applied_offsets) {
            const bool temp_changed = fabsf(ui.webTempOffset() - previous_temp_offset) > 0.0001f;
            const bool hum_changed = fabsf(ui.webHumOffset() - previous_hum_offset) > 0.0001f;
            if ((temp_changed || hum_changed) &&
                !ui.webSetOffsets(previous_temp_offset, previous_hum_offset)) {
                rollback_failed = true;
            }
        }
        if (applied_display_name && context->storage) {
            context->storage->config().web_display_name = previous_display_name;
            if (!context->storage->saveConfig(true)) {
                context->storage->requestSave();
                rollback_failed = true;
            }
        }

        return !rollback_failed;
    };

    auto respond_apply_failure = [&](int status_code, const char *message) {
        if (!rollback()) {
            LOGE("Web", "/api/settings rollback failed");
            send_settings_error(500, "Failed to apply settings atomically");
            return;
        }
        send_settings_error(status_code, message);
    };

    // Apply only after full validation; on any failure, rollback previous changes.
    if (has_units_c) {
        if (!ui.webSetUnitsC(requested_units_c)) {
            respond_apply_failure(500, "Failed to persist units setting");
            return;
        }
        applied_units = true;
    }
    if (has_offsets) {
        if (!ui.webSetOffsets(requested_temp_offset, requested_hum_offset)) {
            respond_apply_failure(500, "Failed to persist offsets");
            return;
        }
        applied_offsets = true;
    }
    if (has_display_name && context->storage) {
        context->storage->config().web_display_name = requested_display_name;
        if (!context->storage->saveConfig(true)) {
            context->storage->config().web_display_name = previous_display_name;
            context->storage->requestSave();
            respond_apply_failure(500, "Failed to persist display_name");
            return;
        }
        applied_display_name = true;
    }
    if (has_night_mode) {
        if (!ui.webSetNightMode(requested_night_mode)) {
            respond_apply_failure(409, "night_mode is locked by auto mode");
            return;
        }
        applied_night_mode = true;
    }
    if (has_backlight) {
        if (!ui.webSetBacklight(requested_backlight)) {
            respond_apply_failure(409, "backlight state could not be applied");
            return;
        }
        applied_backlight = true;
    }

    ArduinoJson::JsonDocument response_doc;
    response_doc["success"] = true;
    response_doc["restart"] = restart_requested;
    ArduinoJson::JsonObject response_settings = response_doc["settings"].to<ArduinoJson::JsonObject>();
    fill_web_settings_json(response_settings, *context);

    String json;
    serializeJson(response_doc, json);
    send_no_store_headers(server);
    server.send(200, "application/json", json);

    if (restart_requested) {
        g_restart_controller.schedule(millis(), kDeferredActionDelayMs);
        LOGI("Web", "settings restart requested, deferred reboot in %u ms",
             static_cast<unsigned>(kDeferredActionDelayMs));
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
        if (WebHandlersIsOtaBusy()) {
            LOGW("OTA", "reject upload start while OTA is busy");
            return;
        }
        ota_reset_state();
        g_ota_upload_seen = true;
        g_ota_upload_active = true;
        g_ota_upload_start_ms = millis();
        ota_disable_wifi_power_save_for_upload();
        if (WiFi.status() == WL_CONNECTED) {
            g_ota_start_rssi_valid = true;
            g_ota_start_rssi = WiFi.RSSI();
        }
        ota_extend_task_wdt();
        size_t expected_size = 0;
        const bool size_known = parse_size_arg(server.arg("ota_size"), expected_size);
        const uint32_t client_timeout_ms = ota_upload_timeout_ms(size_known ? expected_size : 0);
        server.client().setTimeout(client_timeout_ms);
        if (context->wifi_stop_scan) {
            context->wifi_stop_scan();
        }
        ota_set_ui_screen(true);

        const esp_partition_t *target_partition = esp_ota_get_next_update_partition(nullptr);
        if (!target_partition) {
            ota_set_error("OTA partition unavailable");
            LOGE("OTA", "no target partition");
            return;
        }
        g_ota_slot_size = target_partition->size;
        g_ota_size_known = size_known;
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

        if (g_ota_start_rssi_valid) {
            LOGI("OTA", "upload started (slot=%u, expected=%u, known=%s, timeout=%u ms, rssi=%d dBm)",
                 static_cast<unsigned>(g_ota_slot_size),
                 static_cast<unsigned>(g_ota_expected_size),
                 g_ota_size_known ? "YES" : "NO",
                 static_cast<unsigned>(client_timeout_ms),
                 g_ota_start_rssi);
        } else {
            LOGI("OTA", "upload started (slot=%u, expected=%u, known=%s, timeout=%u ms, rssi=n/a)",
                 static_cast<unsigned>(g_ota_slot_size),
                 static_cast<unsigned>(g_ota_expected_size),
                 g_ota_size_known ? "YES" : "NO",
                 static_cast<unsigned>(client_timeout_ms));
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (!g_ota_upload_active || !g_ota_error.isEmpty()) {
            return;
        }
        if (upload.currentSize == 0) {
            return;
        }
        if (!g_ota_first_chunk_seen) {
            g_ota_first_chunk_seen = true;
            g_ota_first_chunk_ms = millis();
            LOGI("OTA", "first chunk received after %u ms (size=%u bytes)",
                 static_cast<unsigned>(g_ota_first_chunk_ms - g_ota_upload_start_ms),
                 static_cast<unsigned>(upload.currentSize));
        }
        g_ota_last_chunk_ms = millis();
        if (g_ota_chunk_count == 0 || upload.currentSize < g_ota_chunk_min_size) {
            g_ota_chunk_min_size = upload.currentSize;
        }
        if (upload.currentSize > g_ota_chunk_max_size) {
            g_ota_chunk_max_size = upload.currentSize;
        }
        g_ota_chunk_sum_size += upload.currentSize;
        g_ota_chunk_count++;
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
        const uint32_t finalize_start_ms = millis();
        if (!Update.end(true)) {
            g_ota_finalize_ms = millis() - finalize_start_ms;
            ota_set_error(ota_error_prefixed("Update finalize failed"));
            LOGE("OTA", "%s", g_ota_error.c_str());
            return;
        }
        g_ota_finalize_ms = millis() - finalize_start_ms;
        g_ota_upload_success = true;
        g_ota_upload_active = false;
        const uint32_t total_ms = millis() - g_ota_upload_start_ms;
        const uint32_t first_chunk_delay_ms =
            g_ota_first_chunk_seen ? (g_ota_first_chunk_ms - g_ota_upload_start_ms) : 0;
        const size_t avg_chunk_size =
            g_ota_chunk_count > 0 ? (g_ota_chunk_sum_size / g_ota_chunk_count) : 0;
        LOGI("OTA",
             "upload complete, written=%u bytes, total=%u ms, first_chunk_seen=%s, first_chunk=%u ms, finalize=%u ms, chunks=%u, chunk[min/avg/max]=%u/%u/%u bytes",
             static_cast<unsigned>(g_ota_written_size),
             static_cast<unsigned>(total_ms),
             g_ota_first_chunk_seen ? "YES" : "NO",
             static_cast<unsigned>(first_chunk_delay_ms),
             static_cast<unsigned>(g_ota_finalize_ms),
             static_cast<unsigned>(g_ota_chunk_count),
             static_cast<unsigned>(g_ota_chunk_min_size),
             static_cast<unsigned>(avg_chunk_size),
             static_cast<unsigned>(g_ota_chunk_max_size));
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
    if (WebHandlersIsOtaBusy() && !g_ota_upload_success) {
        send_ota_busy_json(server);
        return;
    }
    const bool has_upload = g_ota_upload_seen;
    const bool success = has_upload && g_ota_upload_success && g_ota_error.isEmpty();
    const size_t written_size = g_ota_written_size;
    const size_t slot_size = g_ota_slot_size;
    const bool size_known = g_ota_size_known;
    const size_t expected_size = g_ota_expected_size;
    const uint32_t upload_start_ms = g_ota_upload_start_ms;
    const uint32_t first_chunk_ms = g_ota_first_chunk_ms;
    const uint32_t last_chunk_ms = g_ota_last_chunk_ms;
    const bool first_chunk_seen = g_ota_first_chunk_seen;
    const uint32_t finalize_ms = g_ota_finalize_ms;
    const uint32_t chunk_count = g_ota_chunk_count;
    const size_t chunk_min = g_ota_chunk_min_size;
    const size_t chunk_max = g_ota_chunk_max_size;
    const size_t chunk_sum = g_ota_chunk_sum_size;
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
    send_no_store_headers(server);
    server.send(status_code, "application/json", json);

    if (has_upload) {
        const uint32_t total_ms = upload_start_ms == 0 ? 0 : (millis() - upload_start_ms);
        const uint32_t first_chunk_delay_ms =
            first_chunk_seen ? (first_chunk_ms - upload_start_ms) : 0;
        const uint32_t transfer_phase_ms =
            first_chunk_seen && last_chunk_ms >= first_chunk_ms ? (last_chunk_ms - first_chunk_ms) : 0;
        const size_t avg_chunk = chunk_count > 0 ? (chunk_sum / chunk_count) : 0;
        LOGI("OTA",
             "summary success=%s written=%u slot=%u expected=%u known=%s total=%u ms first_chunk_seen=%s first_chunk=%u ms transfer=%u ms finalize=%u ms chunks=%u chunk[min/avg/max]=%u/%u/%u bytes",
             success ? "YES" : "NO",
             static_cast<unsigned>(written_size),
             static_cast<unsigned>(slot_size),
             static_cast<unsigned>(expected_size),
             size_known ? "YES" : "NO",
             static_cast<unsigned>(total_ms),
             first_chunk_seen ? "YES" : "NO",
             static_cast<unsigned>(first_chunk_delay_ms),
             static_cast<unsigned>(transfer_phase_ms),
             static_cast<unsigned>(finalize_ms),
             static_cast<unsigned>(chunk_count),
             static_cast<unsigned>(chunk_min),
             static_cast<unsigned>(avg_chunk),
             static_cast<unsigned>(chunk_max));
    }

    if (success) {
        LOGI("OTA", "response sent, deferred reboot in %u ms",
             static_cast<unsigned>(kDeferredRestartDelayMs));
        g_restart_controller.schedule(millis(), kDeferredRestartDelayMs);
    } else {
        ota_set_ui_screen(false);
    }
    ota_restore_wifi_power_save();
    ota_restore_task_wdt();
    ota_reset_state();
}

void events_handle_data() {
    WebHandlerContext *context = ctx();
    if (!context || !context->server) {
        return;
    }
    if (WebHandlersIsOtaBusy()) {
        send_ota_busy_json(*context->server);
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
        if (!should_emit_web_event(entry)) {
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
    send_no_store_headers(server);
    server.send(200, "application/json", json);
}
