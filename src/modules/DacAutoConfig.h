// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <Arduino.h>

struct DacAutoBandConfig {
    uint8_t green_percent = 20;
    uint8_t yellow_percent = 40;
    uint8_t orange_percent = 70;
    uint8_t red_percent = 100;
};

struct DacAutoSensorConfig {
    bool enabled = true;
    DacAutoBandConfig band{};
};

struct DacAutoConfig {
    bool enabled = false;

    DacAutoSensorConfig co2{true, {30, 50, 70, 100}};
    DacAutoSensorConfig co{true, {20, 50, 100, 100}};
    DacAutoSensorConfig pm25{true, {20, 40, 70, 100}};
    DacAutoSensorConfig voc{true, {20, 50, 80, 100}};
    DacAutoSensorConfig nox{true, {20, 40, 70, 100}};
};

namespace DacAutoConfigJson {

void sanitize(DacAutoConfig &cfg);
String serialize(const DacAutoConfig &cfg);
bool deserialize(const String &json, DacAutoConfig &cfg);

} // namespace DacAutoConfigJson

