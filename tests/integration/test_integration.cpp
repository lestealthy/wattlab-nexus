#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nexus_config.h"
#include "nexus_errors.h"
#include "nexus_log.h"
#include "nexus_resource.h"
#include "nexus_capture.h"
#include "nexus_device_db.h"
#include "nexus_test_engine.h"

int test_integration(void) {
    printf("\n");

    // Integration test: Full system initialization
    printf("  Testing full system initialization...\n");

    nexus_err_t err;

    // Initialize all modules
    nexus_log_init();

    err = nexus_config_init();
    if (err != NEXUS_OK) { printf("  FAIL: config_init\n"); return 1; }

    err = nexus_resource_init();
    if (err != NEXUS_OK) { printf("  FAIL: resource_init\n"); return 1; }

    err = nexus_capture_init();
    if (err != NEXUS_OK) { printf("  FAIL: capture_init\n"); return 1; }

    err = nexus_device_db_init();
    if (err != NEXUS_OK) { printf("  FAIL: device_db_init\n"); return 1; }

    err = nexus_test_engine_init();
    if (err != NEXUS_OK) { printf("  FAIL: test_engine_init\n"); return 1; }

    printf("  PASS: All modules initialized\n");

    // Integration test: Device discovery simulation
    printf("  Testing device discovery simulation...\n");

    // Simulate I2C scan finding BME280
    const nexus_device_info_t* dev = nexus_device_db_identify(NEXUS_BUS_I2C, 0x76, 0xD0, 0x60);
    if (dev == NULL) {
        printf("  FAIL: BME280 not identified\n");
        return 1;
    }
    printf("  PASS: BME280 identified at 0x76\n");

    // Integration test: Capture with trigger
    printf("  Testing capture with trigger...\n");

    nexus_capture_config_t cap_config = {
        .type = NEXUS_CAPTURE_ANALOG,
        .channel = 0,
        .sample_rate_hz = 1000,
        .pre_trigger_samples = 50,
        .post_trigger_samples = 50,
        .trigger_type = NEXUS_TRIG_RISING,
        .trigger_channel = 0,
        .trigger_threshold = 2048,
        .armed = false,
    };

    err = nexus_capture_configure_channel(0, &cap_config);
    if (err != NEXUS_OK) { printf("  FAIL: capture configure\n"); return 1; }

    err = nexus_capture_arm(0);
    if (err != NEXUS_OK) { printf("  FAIL: capture arm\n"); return 1; }

    // Generate samples with a rising edge
    for (int i = 0; i < 100; i++) {
        nexus_capture_sample_t sample = {
            .timestamp_ms = (uint32_t)i,
            .value = (i < 50) ? (uint16_t)(i * 10) : (uint16_t)(500 + (i - 50) * 30),
            .flags = 0,
        };
        nexus_capture_store_sample(0, &sample);
    }

    bool triggered = nexus_capture_is_triggered(0);
    printf("  INFO: Capture triggered = %s\n", triggered ? "true" : "false");

    // Integration test: Test engine with device
    printf("  Testing test engine with device...\n");

    const char* test_json = R"({
        "name": "BME280 Startup Test",
        "power": {"voltage": 3.3, "enabled": true},
        "tests": [
            {"type": "delay", "ms": 100},
            {"type": "i2c_scan", "expect": ["0x76"]},
            {"type": "uart_expect", "text": "READY", "timeout": 1000}
        ]
    })";

    nexus_test_case_t test;
    err = nexus_test_engine_load_test(test_json, &test);
    if (err != NEXUS_OK) { printf("  FAIL: load test\n"); return 1; }

    err = nexus_test_engine_validate(&test);
    if (err != NEXUS_OK) { printf("  FAIL: validate test\n"); return 1; }

    nexus_test_report_t report;
    err = nexus_test_engine_execute(&test, &report);
    if (err != NEXUS_OK) { printf("  FAIL: execute test\n"); return 1; }

    if (report.result != NEXUS_TEST_PASS) {
        printf("  FAIL: test result = %d\n", report.result);
        return 1;
    }
    printf("  PASS: Test engine execution\n");

    // Integration test: Resource management with protocols
    printf("  Testing resource management...\n");

    err = nexus_resource_acquire(1, NEXUS_RES_TYPE_I2C, "DiscoveryEngine", NULL);
    if (err != NEXUS_OK) { printf("  FAIL: acquire I2C\n"); return 1; }

    err = nexus_resource_acquire(2, NEXUS_RES_TYPE_UART, "Terminal", NULL);
    if (err != NEXUS_OK) { printf("  FAIL: acquire UART\n"); return 1; }

    if (!nexus_resource_is_available(3, NEXUS_RES_TYPE_SPI)) {
        printf("  FAIL: SPI should be available\n");
        return 1;
    }

    nexus_resource_release(1, NEXUS_RES_TYPE_I2C, "DiscoveryEngine");
    nexus_resource_release(2, NEXUS_RES_TYPE_UART, "Terminal");

    printf("  PASS: Resource management\n");

    printf("\n  All integration tests passed!\n");
    return 0;
}
