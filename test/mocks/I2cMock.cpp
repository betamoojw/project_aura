#include "I2cMock.h"

#include <array>
#include <unordered_set>
#include <vector>

#include "driver/i2c.h"

struct MockI2cCmd {
    bool has_address = false;
    uint8_t addr = 0;
    std::vector<uint8_t> payload;
};

namespace {

struct DeviceState {
    bool present = false;
    std::array<uint8_t, 256> regs{};
    std::array<bool, 256> read_fail{};
    std::array<bool, 256> write_fail{};
    std::unordered_set<uint16_t> failing_cmds;
};

std::array<DeviceState, 256> g_devices{};

DeviceState &device(uint8_t addr) {
    return g_devices[addr];
}

} // namespace

namespace I2cMock {

void reset() {
    g_devices = {};
}

void setDevicePresent(uint8_t addr, bool present) {
    device(addr).present = present;
}

void setCommandFailure(uint8_t addr, uint16_t cmd, bool fail) {
    if (fail) {
        device(addr).failing_cmds.insert(cmd);
    } else {
        device(addr).failing_cmds.erase(cmd);
    }
}

void setRegister(uint8_t addr, uint8_t reg, uint8_t value) {
    device(addr).regs[reg] = value;
}

void setRegisters(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
    if (!data) {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        device(addr).regs[static_cast<uint8_t>(reg + i)] = data[i];
    }
}

void setReadFailure(uint8_t addr, uint8_t reg, bool fail) {
    device(addr).read_fail[reg] = fail;
}

void setWriteFailure(uint8_t addr, uint8_t reg, bool fail) {
    device(addr).write_fail[reg] = fail;
}

uint8_t getRegister(uint8_t addr, uint8_t reg) {
    return device(addr).regs[reg];
}

} // namespace I2cMock

i2c_cmd_handle_t i2c_cmd_link_create() {
    return new MockI2cCmd();
}

void i2c_cmd_link_delete(i2c_cmd_handle_t cmd) {
    delete cmd;
}

esp_err_t i2c_master_start(i2c_cmd_handle_t cmd) {
    return cmd ? ESP_OK : ESP_ERR_INVALID_ARG;
}

esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd) {
    return cmd ? ESP_OK : ESP_ERR_INVALID_ARG;
}

esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool) {
    if (!cmd) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!cmd->has_address) {
        cmd->has_address = true;
        cmd->addr = static_cast<uint8_t>(data >> 1);
        return ESP_OK;
    }
    cmd->payload.push_back(data);
    return ESP_OK;
}

esp_err_t i2c_master_write(i2c_cmd_handle_t cmd, const uint8_t *data, size_t data_len, bool) {
    if (!cmd || (!data && data_len != 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (data && data_len > 0) {
        cmd->payload.insert(cmd->payload.end(), data, data + data_len);
    }
    return ESP_OK;
}

esp_err_t i2c_master_cmd_begin(i2c_port_t, i2c_cmd_handle_t cmd, TickType_t) {
    if (!cmd || !cmd->has_address) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!device(cmd->addr).present) {
        return ESP_FAIL;
    }
    if (cmd->payload.size() >= 2) {
        const uint16_t sensor_cmd =
            (static_cast<uint16_t>(cmd->payload[0]) << 8) | cmd->payload[1];
        if (device(cmd->addr).failing_cmds.count(sensor_cmd) != 0) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t i2c_master_write_read_device(i2c_port_t,
                                       uint8_t addr,
                                       const uint8_t *write_buffer,
                                       size_t write_size,
                                       uint8_t *read_buffer,
                                       size_t read_size,
                                       TickType_t) {
    if (!device(addr).present || !write_buffer || write_size == 0 ||
        !read_buffer || read_size == 0) {
        return ESP_FAIL;
    }
    const uint8_t reg = write_buffer[0];
    if (device(addr).read_fail[reg]) {
        return ESP_FAIL;
    }
    for (size_t i = 0; i < read_size; ++i) {
        read_buffer[i] = device(addr).regs[static_cast<uint8_t>(reg + i)];
    }
    return ESP_OK;
}

esp_err_t i2c_master_write_to_device(i2c_port_t,
                                     uint8_t addr,
                                     const uint8_t *write_buffer,
                                     size_t write_size,
                                     TickType_t) {
    if (!device(addr).present || !write_buffer || write_size < 2) {
        return ESP_FAIL;
    }
    const uint8_t reg = write_buffer[0];
    if (device(addr).write_fail[reg]) {
        return ESP_FAIL;
    }
    for (size_t i = 1; i < write_size; ++i) {
        device(addr).regs[static_cast<uint8_t>(reg + i - 1)] = write_buffer[i];
    }
    return ESP_OK;
}

esp_err_t i2c_master_read_from_device(i2c_port_t,
                                      uint8_t addr,
                                      uint8_t *read_buffer,
                                      size_t read_size,
                                      TickType_t) {
    if (!device(addr).present || !read_buffer || read_size == 0) {
        return ESP_FAIL;
    }
    for (size_t i = 0; i < read_size; ++i) {
        read_buffer[i] = 0;
    }
    return ESP_OK;
}
