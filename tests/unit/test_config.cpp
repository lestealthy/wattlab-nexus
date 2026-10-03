#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nexus_config.h"
#include "nexus_errors.h"

int test_config(void) {
    printf("\n");

    // Test 1: Initialize config
    nexus_err_t err = nexus_config_init();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_config_init returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_config_init\n");

    // Test 2: Get config pointer
    nexus_config_t* cfg = nexus_config_get();
    if (cfg == NULL) {
        printf("  FAIL: nexus_config_get returned NULL\n");
        return 1;
    }
    printf("  PASS: nexus_config_get\n");

    // Test 3: Validate default config
    err = nexus_config_validate(cfg);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_config_validate returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_config_validate (default)\n");

    // Test 4: Check default values
    if (cfg->display.brightness != 80) {
        printf("  FAIL: default brightness = %u, expected 80\n", cfg->display.brightness);
        return 1;
    }
    printf("  PASS: default brightness = 80\n");

    if (cfg->uart.default_uart_baud != 115200) {
        printf("  FAIL: default baud = %u, expected 115200\n", cfg->uart.default_uart_baud);
        return 1;
    }
    printf("  PASS: default baud = 115200\n");

    // Test 5: Modify config (checksum becomes stale - should fail validation)
    cfg->display.brightness = 50;
    err = nexus_config_validate(cfg);
    if (err != NEXUS_ERR_INVALID_CONFIG) {
        printf("  FAIL: validate after modify returned %d, expected %d\n", err, NEXUS_ERR_INVALID_CONFIG);
        return 1;
    }
    printf("  PASS: validate after modify correctly detects stale checksum\n");

    // Test 6: Invalid config (bad magic)
    nexus_config_t bad_cfg = *cfg;
    bad_cfg.magic = 0xDEAD;
    err = nexus_config_validate(&bad_cfg);
    if (err != NEXUS_ERR_INVALID_CONFIG) {
        printf("  FAIL: validate bad magic returned %d, expected %d\n", err, NEXUS_ERR_INVALID_CONFIG);
        return 1;
    }
    printf("  PASS: validate bad magic\n");

    // Test 7: Invalid config (bad version)
    nexus_config_t bad_ver = *cfg;
    bad_ver.version = 999;
    err = nexus_config_validate(&bad_ver);
    if (err != NEXUS_ERR_INVALID_CONFIG) {
        printf("  FAIL: validate bad version returned %d, expected %d\n", err, NEXUS_ERR_INVALID_CONFIG);
        return 1;
    }
    printf("  PASS: validate bad version\n");

    // Test 8: Reset config
    err = nexus_config_reset();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_config_reset returned %d\n", err);
        return 1;
    }
    cfg = nexus_config_get();
    if (cfg->display.brightness != 80) {
        printf("  FAIL: after reset brightness = %u, expected 80\n", cfg->display.brightness);
        return 1;
    }
    printf("  PASS: nexus_config_reset\n");

    // Test 9: Error string
    const char* err_str = nexus_err_string(NEXUS_OK);
    if (err_str == NULL || strcmp(err_str, "OK") != 0) {
        printf("  FAIL: nexus_err_string(NEXUS_OK) = %s\n", err_str ? err_str : "NULL");
        return 1;
    }
    printf("  PASS: nexus_err_string\n");

    return 0;
}
