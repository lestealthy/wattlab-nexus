#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "virtual_dut.h"
#include "nexus_device_db.h"
#include "nexus_test_engine.h"
#include "nexus_storage.h"
#include "nexus_storage_host.h"
#include "nexus_time.h"
#include "nexus_logger.h"
#include "nexus_log.h"
#include "nexus_persist.h"
#include "nexus_version.h"

// Simulated RTC and monotonic clock for the host simulator.
static uint32_t g_mono = 0;
static uint32_t sim_mono(void) { return g_mono; }
static nexus_datetime_t g_rtc_dt = {2026, 10, 3, 1, 25, 30, 0};
static bool sim_rtc_read(nexus_datetime_t* out) { *out = g_rtc_dt; return true; }
static bool sim_rtc_write(const nexus_datetime_t* in) { g_rtc_dt = *in; return true; }
static bool sim_rtc_present(void) { return true; }
static nexus_rtc_backend_t g_rtc = { sim_rtc_read, sim_rtc_write, sim_rtc_present };

int main(void) {
    printf("========================================\n");
    printf("WattLab Nexus - Host Simulator\n");
    printf("========================================\n\n");

    // --- Alpha subsystems: time, storage, logger ---
    const char* base = getenv("TEMP");
    if (!base) base = ".";
    char root[256];
    snprintf(root, sizeof(root), "%s/nexus_simulator", base);
    nexus_storage_host_set_root(root);

    nexus_log_init();
    nexus_time_init();
    nexus_time_set_rtc(&g_rtc);
    nexus_time_set_monotonic(sim_mono);
    nexus_time_sync_from_rtc();

    nexus_storage_init();
    nexus_storage_select(nexus_storage_host_backend());
    nexus_storage_unmount();
    if (nexus_storage_mount() != NEXUS_OK) {
        printf("Failed to mount storage\n");
        return 1;
    }

    nexus_logger_config_t log_cfg;
    nexus_logger_default_config(&log_cfg);
    nexus_logger_init(&log_cfg);
    nexus_log_set_sink(nexus_logger_sink);

    printf("RTC:     %s\n", nexus_time_is_valid() ? "VALID" : "INVALID");
    printf("Storage: %s\n", nexus_storage_state_string(nexus_storage_state()));
    nexus_storage_info_t info;
    if (nexus_storage_get_info(&info) == NEXUS_OK) {
        printf("Capacity: %llu MB, Free: %llu MB\n",
               (unsigned long long)(info.total_bytes / (1024 * 1024)),
               (unsigned long long)(info.free_bytes / (1024 * 1024)));
    }

    // Initialize Nexus subsystems
    nexus_device_db_init();
    nexus_test_engine_init();

    // Initialize virtual DUT
    vdut_context_t vdut;
    if (vdut_init(&vdut) != 0) {
        printf("Failed to initialize virtual DUT\n");
        return 1;
    }

    // Add virtual devices
    printf("\nCreating virtual DUT...\n");

    // I2C: BME280 at 0x76
    vdut_add_i2c_device(&vdut, 0x76, 0xD0, 0x60);
    printf("  Added I2C device: BME280 at 0x76\n");

    // I2C: MPU6050 at 0x68
    vdut_add_i2c_device(&vdut, 0x68, 0x75, 0x68);
    printf("  Added I2C device: MPU6050 at 0x68\n");

    // UART: 115200 baud, sends "READY\r\n"
    vdut_add_uart_device(&vdut, 115200, "READY\r\n");
    printf("  Added UART device: 115200 baud\n");

    // GPIO: D5 heartbeat at 1Hz
    vdut_add_gpio_device(&vdut, 5, 1, 0.5f);
    printf("  Added GPIO device: D5 heartbeat 1Hz\n");

    // Power on
    vdut_power_on(&vdut, 3.3f);
    printf("  Power: ON (3.3V, 183mA)\n");

    printf("\n");

    // Simulate discovery
    printf("Running discovery...\n");

    // I2C scan
    uint8_t addresses[16];
    uint32_t found = 0;
    vdut_scan_i2c(&vdut, addresses, 16, &found);
    printf("  I2C scan found %u devices:\n", found);
    for (uint32_t i = 0; i < found; i++) {
        uint8_t val;
        uint8_t identity_reg = vdut_get_identity_reg(&vdut, addresses[i]);
        vdut_read_i2c(&vdut, addresses[i], identity_reg, &val);
        const nexus_device_info_t* dev = nexus_device_db_identify(NEXUS_BUS_I2C, addresses[i], identity_reg, val);
        if (dev) {
            printf("    0x%02X: %s\n", addresses[i], dev->name);
            NEXUS_LOG_INFO("DISCOVERY", "Found %s at 0x%02X", dev->name, addresses[i]);
        } else {
            printf("    0x%02X: Unknown (reg 0x%02X = 0x%02X)\n", addresses[i], identity_reg, val);
        }
    }

    // UART check
    uint8_t uart_data[32];
    uint32_t uart_received = 0;
    if (vdut_uart_rx(&vdut, uart_data, sizeof(uart_data), &uart_received) == 0) {
        printf("  UART RX: %.*s\n", (int)uart_received, uart_data);
    }

    // GPIO check
    uint8_t gpio_state;
    if (vdut_gpio_read(&vdut, 5, &gpio_state) == 0) {
        printf("  GPIO D5: %s\n", gpio_state ? "HIGH" : "LOW");
    }

    printf("\n");

    // Save a demo capture.
    printf("Saving capture...\n");
    uint8_t samples[512];
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
    if (nexus_persist_capture(&meta, samples, sizeof(samples), cap_dir, sizeof(cap_dir)) == NEXUS_OK) {
        printf("  Saved: %s (%zu bytes)\n", cap_dir, sizeof(samples));
    } else {
        printf("  Capture save FAILED\n");
    }

    // Run a test
    printf("\nRunning test...\n");

    const char* test_json = R"({
        "name": "Virtual DUT Test",
        "power": {"voltage": 3.3, "enabled": true},
        "tests": [
            {"type": "delay", "ms": 100},
            {"type": "i2c_scan", "expect": ["0x76", "0x68"]},
            {"type": "uart_expect", "text": "READY", "timeout": 1000}
        ]
    })";

    nexus_test_case_t test;
    nexus_test_engine_load_test(test_json, &test);
    nexus_test_engine_validate(&test);

    nexus_test_report_t report;
    nexus_test_engine_execute(&test, &report);

    printf("  Test result: %s\n", report.result == NEXUS_TEST_PASS ? "PASS" : "FAIL");
    printf("  Steps: %u passed, %u failed, %u skipped\n",
        report.steps_passed, report.steps_failed, report.steps_skipped);
    printf("  Duration: %u ms\n", report.duration_ms);

    // Save the report.
    char rjson[NEXUS_STORAGE_PATH_MAX], rtxt[NEXUS_STORAGE_PATH_MAX];
    if (nexus_persist_test_report("Virtual DUT Test", (int)report.result, report.duration_ms,
            report.steps_passed, report.steps_failed, report.steps_skipped, report.steps_total,
            rjson, sizeof(rjson), rtxt, sizeof(rtxt)) == NEXUS_OK) {
        printf("  Report saved: %s\n", rjson);
    }

    // Flush logs and show the tree.
    g_mono += 5000;
    nexus_time_update(g_mono);
    nexus_logger_flush(g_mono);
    printf("\nStorage tree:\n");
    nexus_file_entry_t entries[16];
    uint32_t count = 0;
    if (nexus_storage_list("/NEXUS/LOGS", entries, 16, &count) == NEXUS_OK) {
        for (uint32_t i = 0; i < count; i++) printf("  /NEXUS/LOGS/%s (%u bytes)\n", entries[i].name, entries[i].size);
    }
    if (nexus_storage_list("/NEXUS/CAPTURES", entries, 16, &count) == NEXUS_OK) {
        for (uint32_t i = 0; i < count; i++) printf("  /NEXUS/CAPTURES/%s\n", entries[i].name);
    }
    if (nexus_storage_list("/NEXUS/REPORTS", entries, 16, &count) == NEXUS_OK) {
        for (uint32_t i = 0; i < count; i++) printf("  /NEXUS/REPORTS/%s\n", entries[i].name);
    }

    printf("\n");

    // Clean shutdown
    nexus_storage_eject();

    // Cleanup
    vdut_power_off(&vdut);
    vdut_cleanup(&vdut);

    printf("Simulator complete.\n");
    return 0;
}
