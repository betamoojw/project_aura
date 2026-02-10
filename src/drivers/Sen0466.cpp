// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "drivers/Sen0466.h"

#include <driver/i2c.h>
#include <math.h>
#include <string.h>

#include "config/AppConfig.h"
#include "core/Logger.h"

namespace {

constexpr uint8_t kFrameLen = 9;

} // namespace

bool Sen0466::begin() {
    present_ = false;
    data_valid_ = false;
    warned_type_mismatch_ = false;
    co_ppm_ = 0.0f;
    fail_count_ = 0;
    warmup_started_ms_ = 0;
    last_poll_ms_ = 0;
    last_data_ms_ = 0;
    last_retry_ms_ = 0;
    return true;
}

bool Sen0466::start() {
    last_retry_ms_ = millis();
    if (!pingAddress()) {
        present_ = false;
        data_valid_ = false;
        co_ppm_ = 0.0f;
        fail_count_ = 0;
        warned_type_mismatch_ = false;
        return false;
    }

    const bool was_present = present_;
    present_ = true;
    if (!was_present) {
        warmup_started_ms_ = millis();
        data_valid_ = false;
        co_ppm_ = 0.0f;
        fail_count_ = 0;
        warned_type_mismatch_ = false;
    }

    if (!setPassiveMode()) {
        LOGW("SEN0466", "failed to set passive mode");
    }
    return true;
}

void Sen0466::poll() {
    const uint32_t now = millis();

    if (!present_) {
        if (now - last_retry_ms_ >= Config::SEN0466_RETRY_MS) {
            start();
        }
        return;
    }

    if (data_valid_ && last_data_ms_ != 0 &&
        (now - last_data_ms_ > Config::SEN0466_STALE_MS)) {
        data_valid_ = false;
    }

    if (now - last_poll_ms_ < Config::SEN0466_POLL_MS) {
        return;
    }
    last_poll_ms_ = now;

    float co_ppm = 0.0f;
    uint8_t gas_type = 0;
    if (!readGasConcentration(co_ppm, gas_type)) {
        if (fail_count_ < UINT8_MAX) {
            ++fail_count_;
        }
        if (fail_count_ >= Config::SEN0466_MAX_FAILS) {
            data_valid_ = false;
        }
        return;
    }

    fail_count_ = 0;
    last_data_ms_ = now;

    if (gas_type != Config::SEN0466_GAS_TYPE_CO) {
        if (!warned_type_mismatch_) {
            Logger::log(Logger::Warn, "SEN0466",
                        "unexpected gas type 0x%02X (expected 0x%02X)",
                        gas_type, Config::SEN0466_GAS_TYPE_CO);
            warned_type_mismatch_ = true;
        }
        data_valid_ = false;
        return;
    }
    warned_type_mismatch_ = false;

    if (!isfinite(co_ppm) || co_ppm < Config::SEN0466_CO_MIN_PPM) {
        data_valid_ = false;
        return;
    }
    if (co_ppm > Config::SEN0466_CO_MAX_PPM) {
        co_ppm = Config::SEN0466_CO_MAX_PPM;
    }

    co_ppm_ = co_ppm;
    data_valid_ = !isWarmupActive();
}

bool Sen0466::isWarmupActive() const {
    if (!present_ || warmup_started_ms_ == 0) {
        return false;
    }
    return (millis() - warmup_started_ms_) < Config::SEN0466_WARMUP_MS;
}

void Sen0466::invalidate() {
    data_valid_ = false;
}

bool Sen0466::pingAddress() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return false;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (Config::SEN0466_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(
        Config::I2C_PORT,
        cmd,
        pdMS_TO_TICKS(Config::I2C_TIMEOUT_MS)
    );
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool Sen0466::setPassiveMode() {
    uint8_t tx[kFrameLen] = {0};
    buildFrame(Config::SEN0466_CMD_CHANGE_MODE, Config::SEN0466_MODE_PASSIVE, 0, 0, 0, 0, tx);

    uint8_t rx[kFrameLen] = {0};
    if (!transact(tx, rx)) {
        return false;
    }
    if (rx[0] != 0xFF || rx[1] != Config::SEN0466_CMD_CHANGE_MODE) {
        return false;
    }
    if (rx[8] != checksum7(rx) && rx[8] != checksum6(rx)) {
        return false;
    }
    return rx[2] == 0x01;
}

bool Sen0466::readGasConcentration(float &co_ppm, uint8_t &gas_type) {
    uint8_t tx[kFrameLen] = {0};
    buildFrame(Config::SEN0466_CMD_READ_GAS, 0, 0, 0, 0, 0, tx);

    uint8_t rx[kFrameLen] = {0};
    if (!transact(tx, rx)) {
        return false;
    }
    if (rx[0] != 0xFF || rx[1] != Config::SEN0466_CMD_READ_GAS) {
        return false;
    }
    if (rx[8] != checksum7(rx) && rx[8] != checksum6(rx)) {
        return false;
    }

    const uint16_t raw = static_cast<uint16_t>(rx[2] << 8) | rx[3];
    const uint8_t decimals = rx[5];
    float scale = 1.0f;
    if (decimals == 1) {
        scale = 0.1f;
    } else if (decimals == 2) {
        scale = 0.01f;
    } else if (decimals != 0) {
        return false;
    }

    co_ppm = static_cast<float>(raw) * scale;
    gas_type = rx[4];
    return true;
}

bool Sen0466::transact(const uint8_t *tx_frame, uint8_t *rx_frame) {
    uint8_t tx[kFrameLen + 1] = {0};
    tx[0] = 0x00;
    memcpy(&tx[1], tx_frame, kFrameLen);

    esp_err_t err = i2c_master_write_to_device(
        Config::I2C_PORT,
        Config::SEN0466_ADDR,
        tx,
        sizeof(tx),
        pdMS_TO_TICKS(Config::I2C_TIMEOUT_MS)
    );
    if (err != ESP_OK) {
        return false;
    }

    delay(Config::SEN0466_CMD_DELAY_MS);

    uint8_t reg = 0x00;
    err = i2c_master_write_read_device(
        Config::I2C_PORT,
        Config::SEN0466_ADDR,
        &reg,
        1,
        rx_frame,
        kFrameLen,
        pdMS_TO_TICKS(Config::I2C_TIMEOUT_MS)
    );
    return err == ESP_OK;
}

uint8_t Sen0466::checksum7(const uint8_t *frame) {
    uint8_t sum = 0;
    for (uint8_t i = 1; i <= 7; ++i) {
        sum = static_cast<uint8_t>(sum + frame[i]);
    }
    return static_cast<uint8_t>(~sum + 1);
}

uint8_t Sen0466::checksum6(const uint8_t *frame) {
    uint8_t sum = 0;
    for (uint8_t i = 1; i <= 6; ++i) {
        sum = static_cast<uint8_t>(sum + frame[i]);
    }
    return static_cast<uint8_t>(~sum + 1);
}

void Sen0466::buildFrame(uint8_t command,
                         uint8_t arg0,
                         uint8_t arg1,
                         uint8_t arg2,
                         uint8_t arg3,
                         uint8_t arg4,
                         uint8_t *frame) {
    frame[0] = 0xFF;
    frame[1] = 0x01;
    frame[2] = command;
    frame[3] = arg0;
    frame[4] = arg1;
    frame[5] = arg2;
    frame[6] = arg3;
    frame[7] = arg4;
    frame[8] = checksum7(frame);
}
