// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#pragma once

// Initialize the dedicated Core0 restart task ahead of time.
// Returns true if the task is ready.
bool safe_restart_init();

// Restart the system with Core 0 as the initiator without using IPC task stack.
// Does not return.
[[noreturn]] void safe_restart_via_core0();
