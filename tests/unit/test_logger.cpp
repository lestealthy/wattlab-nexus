#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nexus_logger.h"
#include "nexus_log.h"
#include "nexus_storage.h"
#include "nexus_storage_host.h"
#include "nexus_time.h"
#include "nexus_errors.h"

static char g_root[256];
static uint32_t g_mono = 0;
static uint32_t fake_mono(void) { return g_mono; }

static void setup_storage(void) {
    const char* base = getenv("TEMP");
    if (!base) base = ".";
    snprintf(g_root, sizeof(g_root), "%s/nexus_logger_test", base);
    nexus_storage_host_set_root(g_root);
    nexus_storage_host_inject_full(false);
    nexus_storage_host_inject_write_failure(false);
    nexus_storage_host_inject_mount_failure(false);
    nexus_storage_init();
    nexus_storage_select(nexus_storage_host_backend());
    nexus_storage_unmount();
    nexus_storage_mount();
}

static int test_format_txt(void) {
    nexus_logger_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.level = NEXUS_LOG_INFO;
    strncpy(rec.module, "SYSTEM", sizeof(rec.module) - 1);
    strncpy(rec.message, "Boot complete", sizeof(rec.message) - 1);
    rec.wall_valid = true;
    // 2026-10-03T01:23:42.381Z
    nexus_datetime_t dt = {2026, 10, 3, 1, 23, 42, 0};
    rec.wall_ms = (uint64_t)nexus_time_to_unix_seconds(&dt) * 1000ull + 381;

    char line[256];
    if (nexus_logger_format_record(&rec, NEXUS_LOG_FMT_TXT, line, sizeof(line)) != NEXUS_OK) {
        printf("  FAIL: format txt\n"); return 1;
    }
    if (strstr(line, "2026-10-03T01:23:42.381Z") == NULL) { printf("  FAIL: ts missing: %s\n", line); return 1; }
    if (strstr(line, "[INFO]") == NULL) { printf("  FAIL: level missing\n"); return 1; }
    if (strstr(line, "SYSTEM Boot complete") == NULL) { printf("  FAIL: body missing\n"); return 1; }
    printf("  PASS: TXT format with wall clock\n");
    return 0;
}

static int test_format_invalid_time(void) {
    nexus_logger_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.level = NEXUS_LOG_WARN;
    strncpy(rec.module, "RTC", sizeof(rec.module) - 1);
    strncpy(rec.message, "Time invalid", sizeof(rec.message) - 1);
    rec.wall_valid = false;
    rec.uptime_ms = 5000;

    char line[256];
    nexus_logger_format_record(&rec, NEXUS_LOG_FMT_TXT, line, sizeof(line));
    // Must NOT fabricate a date.
    if (strstr(line, "1970") != NULL || strstr(line, "2026") != NULL) {
        printf("  FAIL: fabricated date: %s\n", line); return 1;
    }
    if (strstr(line, "T+5.000") == NULL) { printf("  FAIL: no uptime fallback: %s\n", line); return 1; }
    printf("  PASS: invalid time uses monotonic uptime\n");
    return 0;
}

static int test_format_jsonl(void) {
    nexus_logger_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.level = NEXUS_LOG_ERROR;
    strncpy(rec.module, "SD", sizeof(rec.module) - 1);
    strncpy(rec.message, "write \"failed\"", sizeof(rec.message) - 1);
    rec.wall_valid = true;
    nexus_datetime_t dt = {2026, 10, 3, 1, 23, 42, 0};
    rec.wall_ms = (uint64_t)nexus_time_to_unix_seconds(&dt) * 1000ull;

    char line[320];
    nexus_logger_format_record(&rec, NEXUS_LOG_FMT_JSONL, line, sizeof(line));
    if (strstr(line, "{\"ts\":") == NULL) { printf("  FAIL: jsonl prefix: %s\n", line); return 1; }
    if (strstr(line, "\"level\":\"ERROR\"") == NULL) { printf("  FAIL: jsonl level\n"); return 1; }
    // Quotes in the message must be escaped.
    if (strstr(line, "\\\"failed\\\"") == NULL) { printf("  FAIL: jsonl escaping: %s\n", line); return 1; }
    printf("  PASS: JSONL format with escaping\n");
    return 0;
}

static int test_flush_and_read(void) {
    setup_storage();
    nexus_time_init();
    nexus_time_set_monotonic(fake_mono);
    nexus_datetime_t dt = {2026, 10, 3, 1, 23, 42, 0};
    nexus_time_set_datetime(&dt);

    nexus_logger_config_t cfg;
    nexus_logger_default_config(&cfg);
    cfg.buffer_records = 32;
    nexus_logger_init(&cfg);
    nexus_log_set_sink(nexus_logger_sink);
    nexus_log_set_output(NEXUS_LOG_OUTPUT_NONE); // sink captures; serial silenced

    NEXUS_LOG_INFO("SYSTEM", "first record");
    NEXUS_LOG_WARN("SD", "second record");
    NEXUS_LOG_ERROR("TEST", "third record");

    if (nexus_logger_pending() != 3) { printf("  FAIL: pending=%u\n", nexus_logger_pending()); return 1; }

    if (nexus_logger_flush(1000) != NEXUS_OK) { printf("  FAIL: flush\n"); return 1; }
    if (nexus_logger_pending() != 0) { printf("  FAIL: not drained\n"); return 1; }

    // Read the dated log file back.
    if (!nexus_storage_exists("/NEXUS/LOGS/2026-10-03.log")) {
        printf("  FAIL: log file missing\n"); return 1;
    }
    uint8_t buf[1024];
    size_t n = 0;
    nexus_storage_read_file("/NEXUS/LOGS/2026-10-03.log", buf, sizeof(buf), &n);
    buf[n < sizeof(buf) ? n : sizeof(buf) - 1] = '\0';
    if (strstr((char*)buf, "first record") == NULL ||
        strstr((char*)buf, "third record") == NULL) {
        printf("  FAIL: log contents: %s\n", buf); return 1;
    }
    printf("  PASS: flush writes timestamped log and reads back\n");
    return 0;
}

static int test_priority_drop(void) {
    setup_storage();
    nexus_time_init();
    nexus_time_set_monotonic(fake_mono);
    nexus_datetime_t dt = {2026, 10, 3, 1, 23, 42, 0};
    nexus_time_set_datetime(&dt);

    nexus_logger_config_t cfg;
    nexus_logger_default_config(&cfg);
    cfg.buffer_records = 4; // tiny buffer
    nexus_logger_init(&cfg);
    nexus_log_set_sink(nexus_logger_sink);
    nexus_log_set_output(NEXUS_LOG_OUTPUT_NONE);
    nexus_log_set_level(NEXUS_LOG_DEBUG); // allow DEBUG through to exercise drops

    // Overflow with DEBUG records - they should be dropped, not crash.
    for (int i = 0; i < 20; i++) {
        NEXUS_LOG_DEBUG("BULK", "debug %d", i);
    }
    if (nexus_logger_pending() > 4) { printf("  FAIL: buffer over cap\n"); return 1; }

    // Now push critical records - they must be retained.
    NEXUS_LOG_ERROR("CRIT", "important-1");
    NEXUS_LOG_ERROR("CRIT", "important-2");

    nexus_logger_flush(2000);
    uint8_t buf[2048];
    size_t n = 0;
    nexus_storage_read_file("/NEXUS/LOGS/2026-10-03.log", buf, sizeof(buf), &n);
    buf[n < sizeof(buf) ? n : sizeof(buf) - 1] = '\0';
    if (strstr((char*)buf, "important-2") == NULL) {
        printf("  FAIL: critical record lost: %s\n", buf); return 1;
    }
    nexus_logger_stats_t st;
    nexus_logger_get_stats(&st);
    if (st.dropped == 0) { printf("  FAIL: expected drops reported\n"); return 1; }
    printf("  PASS: priority drop policy (dropped=%u)\n", st.dropped);
    return 0;
}

static int test_storage_failure(void) {
    setup_storage();
    nexus_time_init();
    nexus_time_set_monotonic(fake_mono);
    nexus_datetime_t dt = {2026, 10, 3, 1, 23, 42, 0};
    nexus_time_set_datetime(&dt);

    nexus_logger_config_t cfg;
    nexus_logger_default_config(&cfg);
    nexus_logger_init(&cfg);
    nexus_log_set_sink(nexus_logger_sink);
    nexus_log_set_output(NEXUS_LOG_OUTPUT_NONE);

    NEXUS_LOG_INFO("SYSTEM", "held during failure");
    nexus_storage_host_inject_write_failure(true);
    nexus_err_t e = nexus_logger_flush(3000);
    if (e == NEXUS_OK) { printf("  FAIL: flush should fail\n"); return 1; }
    // Record must remain buffered for retry, not vanish.
    if (nexus_logger_pending() == 0) { printf("  FAIL: record lost on failure\n"); return 1; }

    nexus_storage_host_inject_write_failure(false);
    if (nexus_logger_flush(4000) != NEXUS_OK) { printf("  FAIL: recovery flush\n"); return 1; }
    if (nexus_logger_pending() != 0) { printf("  FAIL: not drained after recovery\n"); return 1; }
    printf("  PASS: storage failure retains records, recovers\n");
    return 0;
}

static int test_rotation(void) {
    setup_storage();
    nexus_time_init();
    nexus_time_set_monotonic(fake_mono);
    // Use invalid wall time so rotation falls back to indexed files.
    nexus_logger_config_t cfg;
    nexus_logger_default_config(&cfg);
    cfg.max_file_bytes = 64;   // tiny to force rotation
    cfg.max_files = 3;
    nexus_logger_init(&cfg);
    nexus_log_set_sink(nexus_logger_sink);
    nexus_log_set_output(NEXUS_LOG_OUTPUT_NONE);

    char big[120];
    memset(big, 'A', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';

    for (int i = 0; i < 10; i++) {
        NEXUS_LOG_INFO("ROT", "%s", big);
        nexus_logger_flush((uint32_t)(5000 + i));
    }
    nexus_logger_stats_t st;
    nexus_logger_get_stats(&st);
    if (st.rotations == 0) { printf("  FAIL: no rotations\n"); return 1; }
    printf("  PASS: rotation (rotations=%u)\n", st.rotations);
    return 0;
}

int test_logger(void) {
    printf("\n");
    if (test_format_txt()) return 1;
    if (test_format_invalid_time()) return 1;
    if (test_format_jsonl()) return 1;
    if (test_flush_and_read()) return 1;
    if (test_priority_drop()) return 1;
    if (test_storage_failure()) return 1;
    if (test_rotation()) return 1;
    // Restore default outputs for following suites.
    nexus_log_set_output(NEXUS_LOG_OUTPUT_SERIAL);
    nexus_log_set_level(NEXUS_LOG_INFO);
    nexus_log_set_sink(NULL);
    return 0;
}
