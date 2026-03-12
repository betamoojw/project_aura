// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/SystemLogFilter.h"

#include <string.h>

namespace SystemLogFilter {

bool isSoftWarning(Logger::Level level, const char *tag, const char *message) {
    if (level != Logger::Warn || !tag || !message || strcmp(tag, "Sensors") != 0) {
        return false;
    }

    static constexpr const char *kSoftMetricPrefixes[] = {
        "CO2 ", "CO ", "PM0.5 ", "PM1.0 ", "PM2.5 ",
        "PM4.0 ", "PM10 ", "HCHO ", "VOC ", "NOx "
    };
    for (const char *prefix : kSoftMetricPrefixes) {
        const size_t prefix_len = strlen(prefix);
        if (strncmp(message, prefix, prefix_len) == 0) {
            const char *suffix = message + prefix_len;
            if (strncmp(suffix, "elevated: ", 10) == 0 ||
                strncmp(suffix, "high: ", 6) == 0 ||
                strncmp(suffix, "critical: ", 10) == 0) {
                return true;
            }
            break;
        }
    }

    return strstr(message, " worsened to ") != nullptr ||
           strncmp(message, "Temperature outside recommended range:", 38) == 0 ||
           strncmp(message, "Humidity outside recommended range:", 35) == 0 ||
           strcmp(message, "Pressure sensor not found") == 0 ||
           strcmp(message, "SFA30 not installed") == 0 ||
           strcmp(message, "SEN0466 CO not installed") == 0;
}

bool shouldStoreAlert(Logger::Level level, const char *tag, const char *message) {
    if (level == Logger::Error) {
        return true;
    }
    if (level != Logger::Warn) {
        return false;
    }
    return !isSoftWarning(level, tag, message);
}

} // namespace SystemLogFilter
