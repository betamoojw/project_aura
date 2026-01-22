// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

#include <math.h>

namespace MathUtils {

inline float compute_dew_point_c(float temp_c, float rh) {
    if (!isfinite(temp_c) || !isfinite(rh) || rh <= 0.0f) {
        return NAN;
    }
    float rh_clamped = fminf(fmaxf(rh, 1.0f), 100.0f);
    constexpr float kA = 17.62f;
    constexpr float kB = 243.12f;
    float gamma = logf(rh_clamped / 100.0f) + (kA * temp_c) / (kB + temp_c);
    return (kB * gamma) / (kA - gamma);
}

} // namespace MathUtils
