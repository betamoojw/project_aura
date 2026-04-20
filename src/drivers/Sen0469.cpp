// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "drivers/Sen0469.h"

#include "config/AppConfig.h"

Sen0469::Sen0469()
    : DfrMultiGasSensor({
          "SEN0469",
          "SEN0469 NH3",
          Config::SEN0469_ADDR,
          Config::SEN0469_GAS_TYPE_NH3,
          Config::SEN0469_NH3_MIN_PPM,
          Config::SEN0469_NH3_MAX_PPM,
      }) {}
