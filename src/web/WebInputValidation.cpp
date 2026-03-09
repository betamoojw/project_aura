#include "web/WebInputValidation.h"

#include <ctype.h>
#include <stdlib.h>

namespace WebInputValidation {

bool isWifiSsidValid(const String &value, size_t max_len) {
    const size_t len = value.length();
    return len > 0 && len <= max_len;
}

bool parsePortOrDefault(const String &value, uint16_t default_port, uint16_t &out_port) {
    const char *raw = value.c_str();
    if (!raw) {
        out_port = default_port;
        return true;
    }

    while (*raw != '\0' && isspace(static_cast<unsigned char>(*raw))) {
        ++raw;
    }
    if (*raw == '\0') {
        out_port = default_port;
        return true;
    }

    char *end = nullptr;
    const unsigned long parsed = strtoul(raw, &end, 10);
    while (end && *end != '\0' && isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (!end || *end != '\0' || parsed == 0 || parsed > 65535UL) {
        return false;
    }

    out_port = static_cast<uint16_t>(parsed);
    return true;
}

} // namespace WebInputValidation
