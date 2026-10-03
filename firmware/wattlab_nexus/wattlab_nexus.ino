/*
 * WattLab Nexus - Universal Embedded Engineering Workstation
 * Main firmware entry point
 *
 * Target: STM32F746G-DISCO (STM32F746NGH6)
 * Core:   STM32duino
 * RTOS:   FreeRTOS
 *
 * Alpha boot order (see docs/ALPHA_TEST_PLAN.md):
 *   CPU -> memory -> FreeRTOS prerequisites -> board resources ->
 *   RTC/TimeService -> storage (SD) -> config -> device DB -> logger ->
 *   UI -> analyzers -> test engine -> READY
 */

#include <Arduino.h>
#include "nexus_version.h"
#include "nexus_errors.h"
#include "nexus_log.h"
#include "nexus_config.h"
#include "nexus_board.h"
#include "nexus_resource.h"
#include "nexus_capture.h"
#include "nexus_device_db.h"
#include "nexus_test_engine.h"
#include "nexus_discovery.h"
#include "nexus_calibration.h"
#include "nexus_power.h"
#include "nexus_storage.h"
#include "nexus_storage_sd.h"
#include "nexus_diagnostics.h"
#include "nexus_measurement.h"
#include "nexus_ui.h"
#include "nexus_crash.h"
#include "nexus_time.h"
#include "nexus_logger.h"
#include "nexus_persist.h"
#include "nexus_boot.h"
#include "nexus_platform.h"

// FreeRTOS includes
#include <STM32FreeRTOS.h>

// Task handles
static TaskHandle_t s_system_task_handle = NULL;
static TaskHandle_t s_ui_task_handle = NULL;
static TaskHandle_t s_capture_task_handle = NULL;
static TaskHandle_t s_protocol_task_handle = NULL;
static TaskHandle_t s_storage_task_handle = NULL;
static TaskHandle_t s_test_task_handle = NULL;
static TaskHandle_t s_logger_task_handle = NULL;
static TaskHandle_t s_health_task_handle = NULL;

// System state
static volatile bool s_system_ready = false;
static volatile uint32_t s_boot_time = 0;
static volatile uint32_t s_last_reset_reason = NEXUS_RESET_UNKNOWN;

// Task functions
static void system_task(void* param);
static void ui_task(void* param);
static void capture_task(void* param);
static void protocol_task(void* param);
static void storage_task(void* param);
static void test_task(void* param);
static void logger_task(void* param);
static void health_task(void* param);

void setup() {
    // Serial first so we can log the boot sequence.
    Serial.begin(115200);
    delay(100);

    // --- logging level/output is decided before anything else logs ---
    nexus_log_init();

    NEXUS_LOG_INFO("SYSTEM", "WattLab Nexus v%s starting...", NEXUS_VERSION_STRING);
    NEXUS_LOG_INFO("SYSTEM", "Build: %s %s", NEXUS_BUILD_DATE, NEXUS_BUILD_TIME);

    // --- core, host-testable modules ---
    nexus_crash_init();
    nexus_config_init();
    nexus_resource_init();
    nexus_capture_init();
    nexus_device_db_init();
    nexus_test_engine_init();
    nexus_discovery_init();
    nexus_calibration_init();
    nexus_power_init();
    nexus_diagnostics_init();
    nexus_measurement_init();
    nexus_ui_init();
    nexus_time_init();

    // --- platform hardware: reset reason + RTC/TimeService ---
    // nexus_platform_init() configures the reset reason, RTC and TimeService
    // and returns the reset reason (it clears the hardware flags internally).
    s_last_reset_reason = nexus_platform_init();

    // --- storage: select the SD backend, mount, create layout ---
    nexus_storage_init();
    nexus_storage_select(nexus_storage_sd_backend());
    nexus_storage_mount();

    // --- persistent logger (buffer, then flush from the logger/storage task) ---
    nexus_logger_config_t log_cfg;
    nexus_logger_default_config(&log_cfg);
    // Follow the configured level/output once config is loaded from SD.
    nexus_logger_init(&log_cfg);
    nexus_log_set_sink(nexus_logger_sink);

    // Log the boot banner into the persistent log.
    NEXUS_LOG_INFO("SYSTEM", "Reset reason: %s",
                   nexus_crash_reset_reason_string(s_last_reset_reason));
    NEXUS_LOG_INFO("TIME", "State: %s",
                   nexus_time_is_valid() ? "VALID" : "INVALID");
    NEXUS_LOG_INFO("STORAGE", "SD: %s", nexus_storage_state_string(nexus_storage_state()));

    // --- persist a crash from a previous run, now that storage may be ready ---
    if (nexus_crash_has_record()) {
        char crash_buf[256];
        nexus_crash_format(crash_buf, sizeof(crash_buf));
        NEXUS_LOG_WARN("SYSTEM", "Previous crash detected:\n%s", crash_buf);
        nexus_boot_persist_crash();
    }

    // --- boot self-test ---
    {
        nexus_selftest_entry_t results[24];
        uint32_t count = 0;
        nexus_diagnostics_run_selftest(results, 24, &count);
        for (uint32_t i = 0; i < count; i++) {
            const char* r = "?";
            switch (results[i].result) {
                case NEXUS_SELFTEST_PASS:         r = "PASS"; break;
                case NEXUS_SELFTEST_FAIL:         r = "FAIL"; break;
                case NEXUS_SELFTEST_SKIP:         r = "SKIP"; break;
                case NEXUS_SELFTEST_NOT_TESTABLE: r = "NOT TESTABLE"; break;
                case NEXUS_SELFTEST_NOT_PRESENT:  r = "NOT PRESENT"; break;
            }
            NEXUS_LOG_INFO("SELFTEST", "%-10s %-12s %s", results[i].name, r, results[i].message);
        }
    }

    NEXUS_LOG_INFO("SYSTEM", "Core modules initialized");

    // --- FreeRTOS tasks. Priorities reflect data path importance:
    // Health(highest) > Capture > Protocol/UI > System/Test > Storage/Logger.
    xTaskCreate(system_task, "System", 4096, NULL, 2, &s_system_task_handle);
    xTaskCreate(ui_task, "UI", 8192, NULL, 3, &s_ui_task_handle);
    xTaskCreate(capture_task, "Capture", 4096, NULL, 4, &s_capture_task_handle);
    xTaskCreate(protocol_task, "Protocol", 4096, NULL, 3, &s_protocol_task_handle);
    xTaskCreate(storage_task, "Storage", 4096, NULL, 1, &s_storage_task_handle);
    xTaskCreate(test_task, "Test", 4096, NULL, 2, &s_test_task_handle);
    xTaskCreate(logger_task, "Logger", 2048, NULL, 1, &s_logger_task_handle);
    xTaskCreate(health_task, "Health", 2048, NULL, 5, &s_health_task_handle);

    NEXUS_LOG_INFO("SYSTEM", "FreeRTOS tasks created");
    NEXUS_LOG_INFO("SYSTEM", "Boot complete");

    vTaskStartScheduler();

    NEXUS_LOG_FATAL("SYSTEM", "Scheduler failed to start!");
    while (1) {}
}

void loop() {
    // FreeRTOS scheduler handles everything; loop() is unused.
    delay(1000);
}

static void system_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("SYSTEM", "System task started");

    s_boot_time = millis();
    pinMode(NexusPins::NEXUS_LED, OUTPUT);
    digitalWrite(NexusPins::NEXUS_LED, LOW);

    s_system_ready = true;
    NEXUS_LOG_INFO("SYSTEM", "System ready");

    while (1) {
        // Advance the wall clock from the monotonic millisecond counter.
        nexus_time_update(millis());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void ui_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("UI", "UI task started");
    while (!s_system_ready) vTaskDelay(pdMS_TO_TICKS(10));
    NEXUS_LOG_INFO("UI", "UI ready");
    while (1) {
        nexus_ui_update();
        vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS
    }
}

static void capture_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("CAPTURE", "Capture task started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void protocol_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("PROTOCOL", "Protocol task started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// Storage task: drains queued writes (logs first). This is the ONLY place
// that performs slow SD writes, keeping UI/capture/protocol tasks responsive.
static void storage_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("STORAGE", "Storage task started");
    while (1) {
        if (nexus_storage_is_ready()) {
            nexus_logger_flush(millis());
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // flush interval
    }
}

static void test_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("TEST", "Test task started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void logger_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("LOGGER", "Logger task started");
    while (1) {
        // Periodic flush in case the storage task is busy.
        if (nexus_storage_is_ready()) nexus_logger_flush(millis());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void health_task(void* param) {
    (void)param;
    NEXUS_LOG_INFO("HEALTH", "Health task started");
    while (1) {
        digitalWrite(NexusPins::NEXUS_LED, !digitalRead(NexusPins::NEXUS_LED));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
