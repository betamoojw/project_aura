// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "config/AppData.h"

const TimeZoneEntry kTimeZones[] = {
    { "Etc/GMT+12", -12 * 60, nullptr },
    { "Pacific/Midway", -11 * 60, nullptr },
    { "Pacific/Honolulu", -10 * 60, nullptr },
    { "America/Anchorage", -9 * 60, nullptr },
    { "America/Los_Angeles", -8 * 60, nullptr },
    { "America/Denver", -7 * 60, nullptr },
    { "America/Chicago", -6 * 60, nullptr },
    { "America/New_York", -5 * 60, "EST5EDT,M3.2.0,M11.1.0" },
    { "America/Santiago", -4 * 60, nullptr },
    { "America/St_Johns", -3 * 60 - 30, nullptr },
    { "America/Sao_Paulo", -3 * 60, nullptr },
    { "Atlantic/South_Georgia", -2 * 60, nullptr },
    { "Atlantic/Azores", -1 * 60, nullptr },
    { "Europe/London", 0, "GMT0BST,M3.5.0/1,M10.5.0" },
    { "Europe/Paris", 1 * 60, "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Kiev", 2 * 60, "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Africa/Cairo", 2 * 60, nullptr },
    { "Europe/Moscow", 3 * 60, "MSK-3" },
    { "Asia/Tehran", 3 * 60 + 30, nullptr },
    { "Asia/Dubai", 4 * 60, nullptr },
    { "Asia/Kabul", 4 * 60 + 30, nullptr },
    { "Asia/Karachi", 5 * 60, nullptr },
    { "Asia/Kolkata", 5 * 60 + 30, nullptr },
    { "Asia/Kathmandu", 5 * 60 + 45, nullptr },
    { "Asia/Dhaka", 6 * 60, nullptr },
    { "Asia/Yangon", 6 * 60 + 30, nullptr },
    { "Asia/Bangkok", 7 * 60, nullptr },
    { "Asia/Shanghai", 8 * 60, "CST-8" },
    { "Asia/Singapore", 8 * 60, nullptr },
    { "Asia/Tokyo", 9 * 60, nullptr },
    { "Australia/Adelaide", 9 * 60 + 30, nullptr },
    { "Australia/Sydney", 10 * 60, nullptr },
    { "Pacific/Noumea", 11 * 60, nullptr },
    { "Pacific/Auckland", 12 * 60, nullptr },
    { "Pacific/Chatham", 12 * 60 + 45, nullptr },
    { "Pacific/Tongatapu", 13 * 60, nullptr },
    { "Pacific/Kiritimati", 14 * 60, nullptr }
};

const size_t TIME_ZONE_COUNT = sizeof(kTimeZones) / sizeof(kTimeZones[0]);
