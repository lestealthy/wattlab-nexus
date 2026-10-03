#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nexus_test_engine.h"
#include "nexus_errors.h"

int test_test_engine(void) {
    printf("\n");

    // Test 1: Initialize test engine
    nexus_err_t err = nexus_test_engine_init();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_test_engine_init returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_test_engine_init\n");

    // Test 2: Load test from JSON
    const char* json = R"({
        "name": "Sensor Startup Test",
        "power": {"voltage": 5.0, "enabled": true},
        "tests": [
            {"type": "delay", "ms": 500},
            {"type": "i2c_scan", "expect": ["0x76"]},
            {"type": "uart_expect", "text": "READY", "timeout": 2000},
            {"type": "gpio_expect", "pin": "D5", "state": "HIGH", "timeout": 1000}
        ]
    })";

    nexus_test_case_t test;
    err = nexus_test_engine_load_test(json, &test);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_test_engine_load_test returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_test_engine_load_test\n");

    // Test 3: Validate test
    err = nexus_test_engine_validate(&test);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_test_engine_validate returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_test_engine_validate\n");

    // Test 4: Execute test
    nexus_test_report_t report;
    err = nexus_test_engine_execute(&test, &report);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_test_engine_execute returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_test_engine_execute\n");

    // Test 5: Check report
    if (report.result != NEXUS_TEST_PASS) {
        printf("  FAIL: test result = %d, expected PASS\n", report.result);
        return 1;
    }
    printf("  PASS: test result = PASS\n");

    // Test 6: Check steps
    if (report.steps_total != 4) {
        printf("  FAIL: steps_total = %u, expected 4\n", report.steps_total);
        return 1;
    }
    printf("  PASS: steps_total = 4\n");

    // Test 7: Get last report
    const nexus_test_report_t* last = nexus_test_engine_get_last_report();
    if (last == NULL) {
        printf("  FAIL: nexus_test_engine_get_last_report returned NULL\n");
        return 1;
    }
    printf("  PASS: nexus_test_engine_get_last_report\n");

    // Test 8: Cancel test
    err = nexus_test_engine_cancel();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_test_engine_cancel returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_test_engine_cancel\n");

    // Test 9: Check running state
    if (nexus_test_engine_is_running()) {
        printf("  FAIL: test engine still running after cancel\n");
        return 1;
    }
    printf("  PASS: test engine not running\n");

    return 0;
}
