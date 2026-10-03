#include "nexus_logger.h"
#include "nexus_storage.h"
#include "nexus_time.h"
#include <string.h>
#include <stdio.h>

// Monotonic clock provider. On the device this is millis(); on the host the
// test harness injects a value through the record's uptime field. We keep a
// weak default so the module links on both.
static uint32_t s_uptime_hint = 0;

static nexus_logger_config_t s_cfg;
static nexus_logger_record_t s_buf[NEXUS_LOGGER_BUFFER_CAP];
static uint16_t s_head = 0;
static uint16_t s_tail = 0;
static uint16_t s_count = 0;
static bool s_initialized = false;

static nexus_logger_stats_t s_stats;
static uint32_t s_rotating_index = 1;
static uint32_t s_current_bytes = 0;

void nexus_logger_default_config(nexus_logger_config_t* out) {
    if (!out) return;
    out->jsonl = false;
    out->max_file_bytes = 256 * 1024; // 256 KB per file
    out->max_files = 8;               // 8 files retained -> ~2 MB ceiling
    out->flush_interval_ms = 1000;
    out->buffer_records = NEXUS_LOGGER_BUFFER_CAP;
}

static void build_filename(char* out, size_t out_size) {
    // Date-based when wall time is valid, else rotating index.
    nexus_datetime_t dt;
    if (nexus_time_is_valid() && nexus_time_now(&dt) == NEXUS_OK) {
        char date[16];
        if (nexus_time_format_date(&dt, date, sizeof(date)) == NEXUS_OK) {
            snprintf(out, out_size, "%s/LOGS/%s.%s", NEXUS_STORAGE_ROOT, date,
                     s_cfg.jsonl ? "jsonl" : "log");
            return;
        }
    }
    snprintf(out, out_size, "%s/LOGS/LOG_%04u.%s", NEXUS_STORAGE_ROOT,
             (unsigned)s_rotating_index, s_cfg.jsonl ? "jsonl" : "log");
}

nexus_err_t nexus_logger_init(const nexus_logger_config_t* config) {
    if (config) s_cfg = *config;
    else nexus_logger_default_config(&s_cfg);

    if (s_cfg.buffer_records == 0 || s_cfg.buffer_records > NEXUS_LOGGER_BUFFER_CAP) {
        s_cfg.buffer_records = NEXUS_LOGGER_BUFFER_CAP;
    }
    if (s_cfg.max_files == 0) s_cfg.max_files = 1;

    s_head = s_tail = s_count = 0;
    memset(&s_stats, 0, sizeof(s_stats));
    s_rotating_index = 1;
    s_current_bytes = 0;
    s_initialized = true;
    s_stats.current_file[0] = '\0';
    return NEXUS_OK;
}

// Decide whether an incoming record may displace a buffered one.
static bool level_is_critical(nexus_log_level_t l) {
    return l >= NEXUS_LOG_WARN;
}

static void buffer_push(const nexus_logger_record_t* rec) {
    if (s_count >= s_cfg.buffer_records) {
        // Buffer full: apply the documented drop policy.
        if (!level_is_critical(rec->level)) {
            s_stats.dropped++;
            return; // drop DEBUG/INFO under pressure
        }
        // Critical record: drop the oldest non-critical if any exists.
        bool displaced = false;
        for (uint16_t i = 0; i < s_count; i++) {
            uint16_t idx = (uint16_t)((s_tail + i) % NEXUS_LOGGER_BUFFER_CAP);
            if (!level_is_critical(s_buf[idx].level)) {
                // Remove this element by shifting the tail forward is complex;
                // simplest correct action: overwrite it in place.
                s_buf[idx] = *rec;
                displaced = true;
                break;
            }
        }
        if (!displaced) {
            // All critical: overwrite the oldest and count a drop.
            s_buf[s_tail] = *rec;
            s_tail = (uint16_t)((s_tail + 1) % NEXUS_LOGGER_BUFFER_CAP);
            s_stats.dropped++;
        }
        return;
    }

    s_buf[s_head] = *rec;
    s_head = (uint16_t)((s_head + 1) % NEXUS_LOGGER_BUFFER_CAP);
    s_count++;
}

void nexus_logger_sink(nexus_log_level_t level, const char* module, const char* message) {
    if (!s_initialized) return;

    nexus_logger_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.level = level;
    strncpy(rec.module, module ? module : "?", sizeof(rec.module) - 1);
    strncpy(rec.message, message ? message : "", sizeof(rec.message) - 1);
    rec.uptime_ms = s_uptime_hint;

    nexus_datetime_t dt;
    if (nexus_time_is_valid() && nexus_time_now(&dt) == NEXUS_OK) {
        rec.wall_valid = true;
        rec.wall_ms = nexus_time_unix_ms();
    }

    buffer_push(&rec);
}

nexus_err_t nexus_logger_format_record(const nexus_logger_record_t* rec, nexus_logger_format_t fmt,
                                       char* out, size_t out_size) {
    if (!rec || !out || out_size == 0) return NEXUS_ERR_INVALID_PARAM;

    char ts[32];
    if (rec->wall_valid) {
        nexus_datetime_t dt;
        nexus_time_from_unix_seconds((int64_t)(rec->wall_ms / 1000u), &dt);
        dt.ms = (uint16_t)(rec->wall_ms % 1000u);
        nexus_time_format_iso8601(&dt, ts, sizeof(ts));
    } else {
        // No wall clock: emit monotonic uptime instead of a fabricated date.
        snprintf(ts, sizeof(ts), "T+%u.%03u", (unsigned)(rec->uptime_ms / 1000u),
                 (unsigned)(rec->uptime_ms % 1000u));
    }

    if (fmt == NEXUS_LOG_FMT_JSONL) {
        // Minimal JSON string escaping for the two string fields.
        char esc_mod[64];
        char esc_msg[260];
        size_t j = 0;
        for (size_t i = 0; rec->module[i] && j + 2 < sizeof(esc_mod); i++) {
            char c = rec->module[i];
            if (c == '"' || c == '\\') esc_mod[j++] = '\\';
            esc_mod[j++] = c;
        }
        esc_mod[j] = '\0';
        j = 0;
        for (size_t i = 0; rec->message[i] && j + 2 < sizeof(esc_msg); i++) {
            char c = rec->message[i];
            if (c == '"' || c == '\\') esc_msg[j++] = '\\';
            else if (c == '\n') { esc_msg[j++] = '\\'; c = 'n'; }
            else if (c == '\r') { esc_msg[j++] = '\\'; c = 'r'; }
            else if (c == '\t') { esc_msg[j++] = '\\'; c = 't'; }
            esc_msg[j++] = c;
        }
        esc_msg[j] = '\0';

        snprintf(out, out_size,
                 "{\"ts\":\"%s\",\"level\":\"%s\",\"module\":\"%s\",\"message\":\"%s\"}",
                 ts, nexus_log_level_name(rec->level), esc_mod, esc_msg);
        return NEXUS_OK;
    }

    snprintf(out, out_size, "%s [%s] %s %s",
             ts, nexus_log_level_name(rec->level), rec->module, rec->message);
    return NEXUS_OK;
}

// Rotate if the current file would exceed the limit. Keeps a bounded number of
// files; older files are removed.
static void maybe_rotate(const char* current, size_t upcoming) {
    if (s_current_bytes + upcoming <= s_cfg.max_file_bytes) return;

    s_rotating_index++;
    s_stats.rotations++;
    s_current_bytes = 0;

    // Remove the oldest file once we exceed the retention count.
    uint32_t oldest = (s_rotating_index > s_cfg.max_files)
                    ? (s_rotating_index - s_cfg.max_files) : 0;
    if (oldest > 0) {
        char path[96];
        snprintf(path, sizeof(path), "%s/LOGS/LOG_%04u.%s", NEXUS_STORAGE_ROOT,
                 (unsigned)oldest, s_cfg.jsonl ? "jsonl" : "log");
        nexus_storage_remove(path);
    }
    (void)current;
}

nexus_err_t nexus_logger_flush(uint32_t now_ms) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_uptime_hint = now_ms;

    if (s_count == 0) return NEXUS_OK;
    if (!nexus_storage_is_ready()) {
        // Storage unavailable: keep buffering. Never lose records silently to
        // a transient condition; the buffer policy handles overrun.
        return NEXUS_ERR_STORAGE;
    }

    // Drain the queue.
    uint32_t written_this_flush = 0;
    while (s_count > 0) {
        nexus_logger_record_t* rec = &s_buf[s_tail];
        char line[NEXUS_LOGGER_MSG_MAX + 128];
        if (nexus_logger_format_record(rec, s_cfg.jsonl ? NEXUS_LOG_FMT_JSONL : NEXUS_LOG_FMT_TXT,
                                       line, sizeof(line)) != NEXUS_OK) {
            // Formatting failed; drop this record rather than stall.
            s_tail = (uint16_t)((s_tail + 1) % NEXUS_LOGGER_BUFFER_CAP);
            s_count--;
            s_stats.dropped++;
            continue;
        }

        size_t line_len = strlen(line);
        maybe_rotate(s_stats.current_file, line_len + 1);

        char path[96];
        build_filename(path, sizeof(path));
        strncpy(s_stats.current_file, path, sizeof(s_stats.current_file) - 1);

        // Append the line plus newline.
        char out[NEXUS_LOGGER_MSG_MAX + 132];
        int n = snprintf(out, sizeof(out), "%s\n", line);
        if (n < 0) n = 0;

        nexus_err_t err = nexus_storage_append_file(path, (const uint8_t*)out, (size_t)n);
        if (err != NEXUS_OK) {
            // Stop flushing; retry on the next call. Record stays buffered.
            s_stats.dropped++; // counted as a transient failure, not a loss yet
            return err;
        }

        s_current_bytes += (uint32_t)n;
        s_stats.written++;
        written_this_flush++;

        s_tail = (uint16_t)((s_tail + 1) % NEXUS_LOGGER_BUFFER_CAP);
        s_count--;
    }
    (void)written_this_flush;
    return NEXUS_OK;
}

uint32_t nexus_logger_pending(void) { return s_count; }

void nexus_logger_get_stats(nexus_logger_stats_t* out) {
    if (out) *out = s_stats;
}
