// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "drivers/DfrMultiGasSensor.h"

#include <driver/i2c.h>
#include <math.h>
#include <string.h>

#include "config/AppConfig.h"
#include "core/Logger.h"

namespace {

constexpr uint8_t kFrameLen = 9;

} // namespace

bool DfrMultiGasSensor::begin() {
    present_ = false;
    data_valid_ = false;
    warned_type_mismatch_ = false;
    ppm_ = 0.0f;
    fail_count_ = 0;
    warmup_started_ms_ = 0;
    last_poll_ms_ = 0;
    last_data_ms_ = 0;
    last_retry_ms_ = 0;
    fail_cooldown_active_ = false;
    fail_cooldown_started_ms_ = 0;
    cooldown_recover_fail_count_ = 0;
    start_attempts_ = 0;
    start_retry_exhausted_logged_ = false;
    return true;
}

bool DfrMultiGasSensor::start() {
    last_retry_ms_ = millis();
    if (!pingAddress()) {
        if (start_attempts_ < UINT8_MAX) {
            ++start_attempts_;
        }
        if (!start_retry_exhausted_logged_ &&
            start_attempts_ >= Config::DFR_GAS_MAX_START_ATTEMPTS) {
            LOGI(config_.log_tag, "not installed after %u attempts, stop probing until reboot",
                 static_cast<unsigned>(Config::DFR_GAS_MAX_START_ATTEMPTS));
            start_retry_exhausted_logged_ = true;
        }
        present_ = false;
        data_valid_ = false;
        ppm_ = 0.0f;
        fail_count_ = 0;
        warned_type_mismatch_ = false;
        fail_cooldown_active_ = false;
        fail_cooldown_started_ms_ = 0;
        cooldown_recover_fail_count_ = 0;
        return false;
    }

    start_attempts_ = 0;
    start_retry_exhausted_logged_ = false;
    const bool was_present = present_;
    present_ = true;
    if (!was_present) {
        warmup_started_ms_ = millis();
        data_valid_ = false;
        ppm_ = 0.0f;
        fail_count_ = 0;
        warned_type_mismatch_ = false;
        fail_cooldown_active_ = false;
        fail_cooldown_started_ms_ = 0;
        cooldown_recover_fail_count_ = 0;
    }

    if (!setPassiveMode()) {
        LOGW(config_.log_tag, "failed to set passive mode");
    }
    return true;
}

void DfrMultiGasSensor::poll() {
    const uint32_t now = millis();

    if (!present_) {
        if (start_attempts_ >= Config::DFR_GAS_MAX_START_ATTEMPTS) {
            return;
        }
        if (now - last_retry_ms_ >= Config::DFR_GAS_RETRY_MS) {
            start();
        }
        return;
    }

    if (fail_cooldown_active_) {
        if (now - fail_cooldown_started_ms_ < Config::DFR_GAS_FAIL_COOLDOWN_MS) {
            return;
        }

        fail_cooldown_active_ = false;
        fail_cooldown_started_ms_ = 0;
        if (!setPassiveMode()) {
            if (cooldown_recover_fail_count_ < UINT8_MAX) {
                ++cooldown_recover_fail_count_;
            }
            if (cooldown_recover_fail_count_ >= Config::DFR_GAS_MAX_COOLDOWN_RECOVERY_FAILS) {
                LOGW(config_.log_tag, "cooldown recovery failed %u times, marking sensor not present",
                     static_cast<unsigned>(cooldown_recover_fail_count_));
                present_ = false;
                data_valid_ = false;
                ppm_ = 0.0f;
                fail_count_ = 0;
                warned_type_mismatch_ = false;
                fail_cooldown_active_ = false;
                fail_cooldown_started_ms_ = 0;
                cooldown_recover_fail_count_ = 0;
                last_retry_ms_ = now;
                return;
            }
            fail_cooldown_active_ = true;
            fail_cooldown_started_ms_ = now;
            LOGW(config_.log_tag, "cooldown elapsed, passive mode restore failed (%u/%u)",
                 static_cast<unsigned>(cooldown_recover_fail_count_),
                 static_cast<unsigned>(Config::DFR_GAS_MAX_COOLDOWN_RECOVERY_FAILS));
            return;
        }

        cooldown_recover_fail_count_ = 0;
        fail_count_ = 0;
        warned_type_mismatch_ = false;
        last_poll_ms_ = now;
        LOGI(config_.log_tag, "cooldown elapsed, passive mode restored");
        return;
    }

    if (data_valid_ && last_data_ms_ != 0 &&
        (now - last_data_ms_ > Config::DFR_GAS_STALE_MS)) {
        data_valid_ = false;
    }

    if (now - last_poll_ms_ < Config::DFR_GAS_POLL_MS) {
        return;
    }
    last_poll_ms_ = now;

    float ppm = 0.0f;
    uint8_t gas_type = 0;
    if (!readGasConcentration(ppm, gas_type)) {
        if (fail_count_ < UINT8_MAX) {
            ++fail_count_;
        }
        if (fail_count_ >= Config::DFR_GAS_MAX_FAILS) {
            data_valid_ = false;
            fail_cooldown_active_ = true;
            fail_cooldown_started_ms_ = now;
            cooldown_recover_fail_count_ = 0;
            LOGW(config_.log_tag, "read failed %u times, entering cooldown %lu ms",
                 static_cast<unsigned>(fail_count_),
                 static_cast<unsigned long>(Config::DFR_GAS_FAIL_COOLDOWN_MS));
        }
        return;
    }

    cooldown_recover_fail_count_ = 0;
    fail_count_ = 0;
    last_data_ms_ = now;

    if (gas_type != config_.expected_gas_type) {
        if (!warned_type_mismatch_) {
            Logger::log(Logger::Warn, config_.log_tag,
                        "unexpected gas type 0x%02X (expected 0x%02X)",
                        gas_type, config_.expected_gas_type);
            warned_type_mismatch_ = true;
        }
        data_valid_ = false;
        return;
    }
    warned_type_mismatch_ = false;

    if (!isfinite(ppm) || ppm < config_.min_ppm) {
        data_valid_ = false;
        return;
    }
    if (ppm > config_.max_ppm) {
        ppm = config_.max_ppm;
    }

    ppm_ = ppm;
    data_valid_ = !isWarmupActive();
}

bool DfrMultiGasSensor::isWarmupActive() const {
    if (!present_ || warmup_started_ms_ == 0) {
        return false;
    }
    return (millis() - warmup_started_ms_) < Config::DFR_GAS_WARMUP_MS;
}

void DfrMultiGasSensor::invalidate() {
    data_valid_ = false;
}

bool DfrMultiGasSensor::pingAddress() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return false;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (config_.address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(
        Config::I2C_PORT,
        cmd,
        pdMS_TO_TICKS(Config::DFR_GAS_I2C_TIMEOUT_MS)
    );
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool DfrMultiGasSensor::setPassiveMode() {
    uint8_t tx[kFrameLen] = {0};
    buildFrame(Config::DFR_GAS_CMD_CHANGE_MODE, Config::DFR_GAS_MODE_PASSIVE, 0, 0, 0, 0, tx);

    uint8_t rx[kFrameLen] = {0};
    if (!transact(tx, rx)) {
        return false;
    }
    if (rx[0] != 0xFF || rx[1] != Config::DFR_GAS_CMD_CHANGE_MODE) {
        return false;
    }
    if (rx[8] != checksum7(rx) && rx[8] != checksum6(rx)) {
        return false;
    }
    return rx[2] == 0x01;
}

bool DfrMultiGasSensor::readGasConcentration(float &ppm, uint8_t &gas_type) {
    uint8_t tx[kFrameLen] = {0};
    buildFrame(Config::DFR_GAS_CMD_READ_GAS, 0, 0, 0, 0, 0, tx);

    uint8_t rx[kFrameLen] = {0};
    if (!transact(tx, rx)) {
        return false;
    }
    if (rx[0] != 0xFF || rx[1] != Config::DFR_GAS_CMD_READ_GAS) {
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

    ppm = static_cast<float>(raw) * scale;
    gas_type = rx[4];
    return true;
}

bool DfrMultiGasSensor::transact(const uint8_t *tx_frame, uint8_t *rx_frame) {
    uint8_t tx[kFrameLen + 1] = {0};
    tx[0] = 0x00;
    memcpy(&tx[1], tx_frame, kFrameLen);

    esp_err_t err = i2c_master_write_to_device(
        Config::I2C_PORT,
        config_.address,
        tx,
        sizeof(tx),
        pdMS_TO_TICKS(Config::DFR_GAS_I2C_TIMEOUT_MS)
    );
    if (err != ESP_OK) {
        return false;
    }

    delay(Config::DFR_GAS_CMD_DELAY_MS);

    uint8_t reg = 0x00;
    err = i2c_master_write_read_device(
        Config::I2C_PORT,
        config_.address,
        &reg,
        1,
        rx_frame,
        kFrameLen,
        pdMS_TO_TICKS(Config::DFR_GAS_I2C_TIMEOUT_MS)
    );
    return err == ESP_OK;
}

uint8_t DfrMultiGasSensor::checksum7(const uint8_t *frame) {
    uint8_t sum = 0;
    for (uint8_t i = 1; i <= 7; ++i) {
        sum = static_cast<uint8_t>(sum + frame[i]);
    }
    return static_cast<uint8_t>(~sum + 1);
}

uint8_t DfrMultiGasSensor::checksum6(const uint8_t *frame) {
    uint8_t sum = 0;
    for (uint8_t i = 1; i <= 6; ++i) {
        sum = static_cast<uint8_t>(sum + frame[i]);
    }
    return static_cast<uint8_t>(~sum + 1);
}

void DfrMultiGasSensor::buildFrame(uint8_t command,
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
