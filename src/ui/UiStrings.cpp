// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiStrings.h"

#include <stddef.h>

namespace UiStrings {

namespace {

constexpr const char *kStringsEn[] = {
    "---",
    "--",
    "0%",
    "--:--",
    "--.--.----",
    "C",
    "F",
    "ppb",
    "Index",
    "HCHO",
    "AQI",
    "Initializing",
    "Fresh Air - All Good",
    "Excellent",
    "Good",
    "Moderate",
    "Poor",
    "OFF",
    "ON",
    "OK",
    "ERR",
    "SYNC",
    "Connected",
    "AP Mode",
    "Error",
    "Connecting",
    "Disabled",
    "No WiFi",
    "Connected",
    "Error",
    "Retrying (10m)",
    "Retrying (1h)",
    "Connecting...",
    "ON / OFF",
    "Every 6h",
    "http://192.168.4.1",
    "http://aura.local/mqtt",
    "http://aura.local/theme",
    "CO2 Very High! Ventilate now",
    "CO2 High - Ventilate soon",
    "CO2 Rising - Consider ventilation",
    "PM2.5 Very High! Purify - stop smoke",
    "PM2.5 High - Purify - check source",
    "PM2.5 Elevated - Consider purifier",
    "PM10 Very High! Purify - check source",
    "PM10 High - Purify - reduce dust",
    "PM10 Elevated - Purify - reduce dust",
    "HCHO Very High! Ventilate - remove source",
    "HCHO High - Ventilate - remove source",
    "HCHO Detected - Check sources",
    "VOC Very High! Find source - vent",
    "VOC High - Increase ventilation",
    "NOx Very High! Stop source - ventilate",
    "NOx High - Use hood - check source",
    "NOx Elevated - Check stove/outside",
    "Temp Too Cold! Heat now",
    "Temp Cold - Heat the room",
    "Temp Slightly Cool - Add heat",
    "Temp Too Hot! Cool now",
    "Temp Warm - Ventilate / cool",
    "Temp Slightly Warm - Increase airflow",
    "DP Very Low! Humidify",
    "DP Low - Air may feel dry",
    "DP Muggy! Dehumidify now",
    "DP Very High - Vent/dehumidify",
    "DP High - Air may feel muggy",
    "Humidity Extremely Low! Humidify",
    "Humidity Very Low - Use humidifier",
    "Humidity Low - Consider humidifier",
    "Humidity Extremely High! Dehumidify",
    "Humidity Very High - Dehumidify",
    "Humidity High - Increase airflow"
};

// TODO: fill translations. Null entries fall back to English.
constexpr const char *kStringsDe[static_cast<size_t>(TextId::Count)] = {};
constexpr const char *kStringsEs[static_cast<size_t>(TextId::Count)] = {};

static_assert(sizeof(kStringsEn) / sizeof(kStringsEn[0]) ==
              static_cast<size_t>(TextId::Count),
              "UiStrings: EN table size mismatch");

Language g_language = Language::EN;

const char *const *tableFor(Language lang) {
    switch (lang) {
        case Language::DE: return kStringsDe;
        case Language::ES: return kStringsEs;
        case Language::EN:
        default:
            return kStringsEn;
    }
}

} // namespace

void setLanguage(Language lang) {
    g_language = lang;
}

Language language() {
    return g_language;
}

const char *text(TextId id) {
    const size_t index = static_cast<size_t>(id);
    const char *const *table = tableFor(g_language);
    const char *value = table ? table[index] : nullptr;
    if (!value || !*value) {
        value = kStringsEn[index];
    }
    return value ? value : "";
}

} // namespace UiStrings
