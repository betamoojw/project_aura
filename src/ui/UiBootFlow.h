// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <Arduino.h>

class UiController;

class UiBootFlow {
public:
    static void releaseBootScreens(UiController &owner);
    static void updateBootDiag(UiController &owner, uint32_t now_ms);

private:
    static void clearBootObjectRefs(UiController &owner);
    static bool bootDiagHasErrors(UiController &owner, uint32_t now_ms);
};
