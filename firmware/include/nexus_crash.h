#ifndef NEXUS_CRASH_H
#define NEXUS_CRASH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/*
 * Crash report architecture.
 *
 * A compact, fixed-size record is written to a reserved region (backup SRAM on
 * hardware, a plain struct on the host). It is intentionally POD so it can be
 * persisted without a filesystem dependency. Recovery is deterministic: the
 * next boot reports the crash rather than attempting to resume execution.
 *
 * IMPORTANT: the fault handler must NOT touch the filesystem. It only records
 * the compact struct. At the next boot, normal firmware reads the record and
 * (if storage is available) writes it to /NEXUS/CRASHES/.
 */

#define NEXUS_CRASH_MAGIC   0x4E584352  // "NXCR"
#define NEXUS_CRASH_VERSION 1
#define NEXUS_CRASH_TASK_LEN 12

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t valid;
    uint32_t pc;
    uint32_t lr;
    uint32_t sp;
    uint32_t xpsr;
    uint32_t cfsr;        // Configurable Fault Status Register
    uint32_t hfsr;        // HardFault Status Register
    uint32_t mmfar;       // MemManage Fault Address
    uint32_t bfar;        // BusFault Address
    uint32_t timestamp_ms;
    uint32_t reset_reason;
    uint32_t free_heap;
    char     task_name[NEXUS_CRASH_TASK_LEN];
    uint32_t checksum;
} nexus_crash_record_t;

typedef enum {
    NEXUS_RESET_UNKNOWN = 0,
    NEXUS_RESET_POWER_ON,
    NEXUS_RESET_PIN,
    NEXUS_RESET_SOFTWARE,
    NEXUS_RESET_IWDG,
    NEXUS_RESET_WWDG,
    NEXUS_RESET_LOW_POWER,
    NEXUS_RESET_BROWNOUT,
} nexus_reset_reason_t;

nexus_err_t nexus_crash_init(void);

// Record a crash. On hardware this may be called from the fault handler, so it
// must not allocate or block.
void nexus_crash_record(const char* task_name, uint32_t reset_reason);

// Returns true if a valid crash record exists from a previous boot.
bool nexus_crash_has_record(void);

const nexus_crash_record_t* nexus_crash_get_record(void);

// Clear the stored record (called after the user acknowledges it).
nexus_err_t nexus_crash_clear(void);

// Compact human-readable rendering, e.g. for the crash screen.
nexus_err_t nexus_crash_format(char* buffer, size_t buffer_size);

// Machine-readable (JSON) rendering.
nexus_err_t nexus_crash_format_json(char* buffer, size_t buffer_size);

// Human-readable reset reason.
const char* nexus_crash_reset_reason_string(uint32_t reason);

// Platform hook: return the actual reset reason from hardware flags.
// Default implementation returns NEXUS_RESET_UNKNOWN.
__attribute__((weak)) uint32_t nexus_crash_read_reset_reason(void);

#endif
