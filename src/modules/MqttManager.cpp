// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "modules/MqttManager.h"

#include <ctype.h>
#include <math.h>
#include <string.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "core/Logger.h"
#include "core/MathUtils.h"
#include "modules/StorageManager.h"
#include "modules/NetworkManager.h"

namespace {

MqttManager *g_mqtt = nullptr;

constexpr uint8_t kMqttRetryStages = 3;
constexpr uint8_t kMqttRetryStageAttempts = Config::MQTT_CONNECT_MAX_FAILS;
constexpr uint8_t kMqttRetryMaxAttempts = kMqttRetryStages * kMqttRetryStageAttempts;
constexpr size_t kTopicBufferSize = 256;
constexpr uint32_t kMqttMdnsSuccessCacheMs = 5UL * 60UL * 1000UL;
constexpr uint32_t kMqttMdnsFailureCacheMs = 60UL * 1000UL;

uint8_t retry_stage_for_attempts(uint32_t attempts) {
    return static_cast<uint8_t>(attempts / kMqttRetryStageAttempts);
}

uint32_t retry_delay_for_stage(uint8_t stage) {
    switch (stage) {
        case 0: return Config::MQTT_RETRY_MS;
        case 1: return Config::MQTT_RETRY_LONG_MS;
        default: return Config::MQTT_RETRY_HOURLY_MS;
    }
}

const char *retry_delay_label(uint32_t delay_ms) {
    if (delay_ms == Config::MQTT_RETRY_MS) {
        return "30 seconds";
    }
    if (delay_ms == Config::MQTT_RETRY_LONG_MS) {
        return "10 minutes";
    }
    return "1 hour";
}

void build_state_topic(char *out, size_t out_size, const String &base) {
    snprintf(out, out_size, "%s/state", base.c_str());
}

void build_availability_topic(char *out, size_t out_size, const String &base) {
    snprintf(out, out_size, "%s/status", base.c_str());
}

void build_night_mode_availability_topic(char *out, size_t out_size, const String &base) {
    snprintf(out, out_size, "%s/availability/night_mode", base.c_str());
}

void build_command_topic(char *out, size_t out_size, const String &base, const char *command) {
    snprintf(out, out_size, "%s/command/%s", base.c_str(), command);
}

void build_discovery_topic(char *out, size_t out_size, const char *component,
                           const String &device_id, const char *object_id) {
    snprintf(out, out_size, "homeassistant/%s/%s_%s/config",
             component, device_id.c_str(), object_id);
}

bool equals_ignore_case(const char *a, const char *b) {
    if (!a || !b) {
        return false;
    }
    while (*a && *b) {
        if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

void trim_ascii(char *text) {
    if (!text) {
        return;
    }
    char *start = text;
    while (*start && isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }
    char *end = start + strlen(start);
    while (end > start && isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    size_t len = static_cast<size_t>(end - start);
    if (start != text) {
        memmove(text, start, len);
    }
    text[len] = '\0';
}

} // namespace

MqttManager::MqttManager() : client_(net_) {}

void MqttManager::begin(StorageManager &storage, AuraNetworkManager &network) {
    storage_ = &storage;
    network_ = &network;
    g_mqtt = this;
    loadPrefs();
    initDeviceId();
    setupClient();
}

void MqttManager::loadPrefs() {
    if (!storage_) {
        return;
    }
    storage_->loadMqttSettings(mqtt_host_, mqtt_port_, mqtt_user_, mqtt_pass_, mqtt_base_topic_,
                               mqtt_device_name_, mqtt_user_enabled_, mqtt_discovery_,
                               mqtt_anonymous_);
    mqtt_enabled_ = mqtt_user_enabled_;
    if (mqtt_base_topic_.endsWith("/")) {
        mqtt_base_topic_.remove(mqtt_base_topic_.length() - 1);
    }
    if (mqtt_base_topic_.isEmpty()) {
        mqtt_base_topic_ = Config::MQTT_DEFAULT_BASE;
    }
    if (mqtt_device_name_.isEmpty()) {
        mqtt_device_name_ = Config::MQTT_DEFAULT_NAME;
    }
    if (mqtt_port_ == 0) {
        mqtt_port_ = Config::MQTT_DEFAULT_PORT;
    }
}

void MqttManager::initDeviceId() {
    uint64_t mac = ESP.getEfuseMac();
    char buf[24];
    snprintf(buf, sizeof(buf), "aura_%04X%08X", static_cast<uint16_t>(mac >> 32),
             static_cast<uint32_t>(mac & 0xFFFFFFFF));
    mqtt_device_id_ = buf;
}

void MqttManager::setupClient() {
    client_.setServer(mqtt_host_.c_str(), mqtt_port_);
    client_.setBufferSize(Config::MQTT_BUFFER_SIZE);
    client_.setKeepAlive(30);
    client_.setSocketTimeout(1);
    client_.setCallback(MqttManager::staticCallback);
}

bool MqttManager::prepareBrokerEndpoint(IPAddress &resolved_ip, bool &using_resolved_ip,
                                        bool &is_mdns_host) {
    using_resolved_ip = false;
    is_mdns_host = false;
    resolved_ip = IPAddress();

    String broker_host = mqtt_host_;
    broker_host.trim();
    if (broker_host.isEmpty()) {
        return false;
    }
    if (broker_host != mqtt_host_) {
        mqtt_host_ = broker_host;
    }

    String host_lc = broker_host;
    host_lc.toLowerCase();
    is_mdns_host = host_lc.endsWith(".local");
    if (!is_mdns_host) {
        client_.setServer(mqtt_host_.c_str(), mqtt_port_);
        return true;
    }

    const uint32_t now = millis();
    if (mqtt_mdns_cache_valid_ && broker_host.equalsIgnoreCase(mqtt_mdns_cache_host_)) {
        const uint32_t cache_age_ms = now - mqtt_mdns_cache_ts_ms_;
        const uint32_t cache_ttl_ms =
            mqtt_mdns_cache_success_ ? kMqttMdnsSuccessCacheMs : kMqttMdnsFailureCacheMs;
        if (cache_age_ms < cache_ttl_ms) {
            if (mqtt_mdns_cache_success_) {
                client_.setServer(mqtt_mdns_cache_ip_, mqtt_port_);
                resolved_ip = mqtt_mdns_cache_ip_;
                using_resolved_ip = true;
            } else {
                client_.setServer(mqtt_host_.c_str(), mqtt_port_);
            }
            return true;
        }
    }

    String mdns_name = broker_host.substring(0, broker_host.length() - 6);
    mdns_name.trim();
    if (mdns_name.isEmpty()) {
        client_.setServer(mqtt_host_.c_str(), mqtt_port_);
        return true;
    }

    IPAddress mdns_ip = MDNS.queryHost(mdns_name.c_str());
    mqtt_mdns_cache_host_ = broker_host;
    mqtt_mdns_cache_ts_ms_ = now;
    mqtt_mdns_cache_valid_ = true;
    if (static_cast<uint32_t>(mdns_ip) != 0U) {
        mqtt_mdns_cache_success_ = true;
        mqtt_mdns_cache_ip_ = mdns_ip;
        client_.setServer(mdns_ip, mqtt_port_);
        resolved_ip = mdns_ip;
        using_resolved_ip = true;
    } else {
        mqtt_mdns_cache_success_ = false;
        mqtt_mdns_cache_ip_ = IPAddress();
        client_.setServer(mqtt_host_.c_str(), mqtt_port_);
    }
    return true;
}

void MqttManager::publishDiscoverySensor(const char *object_id, const char *name,
                                         const char *unit, const char *device_class,
                                         const char *state_class, const char *value_template,
                                         const char *icon) {
    if (!client_.connected()) {
        return;
    }
    String payload;
    payload.reserve(520); // Discovery sensor payload ~450 bytes; keep headroom for long IDs.
    payload = "{";
    payload += "\"name\":\"";
    payload += name;
    payload += "\",\"unique_id\":\"";
    payload += mqtt_device_id_;
    payload += "_";
    payload += object_id;
    payload += "\",\"state_topic\":\"";
    char topic[kTopicBufferSize];
    build_state_topic(topic, sizeof(topic), mqtt_base_topic_);
    payload += topic;
    payload += "\",\"availability_topic\":\"";
    build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
    payload += topic;
    payload += "\",\"payload_available\":\"";
    payload += Config::MQTT_AVAIL_ONLINE;
    payload += "\",\"payload_not_available\":\"";
    payload += Config::MQTT_AVAIL_OFFLINE;
    payload += "\"";
    if (value_template && value_template[0] != '\0') {
        payload += ",\"value_template\":\"";
        payload += value_template;
        payload += "\"";
    }
    if (unit && unit[0] != '\0') {
        payload += ",\"unit_of_measurement\":\"";
        payload += unit;
        payload += "\"";
    }
    if (device_class && device_class[0] != '\0') {
        payload += ",\"device_class\":\"";
        payload += device_class;
        payload += "\"";
    }
    if (state_class && state_class[0] != '\0') {
        payload += ",\"state_class\":\"";
        payload += state_class;
        payload += "\"";
    }
    if (icon && icon[0] != '\0') {
        payload += ",\"icon\":\"";
        payload += icon;
        payload += "\"";
    }
    payload += ",\"device\":{\"identifiers\":[\"";
    payload += mqtt_device_id_;
    payload += "\"],\"name\":\"";
    payload += mqtt_device_name_;
    payload += "\",\"manufacturer\":\"21CNCStudio\",\"model\":\"Project Aura\"}";
    payload += "}";

    build_discovery_topic(topic, sizeof(topic), "sensor", mqtt_device_id_, object_id);
    client_.publish(topic, payload.c_str(), true);
}

void MqttManager::publishDiscoverySwitch(const char *object_id, const char *name,
                                         const char *value_template, const char *icon) {
    if (!client_.connected()) {
        return;
    }
    String payload;
    payload.reserve(640); // Switch payload includes availability array; keep headroom.
    payload = "{";
    payload += "\"name\":\"";
    payload += name;
    payload += "\",\"unique_id\":\"";
    payload += mqtt_device_id_;
    payload += "_";
    payload += object_id;
    payload += "\",\"state_topic\":\"";
    char topic[kTopicBufferSize];
    build_state_topic(topic, sizeof(topic), mqtt_base_topic_);
    payload += topic;
    payload += "\",\"command_topic\":\"";
    build_command_topic(topic, sizeof(topic), mqtt_base_topic_, object_id);
    payload += topic;
    if (strcmp(object_id, "night_mode") == 0) {
        payload += "\",\"availability\":[{\"topic\":\"";
        build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
        payload += topic;
        payload += "\",\"payload_available\":\"";
        payload += Config::MQTT_AVAIL_ONLINE;
        payload += "\",\"payload_not_available\":\"";
        payload += Config::MQTT_AVAIL_OFFLINE;
        payload += "\"},{\"topic\":\"";
        build_night_mode_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
        payload += topic;
        payload += "\",\"payload_available\":\"";
        payload += Config::MQTT_AVAIL_ONLINE;
        payload += "\",\"payload_not_available\":\"";
        payload += Config::MQTT_AVAIL_OFFLINE;
        payload += "\"}]";
        payload += ",\"availability_mode\":\"all\"";
    } else {
        payload += "\",\"availability_topic\":\"";
        build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
        payload += topic;
        payload += "\",\"payload_available\":\"";
        payload += Config::MQTT_AVAIL_ONLINE;
        payload += "\",\"payload_not_available\":\"";
        payload += Config::MQTT_AVAIL_OFFLINE;
        payload += "\"";
    }
    payload += ",\"payload_on\":\"ON\",\"payload_off\":\"OFF\"";
    payload += ",\"state_on\":\"ON\",\"state_off\":\"OFF\"";
    if (value_template && value_template[0] != '\0') {
        payload += ",\"value_template\":\"";
        payload += value_template;
        payload += "\"";
    }
    if (icon && icon[0] != '\0') {
        payload += ",\"icon\":\"";
        payload += icon;
        payload += "\"";
    }
    payload += ",\"device\":{\"identifiers\":[\"";
    payload += mqtt_device_id_;
    payload += "\"],\"name\":\"";
    payload += mqtt_device_name_;
    payload += "\",\"manufacturer\":\"21CNCStudio\",\"model\":\"Project Aura\"}";
    payload += "}";

    build_discovery_topic(topic, sizeof(topic), "switch", mqtt_device_id_, object_id);
    client_.publish(topic, payload.c_str(), true);
}

void MqttManager::publishDiscoveryButton(const char *object_id, const char *name,
                                         const char *payload_press, const char *icon) {
    if (!client_.connected()) {
        return;
    }
    String payload;
    payload.reserve(420); // Button payload is smaller but still avoid reallocs.
    payload = "{";
    payload += "\"name\":\"";
    payload += name;
    payload += "\",\"unique_id\":\"";
    payload += mqtt_device_id_;
    payload += "_";
    payload += object_id;
    payload += "\",\"command_topic\":\"";
    char topic[kTopicBufferSize];
    build_command_topic(topic, sizeof(topic), mqtt_base_topic_, object_id);
    payload += topic;
    payload += "\",\"payload_press\":\"";
    payload += payload_press;
    payload += "\",\"availability_topic\":\"";
    build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
    payload += topic;
    payload += "\"";
    if (icon && icon[0] != '\0') {
        payload += ",\"icon\":\"";
        payload += icon;
        payload += "\"";
    }
    payload += ",\"device\":{\"identifiers\":[\"";
    payload += mqtt_device_id_;
    payload += "\"],\"name\":\"";
    payload += mqtt_device_name_;
    payload += "\",\"manufacturer\":\"21CNCStudio\",\"model\":\"Project Aura\"}";
    payload += "}";

    build_discovery_topic(topic, sizeof(topic), "button", mqtt_device_id_, object_id);
    client_.publish(topic, payload.c_str(), true);
}

void MqttManager::publishDiscovery() {
    if (!mqtt_discovery_ || mqtt_discovery_sent_ || !client_.connected()) {
        return;
    }
    publishDiscoverySensor("temperature", "Temperature", "\\u00b0C",
                           "temperature", "measurement", "{{ value_json.temp }}", "");
    publishDiscoverySensor("humidity", "Humidity", "%",
                           "humidity", "measurement", "{{ value_json.humidity }}", "");
    publishDiscoverySensor("dew_point", "Dew Point", "\\u00b0C",
                           "temperature", "measurement", "{{ value_json.dew_point }}", "mdi:thermometer-water");
    publishDiscoverySensor("absolute_humidity", "Absolute Humidity", "g/m\\u00b3",
                           "", "measurement", "{{ value_json.absolute_humidity }}", "mdi:water");
    publishDiscoverySensor("co2", "CO2", "ppm",
                           "carbon_dioxide", "measurement", "{{ value_json.co2 }}", "");
    publishDiscoverySensor("voc_index", "VOC Index", "index",
                           "", "measurement", "{{ value_json.voc_index }}", "mdi:blur");
    publishDiscoverySensor("nox_index", "NOx Index", "index",
                           "", "measurement", "{{ value_json.nox_index }}", "mdi:cloud-alert");
    publishDiscoverySensor("hcho", "HCHO", "ppb",
                           "volatile_organic_compounds_parts", "measurement",
                           "{{ value_json.hcho }}", "mdi:flask-outline");
    publishDiscoverySensor("pm1", "PM1.0", "\\u00b5g/m\\u00b3",
                           "", "measurement", "{{ value_json.pm1 }}", "mdi:molecule");
    publishDiscoverySensor("pm25", "PM2.5", "\\u00b5g/m\\u00b3",
                           "pm25", "measurement", "{{ value_json.pm25 }}", "");
    publishDiscoverySensor("pm4", "PM4.0", "\\u00b5g/m\\u00b3",
                           "", "measurement", "{{ value_json.pm4 }}", "mdi:molecule");
    publishDiscoverySensor("pm10", "PM10", "\\u00b5g/m\\u00b3",
                           "pm10", "measurement", "{{ value_json.pm10 }}", "");
    publishDiscoverySensor("pressure", "Pressure", "hPa",
                           "pressure", "measurement", "{{ value_json.pressure }}", "");
    publishDiscoverySensor("pressure_delta_3h", "Pressure Delta 3h", "hPa",
                           "", "measurement", "{{ value_json.pressure_delta_3h }}", "mdi:trending-up");
    publishDiscoverySensor("pressure_delta_24h", "Pressure Delta 24h", "hPa",
                           "", "measurement", "{{ value_json.pressure_delta_24h }}", "mdi:trending-up");
    publishDiscoverySwitch("night_mode", "Night Mode", "{{ value_json.night_mode }}", "mdi:weather-night");
    publishDiscoverySwitch("alert_blink", "Alert Blink", "{{ value_json.alert_blink }}", "mdi:alarm-light");
    publishDiscoverySwitch("backlight", "Backlight", "{{ value_json.backlight }}", "mdi:television");
    publishDiscoveryButton("restart", "Restart", "PRESS", "mdi:restart");
    mqtt_discovery_sent_ = true;
    publishNightModeAvailability();
}

void MqttManager::publishNightModeAvailability() {
    if (!client_.connected()) {
        return;
    }
    char topic[kTopicBufferSize];
    build_night_mode_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
    const char *payload = auto_night_enabled_ ? Config::MQTT_AVAIL_OFFLINE : Config::MQTT_AVAIL_ONLINE;
    client_.publish(topic, payload, true);
}

void MqttManager::publishState(const SensorData &data, bool night_mode, bool alert_blink, bool backlight_on) {
    if (!client_.connected()) {
        return;
    }
    String payload;
    payload.reserve(560); // State payload ~500 bytes for all fields.
    payload += "{";
    bool first = true;
    auto add_int = [&](const char *key, bool valid, int value) {
        if (!first) payload += ",";
        first = false;
        payload += "\"";
        payload += key;
        payload += "\":";
        if (valid) payload.concat(value);
        else payload += "null";
    };
    auto add_float = [&](const char *key, bool valid, float value, int decimals) {
        if (!first) payload += ",";
        first = false;
        payload += "\"";
        payload += key;
        payload += "\":";
        if (valid) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%.*f", decimals, static_cast<double>(value));
            payload += buf;
        } else {
            payload += "null";
        }
    };
    auto add_bool = [&](const char *key, bool value) {
        if (!first) payload += ",";
        first = false;
        payload += "\"";
        payload += key;
        payload += "\":\"";
        payload += value ? "ON" : "OFF";
        payload += "\"";
    };

    float dew_c = NAN;
    bool dew_valid = data.temp_valid && data.hum_valid;
    if (dew_valid) {
        dew_c = computeDewPointC(data.temperature, data.humidity);
        dew_valid = isfinite(dew_c);
    }
    float ah_gm3 = NAN;
    bool ah_valid = data.temp_valid && data.hum_valid;
    if (ah_valid) {
        ah_gm3 = MathUtils::compute_absolute_humidity_gm3(data.temperature, data.humidity);
        ah_valid = isfinite(ah_gm3);
    }

    add_float("temp", data.temp_valid, data.temperature, 1);
    add_float("humidity", data.hum_valid, data.humidity, 1);
    add_float("dew_point", dew_valid, dew_c, 1);
    add_float("absolute_humidity", ah_valid, ah_gm3, 1);
    add_int("co2", data.co2_valid, data.co2);
    add_int("voc_index", data.voc_valid, data.voc_index);
    add_int("nox_index", data.nox_valid, data.nox_index);
    add_float("hcho", data.hcho_valid, data.hcho, 1);
    add_float("pm1", data.pm_valid, data.pm1, 1);
    add_float("pm25", data.pm25_valid, data.pm25, 1);
    add_float("pm4", data.pm_valid, data.pm4, 1);
    add_float("pm10", data.pm10_valid, data.pm10, 1);
    add_float("pressure", data.pressure_valid, data.pressure, 1);
    add_float("pressure_delta_3h", data.pressure_delta_3h_valid, data.pressure_delta_3h, 1);
    add_float("pressure_delta_24h", data.pressure_delta_24h_valid, data.pressure_delta_24h, 1);
    add_bool("night_mode", night_mode);
    add_bool("alert_blink", alert_blink);
    add_bool("backlight", backlight_on);
    payload += "}";

    char topic[kTopicBufferSize];
    build_state_topic(topic, sizeof(topic), mqtt_base_topic_);
    bool published = client_.publish(topic, payload.c_str(), true);

    if (published) {
        mqtt_fail_count_ = 0;
        mqtt_last_publish_ms_ = millis();
    } else {
        mqtt_fail_count_++;
        Logger::log(Logger::Warn, "MQTT", "publish failed (%u/%u)",
                    mqtt_fail_count_, Config::MQTT_MAX_FAILS);

        if (mqtt_fail_count_ >= Config::MQTT_MAX_FAILS) {
            LOGW("MQTT", "too many failures, disconnecting");
            client_.disconnect();
            mqtt_fail_count_ = 0;
        }
    }
}

bool MqttManager::connectClient(const SensorData &data, bool night_mode, bool alert_blink, bool backlight_on) {
    if (!mqtt_enabled_ || mqtt_host_.isEmpty()) {
        return false;
    }
    if (mqtt_retry_exhausted_) {
        return false;
    }
    IPAddress resolved_broker_ip;
    bool using_resolved_ip = false;
    bool is_mdns_host = false;
    if (!prepareBrokerEndpoint(resolved_broker_ip, using_resolved_ip, is_mdns_host)) {
        return false;
    }

    auto note_connect_failure = [&](int rc, bool log_details) {
        if (mqtt_connect_attempts_ < UINT32_MAX) {
            mqtt_connect_attempts_++;
        }
        if (mqtt_connect_attempts_ >= kMqttRetryMaxAttempts) {
            mqtt_retry_exhausted_ = true;
            mqtt_connect_fail_count_ = Config::MQTT_CONNECT_MAX_FAILS;
            LOGW("MQTT", "retries exhausted, manual reconnect required");
            ui_dirty_ = true;
            return;
        }
        if (log_details) {
            uint8_t stage = retry_stage_for_attempts(mqtt_connect_attempts_);
            uint32_t delay_ms = retry_delay_for_stage(stage);
            Logger::log(Logger::Warn, "MQTT",
                        "connect failed rc=%d (attempt %lu/%u), retry in %s",
                        rc,
                        static_cast<unsigned long>(mqtt_connect_attempts_),
                        static_cast<unsigned>(kMqttRetryMaxAttempts),
                        retry_delay_label(delay_ms));
        }
        ui_dirty_ = true;
    };

    if (!mqtt_anonymous_ && (mqtt_user_.isEmpty() || mqtt_pass_.isEmpty())) {
        LOGW("MQTT", "credentials missing and anonymous mode is OFF, connection disabled");
        note_connect_failure(-1, false);
        return false;
    }

    // Diagnostics: check network state before MQTT connect.
    bool network_ready = network_ && network_->isEnabled() && network_->isConnected();
    wl_status_t wifi_status = WiFi.status();
    IPAddress local_ip = WiFi.localIP();
    int32_t rssi = WiFi.RSSI();
    String broker_endpoint = mqtt_host_;
    if (using_resolved_ip) {
        broker_endpoint = resolved_broker_ip.toString();
        LOGI("MQTT", "resolved mDNS broker %s -> %s",
             mqtt_host_.c_str(), broker_endpoint.c_str());
    } else if (is_mdns_host) {
        LOGW("MQTT", "mDNS resolve failed for %s, falling back to system DNS",
             mqtt_host_.c_str());
    }

    Logger::log(Logger::Info, "MQTT",
                "connecting to %s:%u (NetworkMgr=%s, WiFi.status=%d, IP=%s, RSSI=%ld dBm)",
                broker_endpoint.c_str(),
                static_cast<unsigned>(mqtt_port_),
                network_ready ? "ready" : "NOT READY",
                static_cast<int>(wifi_status),
                local_ip.toString().c_str(),
                static_cast<long>(rssi));

    const char *client_id = mqtt_device_id_.c_str();
    char will_topic[kTopicBufferSize];
    build_availability_topic(will_topic, sizeof(will_topic), mqtt_base_topic_);
    bool ok = false;
    if (mqtt_anonymous_) {
        ok = client_.connect(client_id, nullptr, nullptr,
                             will_topic, 0, true, Config::MQTT_AVAIL_OFFLINE);
    } else if (mqtt_user_.length()) {
        ok = client_.connect(client_id, mqtt_user_.c_str(), mqtt_pass_.c_str(),
                             will_topic, 0, true, Config::MQTT_AVAIL_OFFLINE);
    } else {
        ok = client_.connect(client_id, nullptr, nullptr,
                             will_topic, 0, true, Config::MQTT_AVAIL_OFFLINE);
    }
    if (!ok) {
        note_connect_failure(client_.state(), true);
        return false;
    }
    LOGI("MQTT", "connected");
    mqtt_fail_count_ = 0;
    mqtt_connect_fail_count_ = 0;
    mqtt_connect_attempts_ = 0;
    mqtt_retry_exhausted_ = false;
    ui_dirty_ = true;
    char subscribe_topic[kTopicBufferSize];
    snprintf(subscribe_topic, sizeof(subscribe_topic), "%s/command/#", mqtt_base_topic_.c_str());
    client_.subscribe(subscribe_topic);
    client_.publish(will_topic, Config::MQTT_AVAIL_ONLINE, true);
    publishNightModeAvailability();
    mqtt_discovery_sent_ = false;
    publishDiscovery();
    publishState(data, night_mode, alert_blink, backlight_on);
    return true;
}

bool MqttManager::payloadIsOn(const char *payload) {
    if (!payload) {
        return false;
    }
    if (equals_ignore_case(payload, "ON") || strcmp(payload, "1") == 0 ||
        equals_ignore_case(payload, "TRUE") || equals_ignore_case(payload, "PRESS")) {
        return true;
    }
    return false;
}

bool MqttManager::payloadIsOff(const char *payload) {
    if (!payload) {
        return false;
    }
    if (equals_ignore_case(payload, "OFF") || strcmp(payload, "0") == 0 ||
        equals_ignore_case(payload, "FALSE")) {
        return true;
    }
    return false;
}

void MqttManager::handleCallback(char *topic, uint8_t *payload, unsigned int length) {
    String t(topic ? topic : "");
    char msg[32];
    size_t copy_len = length < (sizeof(msg) - 1) ? length : (sizeof(msg) - 1);
    if (payload && copy_len > 0) {
        memcpy(msg, payload, copy_len);
    }
    msg[copy_len] = '\0';
    trim_ascii(msg);
    if (!t.startsWith(mqtt_base_topic_)) {
        return;
    }
    String suffix = t.substring(mqtt_base_topic_.length());
    if (!suffix.startsWith("/command/")) {
        return;
    }
    String cmd = suffix.substring(strlen("/command/"));
    bool is_on = payloadIsOn(msg);
    bool is_off = payloadIsOff(msg);

    if (cmd == "night_mode") {
        if (auto_night_enabled_) {
            LOGI("MQTT", "night mode ignored (auto night enabled)");
            return;
        }
        if (is_on || is_off) {
            pending_.night_mode_value = is_on;
            pending_.night_mode = true;
        }
    } else if (cmd == "alert_blink") {
        if (is_on || is_off) {
            pending_.alert_blink_value = is_on;
            pending_.alert_blink = true;
        }
    } else if (cmd == "backlight") {
        if (is_on || is_off) {
            pending_.backlight_value = is_on;
            pending_.backlight = true;
        }
    } else if (cmd == "restart") {
        if (is_on) {
            pending_.restart = true;
        }
    }
}

void MqttManager::staticCallback(char *topic, uint8_t *payload, unsigned int length) {
    if (g_mqtt) {
        g_mqtt->handleCallback(topic, payload, length);
    }
}

float MqttManager::computeDewPointC(float temp_c, float rh) {
    if (!isfinite(temp_c) || !isfinite(rh) || rh <= 0.0f) {
        return NAN;
    }
    float rh_clamped = fminf(fmaxf(rh, 1.0f), 100.0f);
    constexpr float kA = 17.62f;
    constexpr float kB = 243.12f;
    float gamma = logf(rh_clamped / 100.0f) + (kA * temp_c) / (kB + temp_c);
    return (kB * gamma) / (kA - gamma);
}

void MqttManager::poll(const SensorData &data, bool night_mode, bool alert_blink, bool backlight_on) {
    if (!mqtt_enabled_) {
        if (client_.connected()) {
            char topic[kTopicBufferSize];
            build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
            client_.publish(topic, Config::MQTT_AVAIL_OFFLINE, true);
            client_.disconnect();
            mqtt_fail_count_ = 0;
            if (!mqtt_user_enabled_) {
                mqtt_connect_fail_count_ = 0;
                mqtt_connect_attempts_ = 0;
                mqtt_retry_exhausted_ = false;
            }
        }
        if (mqtt_connected_last_) {
            mqtt_connected_last_ = false;
            ui_dirty_ = true;
        }
        return;
    }
    if (!network_ || !network_->isConnected()) {
        if (client_.connected()) {
            LOGW("MQTT", "network unavailable, disconnecting gracefully");
            char topic[kTopicBufferSize];
            build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
            client_.publish(topic, Config::MQTT_AVAIL_OFFLINE, true);
            client_.disconnect();
            mqtt_fail_count_ = 0;
        }
        if (mqtt_connected_last_) {
            mqtt_connected_last_ = false;
            ui_dirty_ = true;
            LOGI("MQTT", "marked as disconnected (network unavailable)");
        }
        return;
    }

    client_.loop();
    bool connected = client_.connected();
    if (connected != mqtt_connected_last_) {
        mqtt_connected_last_ = connected;
        ui_dirty_ = true;
    }
    if (!connected) {
        if (mqtt_retry_exhausted_) {
            return;
        }
        uint32_t now = millis();
        if (mqtt_connect_attempts_ >= kMqttRetryMaxAttempts) {
            mqtt_retry_exhausted_ = true;
            mqtt_connect_fail_count_ = Config::MQTT_CONNECT_MAX_FAILS;
            ui_dirty_ = true;
            return;
        }
        uint8_t stage = retry_stage_for_attempts(mqtt_connect_attempts_);
        uint32_t retry_delay = retry_delay_for_stage(stage);
        if (now - mqtt_last_attempt_ms_ >= retry_delay) {
            mqtt_last_attempt_ms_ = now;
            connectClient(data, night_mode, alert_blink, backlight_on);
        }
        return;
    }
    publishDiscovery();
    uint32_t now = millis();
    if (mqtt_publish_requested_ || (now - mqtt_last_publish_ms_ >= Config::MQTT_PUBLISH_MS)) {
        mqtt_publish_requested_ = false;
        publishState(data, night_mode, alert_blink, backlight_on);
    }
}

void MqttManager::syncWithWifi() {
    bool wifi_ready = network_ && network_->isEnabled() && network_->isConnected();
    bool desired = mqtt_user_enabled_ && wifi_ready;
    if (desired != mqtt_enabled_) {
        mqtt_enabled_ = desired;
        if (mqtt_enabled_) {
            mqtt_fail_count_ = 0;
            setupClient();
            if (!mqtt_retry_exhausted_) {
                mqtt_connect_fail_count_ = 0;
                mqtt_last_attempt_ms_ = 0;
            }
        } else {
            if (client_.connected()) {
                if (wifi_ready) {
                    char topic[kTopicBufferSize];
                    build_availability_topic(topic, sizeof(topic), mqtt_base_topic_);
                    client_.publish(topic, Config::MQTT_AVAIL_OFFLINE, true);
                }
                client_.disconnect();
            }
            mqtt_fail_count_ = 0;
            if (!mqtt_user_enabled_) {
                mqtt_connect_fail_count_ = 0;
                mqtt_connect_attempts_ = 0;
                mqtt_retry_exhausted_ = false;
            }
        }
    }
    ui_dirty_ = true;
}

void MqttManager::requestReconnect() {
    LOGI("MQTT", "manual reconnect requested");
    mqtt_connect_fail_count_ = 0;
    mqtt_connect_attempts_ = 0;
    mqtt_retry_exhausted_ = false;
    mqtt_last_attempt_ms_ = 0;
    mqtt_mdns_cache_valid_ = false;
    if (client_.connected()) {
        client_.disconnect();
    }
    ui_dirty_ = true;
}

void MqttManager::requestPublish() {
    mqtt_publish_requested_ = true;
}

void MqttManager::setUserEnabled(bool enabled) {
    if (mqtt_user_enabled_ == enabled) {
        return;
    }
    mqtt_user_enabled_ = enabled;
    mqtt_connect_fail_count_ = 0;
    mqtt_connect_attempts_ = 0;
    mqtt_retry_exhausted_ = false;
    if (storage_) {
        storage_->saveMqttEnabled(mqtt_user_enabled_);
    }
}

void MqttManager::updateNightModeAvailability(bool auto_night_enabled) {
    auto_night_enabled_ = auto_night_enabled;
    publishNightModeAvailability();
}

bool MqttManager::takePending(PendingCommands &out) {
    if (!pending_.night_mode && !pending_.alert_blink && !pending_.backlight && !pending_.restart) {
        return false;
    }
    out = pending_;
    pending_ = PendingCommands{};
    return true;
}
