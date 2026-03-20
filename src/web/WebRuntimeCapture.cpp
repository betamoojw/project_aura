// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebRuntimeCapture.h"

#include <WiFi.h>

#include "modules/MqttRuntime.h"

namespace WebRuntimeCapture {

WebNetworkUtils::Snapshot captureNetworkSnapshot(const WebHandlerContext &context) {
    WebNetworkUtils::Snapshot snapshot{};
    snapshot.wifi_enabled = context.wifi_enabled ? *context.wifi_enabled : false;
    snapshot.ap_mode = context.wifi_is_ap_mode && context.wifi_is_ap_mode();
    snapshot.sta_connected = context.wifi_is_connected && context.wifi_is_connected();
    snapshot.scan_in_progress = context.wifi_scan_in_progress && *context.wifi_scan_in_progress;
    snapshot.sta_status = static_cast<int>(WiFi.status());

    if (snapshot.ap_mode) {
        snapshot.wifi_ssid = WiFi.softAPSSID();
    } else if (snapshot.sta_connected) {
        snapshot.wifi_ssid = WiFi.SSID();
    } else if (context.wifi_ssid) {
        snapshot.wifi_ssid = *context.wifi_ssid;
    }

    snapshot.ip = snapshot.ap_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();

    if (context.hostname) {
        snapshot.has_hostname = true;
        snapshot.hostname = *context.hostname;
    }

    const int rssi = snapshot.sta_connected ? WiFi.RSSI() : 0;
    if (snapshot.sta_connected && rssi < 0) {
        snapshot.has_rssi = true;
        snapshot.rssi = rssi;
    }

    if (context.mqtt_host) {
        snapshot.has_mqtt_broker = true;
        snapshot.mqtt_broker = *context.mqtt_host;
    }
    snapshot.mqtt_enabled = context.mqtt_user_enabled ? *context.mqtt_user_enabled : false;
    snapshot.mqtt_connected = context.mqtt_runtime && context.mqtt_runtime->isConnected();

    return snapshot;
}

} // namespace WebRuntimeCapture
