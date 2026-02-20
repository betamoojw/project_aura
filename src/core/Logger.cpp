// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/Logger.h"

#include <stdio.h>
#include <string.h>

namespace {
constexpr size_t kLogBufferSize = 256;
constexpr uint32_t kRecentDedupWindowMs = 30000;
}

HardwareSerial *Logger::serial_ = &Serial;
Logger::Level Logger::level_ = Logger::Info;
Logger::RecentEntry Logger::recent_[Logger::kRecentCapacity];
size_t Logger::recent_head_ = 0;
size_t Logger::recent_count_ = 0;

void Logger::begin(HardwareSerial &serial, Level level) {
    serial_ = &serial;
    level_ = level;
}

void Logger::setLevel(Level level) {
    level_ = level;
}

Logger::Level Logger::level() {
    return level_;
}

const char *Logger::levelName(Level level) {
    switch (level) {
        case Error:
            return "E";
        case Warn:
            return "W";
        case Info:
            return "I";
        case Debug:
            return "D";
        default:
            return "?";
    }
}

void Logger::log(Level level, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog(level, tag, fmt, args);
    va_end(args);
}

void Logger::vlog(Level level, const char *tag, const char *fmt, va_list args) {
    if (level > level_) {
        return;
    }

    char buffer[kLogBufferSize];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (serial_) {
        serial_->print('[');
        serial_->print(levelName(level));
        serial_->print(']');
        if (tag && tag[0] != '\0') {
            serial_->print('[');
            serial_->print(tag);
            serial_->print(']');
        }
        serial_->print(' ');
        serial_->println(buffer);
    }

    storeRecent(level, tag, buffer);
}

void Logger::storeRecent(Level level, const char *tag, const char *message) {
    uint32_t now_ms = 0;
#if defined(ARDUINO)
    now_ms = millis();
#endif

    char tag_buf[sizeof(RecentEntry::tag)];
    char message_buf[sizeof(RecentEntry::message)];
    tag_buf[0] = '\0';
    message_buf[0] = '\0';

    if (tag) {
        strncpy(tag_buf, tag, sizeof(tag_buf) - 1);
        tag_buf[sizeof(tag_buf) - 1] = '\0';
    }
    if (message) {
        strncpy(message_buf, message, sizeof(message_buf) - 1);
        message_buf[sizeof(message_buf) - 1] = '\0';
    }

    if (recent_count_ > 0) {
        const size_t last_index = (recent_head_ + kRecentCapacity - 1) % kRecentCapacity;
        const RecentEntry &last = recent_[last_index];
        const bool same_event =
            last.level == level &&
            strcmp(last.tag, tag_buf) == 0 &&
            strcmp(last.message, message_buf) == 0;
        const bool within_dedup_window = (now_ms - last.ms) <= kRecentDedupWindowMs;
        if (same_event && within_dedup_window) {
            return;
        }
    }

    RecentEntry &entry = recent_[recent_head_];
    entry.ms = now_ms;
    entry.level = level;
    strncpy(entry.tag, tag_buf, sizeof(entry.tag) - 1);
    entry.tag[sizeof(entry.tag) - 1] = '\0';
    strncpy(entry.message, message_buf, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    recent_head_ = (recent_head_ + 1) % kRecentCapacity;
    if (recent_count_ < kRecentCapacity) {
        recent_count_++;
    }
}

size_t Logger::copyRecent(RecentEntry *out, size_t max_entries) {
    if (!out || max_entries == 0 || recent_count_ == 0) {
        return 0;
    }

    const size_t to_copy = (recent_count_ < max_entries) ? recent_count_ : max_entries;
    const size_t start = (recent_head_ + kRecentCapacity - to_copy) % kRecentCapacity;
    for (size_t i = 0; i < to_copy; ++i) {
        out[i] = recent_[(start + i) % kRecentCapacity];
    }
    return to_copy;
}
