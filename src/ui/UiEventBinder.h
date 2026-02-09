// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <lvgl.h>

class UiController;

class UiEventBinder {
public:
    static lv_obj_t *screenRootById(int screen_id);
    static bool objectBelongsToScreen(lv_obj_t *obj, lv_obj_t *screen_root);

    static void bindAvailableEvents(UiController &owner, int screen_id);
    static void applyToggleStylesForAvailableObjects(UiController &owner, int screen_id);
    static void applyCheckedStatesForAvailableObjects(UiController &owner, int screen_id);
    static void initThemeControlsIfAvailable(UiController &owner);
};

