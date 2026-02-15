// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "modules/DacAutoConfig.h"

#include <ArduinoJson.h>

namespace {

uint8_t clamp_percent(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return static_cast<uint8_t>(value);
}

void sanitize_band(DacAutoBandConfig &band) {
    band.green_percent = clamp_percent(band.green_percent);
    band.yellow_percent = clamp_percent(band.yellow_percent);
    band.orange_percent = clamp_percent(band.orange_percent);
    band.red_percent = clamp_percent(band.red_percent);
}

void sanitize_sensor(DacAutoSensorConfig &sensor) {
    sanitize_band(sensor.band);
}

void write_sensor(ArduinoJson::JsonObject obj, const DacAutoSensorConfig &sensor) {
    obj["enabled"] = sensor.enabled;
    obj["green"] = sensor.band.green_percent;
    obj["yellow"] = sensor.band.yellow_percent;
    obj["orange"] = sensor.band.orange_percent;
    obj["red"] = sensor.band.red_percent;
}

void read_sensor(ArduinoJson::JsonObjectConst obj, DacAutoSensorConfig &sensor) {
    if (obj.isNull()) {
        return;
    }
    sensor.enabled = obj["enabled"] | sensor.enabled;
    sensor.band.green_percent = clamp_percent(obj["green"] | sensor.band.green_percent);
    sensor.band.yellow_percent = clamp_percent(obj["yellow"] | sensor.band.yellow_percent);
    sensor.band.orange_percent = clamp_percent(obj["orange"] | sensor.band.orange_percent);
    sensor.band.red_percent = clamp_percent(obj["red"] | sensor.band.red_percent);
}

} // namespace

namespace DacAutoConfigJson {

void sanitize(DacAutoConfig &cfg) {
    sanitize_sensor(cfg.co2);
    sanitize_sensor(cfg.co);
    sanitize_sensor(cfg.pm25);
    sanitize_sensor(cfg.voc);
    sanitize_sensor(cfg.nox);
}

String serialize(const DacAutoConfig &cfg) {
    DacAutoConfig sanitized = cfg;
    sanitize(sanitized);

    ArduinoJson::JsonDocument doc;
    ArduinoJson::JsonObject root = doc.to<ArduinoJson::JsonObject>();
    root["enabled"] = sanitized.enabled;
    write_sensor(root["co2"].to<ArduinoJson::JsonObject>(), sanitized.co2);
    write_sensor(root["co"].to<ArduinoJson::JsonObject>(), sanitized.co);
    write_sensor(root["pm25"].to<ArduinoJson::JsonObject>(), sanitized.pm25);
    write_sensor(root["voc"].to<ArduinoJson::JsonObject>(), sanitized.voc);
    write_sensor(root["nox"].to<ArduinoJson::JsonObject>(), sanitized.nox);

    String out;
    serializeJson(doc, out);
    return out;
}

bool deserialize(const String &json, DacAutoConfig &cfg) {
    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError err = ArduinoJson::deserializeJson(doc, json);
    if (err) {
        return false;
    }

    DacAutoConfig parsed;
    ArduinoJson::JsonObjectConst root = doc.as<ArduinoJson::JsonObjectConst>();
    ArduinoJson::JsonObjectConst source = root;
    if (root["auto"].is<ArduinoJson::JsonObjectConst>()) {
        source = root["auto"].as<ArduinoJson::JsonObjectConst>();
    }

    parsed.enabled = source["enabled"] | parsed.enabled;
    read_sensor(source["co2"].as<ArduinoJson::JsonObjectConst>(), parsed.co2);
    read_sensor(source["co"].as<ArduinoJson::JsonObjectConst>(), parsed.co);
    read_sensor(source["pm25"].as<ArduinoJson::JsonObjectConst>(), parsed.pm25);
    read_sensor(source["voc"].as<ArduinoJson::JsonObjectConst>(), parsed.voc);
    read_sensor(source["nox"].as<ArduinoJson::JsonObjectConst>(), parsed.nox);

    sanitize(parsed);
    cfg = parsed;
    return true;
}

} // namespace DacAutoConfigJson

