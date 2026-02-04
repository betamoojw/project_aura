// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <stdint.h>
#include "config/AppConfig.h"

namespace UiStrings {

using Language = Config::Language;

enum class TextId : uint16_t {
#define UI_STR_ID(name) name,
#include "ui/strings/UiStrings.keys.inc"
#undef UI_STR_ID
    Count
};

void setLanguage(Language lang);
Language language();
const char *text(TextId id);

} // namespace UiStrings
