// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/OtaDeferredRestart.h"

namespace OtaDeferredRestart {

bool deadline_reached(uint32_t now_ms, uint32_t due_ms) {
    return static_cast<int32_t>(now_ms - due_ms) >= 0;
}

void Controller::reset() {
    deferred_restart_ = false;
    restart_requested_ = false;
    deferred_restart_due_ms_ = 0;
}

void Controller::schedule(uint32_t now_ms, uint32_t delay_ms) {
    deferred_restart_ = true;
    restart_requested_ = false;
    deferred_restart_due_ms_ = now_ms + delay_ms;
}

bool Controller::poll(uint32_t now_ms) {
    if (!deferred_restart_ || !deadline_reached(now_ms, deferred_restart_due_ms_)) {
        return false;
    }
    deferred_restart_ = false;
    deferred_restart_due_ms_ = 0;
    restart_requested_ = true;
    return true;
}

bool Controller::consume_request() {
    if (!restart_requested_) {
        return false;
    }
    restart_requested_ = false;
    return true;
}

bool Controller::is_busy(bool upload_active) const {
    return upload_active || deferred_restart_ || restart_requested_;
}

} // namespace OtaDeferredRestart
