#pragma once

#include <Arduino.h>

namespace WebInputValidation {

constexpr size_t kWifiSsidMaxBytes = 32;

bool isWifiSsidValid(const String &value, size_t max_len = kWifiSsidMaxBytes);
bool parsePortOrDefault(const String &value, uint16_t default_port, uint16_t &out_port);

} // namespace WebInputValidation
