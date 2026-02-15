// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/BoardInit.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_display_panel.hpp>

#include "config/AppConfig.h"
#include "core/BootHelpers.h"
#include "lvgl_v8_port.h"
#include "core/Logger.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

namespace {

Board *g_board_ptr = nullptr;
volatile bool g_board_begin_done = false;
volatile bool g_board_begin_success = false;
TaskHandle_t g_board_begin_task = nullptr;
constexpr uint8_t BOARD_BEGIN_MAX_ATTEMPTS = 3;
constexpr uint32_t BOARD_BEGIN_RETRY_DELAY_MS = 300;
constexpr uint32_t BOARD_BEGIN_WAIT_TIMEOUT_MS = 10000;

enum class BoardBeginResult : uint8_t {
    Success = 0,
    Failed = 1,
    Timeout = 2,
};

void board_begin_task(void *) {
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    LOGI("Main", "[Core %d] Starting board->begin()...", xPortGetCoreID());
    g_board_begin_success = g_board_ptr->begin();
    if (!g_board_begin_success) {
        LOGE("Main", "Board begin failed!");
    }
    g_board_begin_done = true;
    if (g_board_begin_task == self) {
        g_board_begin_task = nullptr;
    }
    vTaskDelete(nullptr);
}

BoardBeginResult run_board_begin_once(Board *board) {
    g_board_ptr = board;
    g_board_begin_done = false;
    g_board_begin_success = false;
    g_board_begin_task = nullptr;

    BaseType_t created = xTaskCreatePinnedToCore(
        board_begin_task,
        "board_init",
        8192,
        nullptr,
        1,
        &g_board_begin_task,
        0  // Core 0
    );
    if (created != pdPASS) {
        LOGE("Main", "Failed to create board_init task");
        return BoardBeginResult::Failed;
    }

    const TickType_t timeout_ticks = pdMS_TO_TICKS(BOARD_BEGIN_WAIT_TIMEOUT_MS);
    const TickType_t start_ticks = xTaskGetTickCount();
    while (!g_board_begin_done) {
        if ((xTaskGetTickCount() - start_ticks) >= timeout_ticks) {
            LOGE("Main", "board->begin() timeout");
            if (g_board_begin_task != nullptr) {
                vTaskDelete(g_board_begin_task);
                g_board_begin_task = nullptr;
            }
            g_board_begin_done = true;
            g_board_begin_success = false;
            return BoardBeginResult::Timeout;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return g_board_begin_success ? BoardBeginResult::Success : BoardBeginResult::Failed;
}

} // namespace

esp_panel::board::Board *BoardInit::initBoard() {
    LOGI("Main", "Initializing board");
    Board *board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif

    // Run board->begin() on Core 0 to avoid IPC stack overflow.
    LOGI("Main", "Running on core: %d", xPortGetCoreID());
    bool timeout_seen = false;
    for (uint8_t attempt = 1; attempt <= BOARD_BEGIN_MAX_ATTEMPTS; ++attempt) {
        const BoardBeginResult result = run_board_begin_once(board);
        if (result == BoardBeginResult::Success) {
            return board;
        }

        if (result == BoardBeginResult::Timeout) {
            timeout_seen = true;
            LOGE("Main", "board->begin attempt %u timed out, abort retries",
                 static_cast<unsigned>(attempt));
            break;
        }

        LOGW("Main", "board->begin attempt %u/%u failed",
             static_cast<unsigned>(attempt),
             static_cast<unsigned>(BOARD_BEGIN_MAX_ATTEMPTS));
        if (attempt < BOARD_BEGIN_MAX_ATTEMPTS) {
            BootHelpers::recoverI2CBus(static_cast<gpio_num_t>(Config::I2C_SDA_PIN),
                                       static_cast<gpio_num_t>(Config::I2C_SCL_PIN));
            vTaskDelay(pdMS_TO_TICKS(BOARD_BEGIN_RETRY_DELAY_MS));
        }
    }

    if (g_board_begin_task != nullptr) {
        vTaskDelete(g_board_begin_task);
        g_board_begin_task = nullptr;
    }

    if (timeout_seen) {
        LOGE("Main", "Board init failed (timeout)");
    } else {
        LOGE("Main", "Board init failed after retries");
    }
    delete board;
    return nullptr;
}
