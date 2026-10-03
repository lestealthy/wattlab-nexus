#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nexus_storage.h"
#include "nexus_storage_host.h"
#include "nexus_time.h"
#include "nexus_logger.h"
#include "nexus_log.h"
#include "nexus_persist.h"
#include "nexus_device_db.h"
#include "nexus_test_engine.h"
#include "nexus_errors.h"

/*
 * End-to-end Alpha flow on the host, exercising the real storage/time/logger/
 * persistence code paths against a real temporary directory:
 *
 *   boot -> time valid -> storage mounted -> logger -> discovery ->
 *   test executed -> capture saved -> report saved -> logs persisted
 */

static char g_root[256];
static uint32_t g_mono = 0;
static uint32_t fake_mono(void) { return g_mono; }

static bool g_rtc_ok = true;
static nexus_datetime_t g_rtc_dt = {2026, 10, 3, 1, 25, 30, 0};
static bool rtc_read_impl(nexus_datetime_t* out) { if (!g_rtc_ok) return false; *out = g_rtc_dt; return true; }
static bool rtc_write_impl(const nexus_datetime_t* in) { g_rtc_dt = *in; return true; }
static bool rtc_present_impl(void) { return g_rtc_ok; }
static nexus_rtc_backend_t g_rtc = { rtc_read_impl, rtc_write_impl, rtc_present_impl };

static int boot_subsystems(void) {
    const char* base = getenv("TEMP");
    if (!base) base = ".";
    snprintf(g_root, sizeof(g_root), "%s/nexus_alpha_e2e", base);

    nexus_storage_host_set_root(g_root);
    nexus_storage_host_inject_full(false);
    nexus_storage_host_inject_write_failure(false);
    nexus_storage_host_inject_mount_failure(false);

    nexus_log_init();
    nexus_time_init();
    nexus_time_set_rtc(&g_rtc);
    nexus_time_set_monotonic(fake_mono);
    nexus_time_sync_from_rtc();

    nexus_storage_init();
    nexus_storage_select(nexus_storage_host_backend());
    nexus_storage_unmount();

    nexus_device_db_init();
    nexus_test_engine_init();
    return 0;
}

int test_alpha_flow(void) {
    printf("\n");
    boot_subsystems();

    // --- time ---
    if (!nexus_time_is_valid()) { printf("  FAIL: time invalid at boot\n"); return 1; }
    nexus_datetime_t now;
    nexus_time_now(&now);
    if (now.year != 2026 || now.hour != 1) { printf("  FAIL: rtc time not applied\n"); return 1; }
    printf("  PASS: RTC time valid at boot\n");

    // --- storage ---
    if (nexus_storage_mount() != NEXUS_OK) { printf("  FAIL: mount\n"); return 1; }
    if (!nexus_storage_is_ready()) { printf("  FAIL: storage not ready\n"); return 1; }
    printf("  PASS: storage mounted with layout\n");

    // --- logger wired to storage ---
    nexus_logger_config_t cfg;
    nexus_logger_default_config(&cfg);
    nexus_logger_init(&cfg);
    nexus_log_set_sink(nexus_logger_sink);

    NEXUS_LOG_INFO("SYSTEM", "Boot complete");
    NEXUS_LOG_INFO("STORAGE", "Mounted");

    // --- discovery ---
    const nexus_device_info_t* dev = nexus_device_db_identify(NEXUS_BUS_I2C, 0x76, 0xD0, 0x60);
    if (!dev) { printf("  FAIL: discovery\n"); return 1; }
    NEXUS_LOG_INFO("DISCOVERY", "Found %s", dev->name);
    printf("  PASS: virtual DUT discovered (%s)\n", dev->name);

    // --- capture persistence ---
    uint8_t samples[256];
    for (size_t i = 0; i < sizeof(samples); i++) samples[i] = (uint8_t)i;

    nexus_capture_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.source, "UART3", sizeof(meta.source) - 1);
    strncpy(meta.protocol, "UART", sizeof(meta.protocol) - 1);
    strncpy(meta.trigger, "RX", sizeof(meta.trigger) - 1);
    meta.baud = 115200;
    meta.sample_rate = 1000000;
    meta.sample_count = sizeof(samples);
    meta.duration_ms = 2310;
    meta.wall_valid = true;

    char cap_dir[NEXUS_STORAGE_PATH_MAX];
    if (nexus_persist_capture(&meta, samples, sizeof(samples), cap_dir, sizeof(cap_dir)) != NEXUS_OK) {
        printf("  FAIL: capture persist\n"); return 1;
    }
    char meta_path[NEXUS_STORAGE_PATH_MAX];
    char data_path[NEXUS_STORAGE_PATH_MAX];
    snprintf(meta_path, sizeof(meta_path), "%s/meta.json", cap_dir);
    snprintf(data_path, sizeof(data_path), "%s/data.bin", cap_dir);
    if (!nexus_storage_exists(meta_path) || !nexus_storage_exists(data_path)) {
        printf("  FAIL: capture files missing\n"); return 1;
    }
    // Verify data integrity.
    uint8_t readback[256];
    size_t rn = 0;
    nexus_storage_read_file(data_path, readback, sizeof(readback), &rn);
    if (rn != sizeof(samples) || memcmp(readback, samples, rn) != 0) {
        printf("  FAIL: capture data mismatch\n"); return 1;
    }
    printf("  PASS: capture saved with metadata + data\n");

    // --- test engine + report persistence ---
    const char* test_json = R"({
        "name": "Alpha Boot Test",
        "power": {"voltage": 3.3, "enabled": true},
        "tests": [
            {"type": "delay", "ms": 50},
            {"type": "i2c_scan", "expect": ["0x76"]},
            {"type": "uart_expect", "text": "READY", "timeout": 1000}
        ]
    })";
    nexus_test_case_t tc;
    if (nexus_test_engine_load_test(test_json, &tc) != NEXUS_OK) { printf("  FAIL: load test\n"); return 1; }
    if (nexus_test_engine_validate(&tc) != NEXUS_OK) { printf("  FAIL: validate test\n"); return 1; }
    nexus_test_report_t report;
    if (nexus_test_engine_execute(&tc, &report) != NEXUS_OK) { printf("  FAIL: execute test\n"); return 1; }
    if (report.result != NEXUS_TEST_PASS) { printf("  FAIL: test result\n"); return 1; }

    char rjson[NEXUS_STORAGE_PATH_MAX], rtxt[NEXUS_STORAGE_PATH_MAX];
    if (nexus_persist_test_report("Alpha Boot Test", (int)report.result, report.duration_ms,
                                  report.steps_passed, report.steps_failed, report.steps_skipped,
                                  report.steps_total, rjson, sizeof(rjson), rtxt, sizeof(rtxt)) != NEXUS_OK) {
        printf("  FAIL: report persist\n"); return 1;
    }
    if (!nexus_storage_exists(rjson) || !nexus_storage_exists(rtxt)) {
        printf("  FAIL: report files missing\n"); return 1;
    }
    printf("  PASS: test executed and report saved\n");

    // --- logs persisted ---
    if (nexus_logger_flush(g_mono) != NEXUS_OK) { printf("  FAIL: log flush\n"); return 1; }
    if (!nexus_storage_exists("/NEXUS/LOGS/2026-10-03.log")) {
        printf("  FAIL: log file missing\n"); return 1;
    }
    uint8_t logbuf[2048];
    size_t ln = 0;
    nexus_storage_read_file("/NEXUS/LOGS/2026-10-03.log", logbuf, sizeof(logbuf), &ln);
    logbuf[ln < sizeof(logbuf) ? ln : sizeof(logbuf) - 1] = '\0';
    if (strstr((char*)logbuf, "Boot complete") == NULL ||
        strstr((char*)logbuf, "Found Bosch BME280") == NULL) {
        printf("  FAIL: log contents incomplete\n"); return 1;
    }
    printf("  PASS: logs persisted with timestamps\n");

    // --- clean shutdown ---
    if (nexus_storage_eject() != NEXUS_OK) { printf("  FAIL: eject\n"); return 1; }
    if (nexus_storage_is_ready()) { printf("  FAIL: still ready after eject\n"); return 1; }
    printf("  PASS: clean eject\n");

    nexus_log_set_output(NEXUS_LOG_OUTPUT_SERIAL);
    nexus_log_set_sink(NULL);
    return 0;
}
