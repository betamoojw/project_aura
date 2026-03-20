// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/ConnectivityRuntime.h"

#include <WiFi.h>

#include "modules/MqttManager.h"
#include "modules/NetworkManager.h"

namespace {

bool ip_has_value(const IPAddress &ip) {
    return ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0;
}

} // namespace

ConnectivityRuntime::ConnectivityRuntime() {
    mutex_ = xSemaphoreCreateMutexStatic(&mutex_buffer_);
}

void ConnectivityRuntime::update(AuraNetworkManager &networkManager, MqttManager &mqttManager) {
    lock();
    snapshot_.wifi_enabled = networkManager.isEnabled();
    snapshot_.wifi_connected = networkManager.isConnected();
    snapshot_.wifi_state = static_cast<int>(networkManager.state());
    snapshot_.wifi_retry_count = networkManager.retryCount();
    snapshot_.wifi_ssid = networkManager.ssid();
    snapshot_.ap_ssid = networkManager.apSsid();
    if (snapshot_.ap_ssid.isEmpty()) {
        snapshot_.ap_ssid = Config::WIFI_AP_SSID;
    }

    const bool sta_mode =
        snapshot_.wifi_enabled &&
        snapshot_.wifi_state == static_cast<int>(AuraNetworkManager::WIFI_STATE_STA_CONNECTED);
    const bool ap_mode =
        snapshot_.wifi_enabled &&
        snapshot_.wifi_state == static_cast<int>(AuraNetworkManager::WIFI_STATE_AP_CONFIG);

    snapshot_.sta_ip.clear();
    if (sta_mode) {
        const IPAddress ip = WiFi.localIP();
        if (ip_has_value(ip)) {
            snapshot_.sta_ip = ip.toString();
        }
    }

    snapshot_.ap_ip.clear();
    if (ap_mode) {
        const IPAddress ip = WiFi.softAPIP();
        if (ip_has_value(ip)) {
            snapshot_.ap_ip = ip.toString();
        }
    }

    snapshot_.dashboard_local_url = networkManager.localUrl("/dashboard");
    snapshot_.dashboard_sta_url = snapshot_.dashboard_local_url;
    if (!snapshot_.sta_ip.isEmpty()) {
        snapshot_.dashboard_sta_url = "http://";
        snapshot_.dashboard_sta_url += snapshot_.sta_ip;
        snapshot_.dashboard_sta_url += "/dashboard";
    }

    snapshot_.mqtt_local_url = networkManager.localUrl("/mqtt");
    snapshot_.mqtt_sta_url = snapshot_.mqtt_local_url;
    if (!snapshot_.sta_ip.isEmpty()) {
        snapshot_.mqtt_sta_url = "http://";
        snapshot_.mqtt_sta_url += snapshot_.sta_ip;
        snapshot_.mqtt_sta_url += "/mqtt";
    }

    snapshot_.theme_local_url = networkManager.localUrl("/theme");
    snapshot_.theme_sta_url = snapshot_.theme_local_url;
    if (!snapshot_.sta_ip.isEmpty()) {
        snapshot_.theme_sta_url = "http://";
        snapshot_.theme_sta_url += snapshot_.sta_ip;
        snapshot_.theme_sta_url += "/theme";
    }

    snapshot_.dac_local_url = networkManager.localUrl("/dac");
    snapshot_.dac_sta_url = snapshot_.dac_local_url;
    if (!snapshot_.sta_ip.isEmpty()) {
        snapshot_.dac_sta_url = "http://";
        snapshot_.dac_sta_url += snapshot_.sta_ip;
        snapshot_.dac_sta_url += "/dac";
    }

    snapshot_.mqtt_user_enabled = mqttManager.isUserEnabled();
    snapshot_.mqtt_enabled = mqttManager.isEnabled();
    snapshot_.mqtt_connected = mqttManager.isConnected();
    snapshot_.mqtt_retry_stage = mqttManager.retryStage();
    snapshot_.mqtt_host = mqttManager.host();
    snapshot_.mqtt_port = mqttManager.port();
    snapshot_.mqtt_base_topic = mqttManager.baseTopic();
    unlock();
}

ConnectivityRuntimeSnapshot ConnectivityRuntime::snapshot() const {
    lock();
    ConnectivityRuntimeSnapshot copy = snapshot_;
    unlock();
    return copy;
}

void ConnectivityRuntime::lock() const {
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
    }
}

void ConnectivityRuntime::unlock() const {
    if (mutex_) {
        xSemaphoreGive(mutex_);
    }
}
