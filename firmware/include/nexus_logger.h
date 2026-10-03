#ifndef NEXUS_LOGGER_H
#define NEXUS_LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"
#include "nexus_log.h"

/*
 * Persistent logger.
 *
 * NEXUS_LOG_* calls are forwarded here via nexus_log_set_sink(). Records are
 * buffered in RAM and written to storage by a single consumer (the storage
 * task on device, the test harness on host). No real-time caller performs I/O.
 *
 * Drop policy when the buffer is full (documented, deterministic):
 *   - TRACE/DEBUG are dropped first.
 *   - INFO is dropped next.
 *   - WARN/ERROR/FATAL are retained; if the buffer is full of WARN+ records,
 *     the oldest WARN record is overwritten only as a last resort.
 *
 * Output formats:
 *   - TXT  : human readable line-per-record (default)
 *   - JSONL: one JSON object per line (machine readable, no giant array)
 */

#define NEXUS_LOGGER_MODULE_MAX  16
#define NEXUS_LOGGER_MSG_MAX     192

typedef enum {
    NEXUS_LOG_FMT_TXT = 0,
    NEXUS_LOG_FMT_JSONL,
} nexus_logger_format_t;

typedef struct {
    bool jsonl;
    uint32_t max_file_bytes;   // rotate when a file would exceed this
    uint8_t  max_files;        // retained log files
    uint32_t flush_interval_ms;
    uint16_t buffer_records;   // must be <= NEXUS_LOGGER_BUFFER_CAP
} nexus_logger_config_t;

typedef struct {
    nexus_log_level_t level;
    char module[NEXUS_LOGGER_MODULE_MAX];
    char message[NEXUS_LOGGER_MSG_MAX];
    uint32_t uptime_ms;    // monotonic, always available
    uint64_t wall_ms;      // 0 when time invalid
    bool wall_valid;
} nexus_logger_record_t;

#define NEXUS_LOGGER_BUFFER_CAP 64

void nexus_logger_default_config(nexus_logger_config_t* out);

nexus_err_t nexus_logger_init(const nexus_logger_config_t* config);

// Sink entry point - register with nexus_log_set_sink(nexus_logger_sink).
void nexus_logger_sink(nexus_log_level_t level, const char* module, const char* message);

// Consumer: flush buffered records to the active storage backend.
nexus_err_t nexus_logger_flush(uint32_t now_ms);

// Number of records currently buffered.
uint32_t nexus_logger_pending(void);

// Statistics (for diagnostics / tests).
typedef struct {
    uint64_t written;
    uint32_t dropped;
    uint32_t rotations;
    char current_file[64];
} nexus_logger_stats_t;
void nexus_logger_get_stats(nexus_logger_stats_t* out);

// Build the log line for a record (exposed for tests).
nexus_err_t nexus_logger_format_record(const nexus_logger_record_t* rec, nexus_logger_format_t fmt,
                                       char* out, size_t out_size);

#endif
