// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/SafeRestart.h"

#include <Arduino.h>
#include <esp_cpu.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>

#include "core/Logger.h"

namespace {

constexpr uint32_t kCore0RestartTaskStackBytes = 4096;
constexpr uint32_t kCore0RestartTaskStackWords =
    kCore0RestartTaskStackBytes / sizeof(StackType_t);
constexpr UBaseType_t kCore0RestartTaskPriority = configMAX_PRIORITIES - 1;
TaskHandle_t core0_restart_task_handle = nullptr;

#if (configSUPPORT_STATIC_ALLOCATION == 1)
StaticTask_t core0_restart_task_tcb;
StackType_t core0_restart_task_stack[kCore0RestartTaskStackWords];
#endif

void core0_restart_task(void *) {
    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_restart();
    }
}

[[noreturn]] void hard_restart_fallback() {
    esp_restart();
    abort();
    while (true) {}
}

} // namespace

bool safe_restart_init() {
    if (core0_restart_task_handle != nullptr) {
        return true;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    core0_restart_task_handle = xTaskCreateStaticPinnedToCore(core0_restart_task,
                                                               "restart_core0",
                                                               kCore0RestartTaskStackWords,
                                                               nullptr,
                                                               kCore0RestartTaskPriority,
                                                               core0_restart_task_stack,
                                                               &core0_restart_task_tcb,
                                                               0);
    if (core0_restart_task_handle == nullptr) {
        LOGE("Restart", "failed to create static Core0 restart task");
        return false;
    }
#else
    TaskHandle_t created = nullptr;
    const BaseType_t ok = xTaskCreatePinnedToCore(core0_restart_task,
                                                   "restart_core0",
                                                   kCore0RestartTaskStackBytes,
                                                   nullptr,
                                                   kCore0RestartTaskPriority,
                                                   &created,
                                                   0);
    if (ok != pdPASS || created == nullptr) {
        LOGE("Restart", "failed to create Core0 restart task");
        return false;
    }
    core0_restart_task_handle = created;
#endif

    return true;
}

[[noreturn]] void safe_restart_via_core0() {
    if (esp_cpu_get_core_id() == 0) {
        hard_restart_fallback();
    }

    if (!safe_restart_init()) {
        LOGE("Restart", "Core0 restart task unavailable");
        abort();
        while (true) {}
    }

    const BaseType_t notify_ok = xTaskNotifyGive(core0_restart_task_handle);
    if (notify_ok != pdPASS) {
        LOGE("Restart", "failed to notify Core0 restart task");
        abort();
        while (true) {}
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
