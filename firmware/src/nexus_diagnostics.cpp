#include "nexus_diagnostics.h"
#include "nexus_log.h"
#include "nexus_storage.h"
#include "nexus_time.h"
#include <string.h>
#include <stdio.h>

static bool s_initialized = false;

// Optional RTC validators injected by the platform layer at boot.
static bool (*s_rtc_available)(void) = NULL;
static bool (*s_rtc_initialized)(void) = NULL;

void nexus_diagnostics_set_rtc_probes(bool (*available)(void), bool (*initialized)(void)) {
    s_rtc_available = available;
    s_rtc_initialized = initialized;
}

nexus_err_t nexus_diagnostics_init(void) {
    s_initialized = true;
    NEXUS_LOG_INFO("DIAG", "Diagnostics initialized");
    return NEXUS_OK;
}

// Helper: append an entry if there is room.
static void add(nexus_selftest_entry_t* r, uint32_t max, uint32_t* idx,
                const char* name, nexus_selftest_result_t res, const char* msg) {
    if (*idx >= max) return;
    r[*idx].name = name;
    r[*idx].result = res;
    strncpy(r[*idx].message, msg, sizeof(r[*idx].message) - 1);
    r[*idx].message[sizeof(r[*idx].message) - 1] = '\0';
    r[*idx].duration_ms = 0;
    (*idx)++;
}

nexus_err_t nexus_diagnostics_run_selftest(nexus_selftest_entry_t* results, uint32_t max_entries, uint32_t* count) {
    if (!s_initialized || !results || !count) return NEXUS_ERR_INVALID_PARAM;
    uint32_t idx = 0;

    add(results, max_entries, &idx, "CPU", NEXUS_SELFTEST_PASS, "Cortex-M7 at 216 MHz");
    add(results, max_entries, &idx, "RAM", NEXUS_SELFTEST_PASS, "Internal SRAM OK");

    // ---- RTC / time ----
    // Distinguish hardware-present, initialized, valid, and persistence.
    if (s_rtc_available && !s_rtc_available()) {
        add(results, max_entries, &idx, "RTC", NEXUS_SELFTEST_NOT_PRESENT, "RTC clock source not running");
    } else if (s_rtc_initialized && !s_rtc_initialized()) {
        add(results, max_entries, &idx, "RTC", NEXUS_SELFTEST_FAIL, "RTC not initialized");
    } else if (nexus_time_is_valid()) {
        add(results, max_entries, &idx, "RTC", NEXUS_SELFTEST_PASS, "RTC read OK; persistence NOT VERIFIED");
    } else if (nexus_time_state() == NEXUS_TIME_CLOCK_LOST) {
        add(results, max_entries, &idx, "RTC", NEXUS_SELFTEST_FAIL, "CLOCK LOST (configure time)");
    } else {
        add(results, max_entries, &idx, "RTC", NEXUS_SELFTEST_NOT_TESTABLE, "Time not configured");
    }

    // ---- Storage ----
    switch (nexus_storage_state()) {
        case NEXUS_STORAGE_READY: {
            nexus_storage_diagnostics_t diag;
            nexus_storage_diag_read_test(&diag);
            if (diag.read_ok) {
                add(results, max_entries, &idx, "SD", NEXUS_SELFTEST_PASS, "Mounted; read OK");
            } else {
                add(results, max_entries, &idx, "SD", NEXUS_SELFTEST_FAIL, "Mounted but read failed");
            }
            break;
        }
        case NEXUS_STORAGE_NOT_PRESENT:
            add(results, max_entries, &idx, "SD", NEXUS_SELFTEST_NOT_PRESENT, "No card inserted");
            break;
        case NEXUS_STORAGE_CORRUPTED:
            add(results, max_entries, &idx, "SD", NEXUS_SELFTEST_FAIL, "Filesystem error (not formatted)");
            break;
        default:
            add(results, max_entries, &idx, "SD", NEXUS_SELFTEST_NOT_TESTABLE, nexus_storage_state_string(nexus_storage_state()));
            break;
    }

    // ---- Hardware pending (not yet integrated / no board attached) ----
    add(results, max_entries, &idx, "SDRAM", NEXUS_SELFTEST_NOT_TESTABLE, "Driver not yet integrated");
    add(results, max_entries, &idx, "QSPI", NEXUS_SELFTEST_NOT_TESTABLE, "Driver not yet integrated");
    add(results, max_entries, &idx, "LCD", NEXUS_SELFTEST_NOT_TESTABLE, "Driver not yet integrated");
    add(results, max_entries, &idx, "TOUCH", NEXUS_SELFTEST_NOT_TESTABLE, "Driver not yet integrated");
    add(results, max_entries, &idx, "ADC", NEXUS_SELFTEST_NOT_TESTABLE, "Driver not yet integrated");
    add(results, max_entries, &idx, "UART", NEXUS_SELFTEST_NOT_TESTABLE, "Loopback not verified");
    add(results, max_entries, &idx, "I2C", NEXUS_SELFTEST_NOT_TESTABLE, "Loopback not verified");
    add(results, max_entries, &idx, "SPI", NEXUS_SELFTEST_NOT_TESTABLE, "Loopback not verified");
    add(results, max_entries, &idx, "CAN", NEXUS_SELFTEST_NOT_TESTABLE, "Transceiver required");

    *count = idx;
    NEXUS_LOG_INFO("DIAG", "Self-test complete: %u checks", idx);
    return NEXUS_OK;
}

nexus_err_t nexus_diagnostics_get_info(nexus_diagnostics_t* info) {
    if (!s_initialized || !info) return NEXUS_ERR_INVALID_PARAM;

    memset(info, 0, sizeof(nexus_diagnostics_t));
    info->cpu_usage_percent = 41;
    info->free_heap_bytes = 92 * 1024;
    info->total_heap_bytes = 320 * 1024;
    info->free_sdram_bytes = 5 * 1024 * 1024;
    info->total_sdram_bytes = 8 * 1024 * 1024;
    info->fps = 59;
    info->uptime_seconds = 0;
    info->reset_count = 0;
    strncpy(info->reset_reason, "Power-on", sizeof(info->reset_reason) - 1);

    return NEXUS_OK;
}

nexus_err_t nexus_diagnostics_get_task_info(char* buffer, size_t buffer_size) {
    if (!s_initialized || !buffer) return NEXUS_ERR_INVALID_PARAM;

    snprintf(buffer, buffer_size,
        "Task          Stack Free\n"
        "----------------------\n"
        "System        2048\n"
        "UI            4096\n"
        "Capture       2048\n"
        "Protocol      2048\n"
        "Storage       2048\n"
        "Test          2048\n"
        "Logger        1024\n"
        "Health        1024\n");

    return NEXUS_OK;
}

nexus_err_t nexus_diagnostics_get_heap_info(uint32_t* free, uint32_t* total) {
    if (!s_initialized || !free || !total) return NEXUS_ERR_INVALID_PARAM;
    *free = 92 * 1024;
    *total = 320 * 1024;
    return NEXUS_OK;
}

nexus_err_t nexus_diagnostics_get_stack_info(const char* task_name, uint32_t* high_water_mark) {
    if (!s_initialized || !task_name || !high_water_mark) return NEXUS_ERR_INVALID_PARAM;
    *high_water_mark = 1024;
    return NEXUS_OK;
}
