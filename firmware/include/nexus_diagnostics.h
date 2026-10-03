#ifndef NEXUS_DIAGNOSTICS_H
#define NEXUS_DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

typedef enum {
    NEXUS_SELFTEST_PASS = 0,
    NEXUS_SELFTEST_FAIL,
    NEXUS_SELFTEST_SKIP,
    NEXUS_SELFTEST_NOT_TESTABLE,
    NEXUS_SELFTEST_NOT_PRESENT,
} nexus_selftest_result_t;

typedef struct {
    const char* name;
    nexus_selftest_result_t result;
    char message[64];
    uint32_t duration_ms;
} nexus_selftest_entry_t;

typedef struct {
    uint32_t cpu_usage_percent;
    uint32_t free_heap_bytes;
    uint32_t total_heap_bytes;
    uint32_t free_sdram_bytes;
    uint32_t total_sdram_bytes;
    uint32_t fps;
    uint32_t uptime_seconds;
    uint32_t reset_count;
    char reset_reason[32];
} nexus_diagnostics_t;

nexus_err_t nexus_diagnostics_init(void);
nexus_err_t nexus_diagnostics_run_selftest(nexus_selftest_entry_t* results, uint32_t max_entries, uint32_t* count);

// Optional platform probes so the self-test can distinguish RTC states.
void nexus_diagnostics_set_rtc_probes(bool (*available)(void), bool (*initialized)(void));
nexus_err_t nexus_diagnostics_get_info(nexus_diagnostics_t* info);
nexus_err_t nexus_diagnostics_get_task_info(char* buffer, size_t buffer_size);
nexus_err_t nexus_diagnostics_get_heap_info(uint32_t* free, uint32_t* total);
nexus_err_t nexus_diagnostics_get_stack_info(const char* task_name, uint32_t* high_water_mark);

#endif
