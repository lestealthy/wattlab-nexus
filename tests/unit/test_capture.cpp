#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nexus_capture.h"
#include "nexus_errors.h"

int test_capture(void) {
    printf("\n");

    // Test 1: Initialize capture engine
    nexus_err_t err = nexus_capture_init();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_capture_init returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_capture_init\n");

    // Test 2: Configure channel
    nexus_capture_config_t config = {
        .type = NEXUS_CAPTURE_DIGITAL,
        .channel = 0,
        .sample_rate_hz = 10000,
        .pre_trigger_samples = 100,
        .post_trigger_samples = 100,
        .trigger_type = NEXUS_TRIG_RISING,
        .trigger_channel = 0,
        .trigger_threshold = 512,
        .armed = false,
    };

    err = nexus_capture_configure_channel(0, &config);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_capture_configure_channel returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_capture_configure_channel\n");

    // Test 3: Arm channel
    err = nexus_capture_arm(0);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_capture_arm returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_capture_arm\n");

    // Test 4: Store samples
    for (int i = 0; i < 50; i++) {
        nexus_capture_sample_t sample = {
            .timestamp_ms = (uint32_t)i,
            .value = (uint16_t)(i * 10),
            .flags = 0,
        };
        err = nexus_capture_store_sample(0, &sample);
        if (err != NEXUS_OK) {
            printf("  FAIL: nexus_capture_store_sample(%d) returned %d\n", i, err);
            return 1;
        }
    }
    printf("  PASS: nexus_capture_store_sample (50 samples)\n");

    // Test 5: Get samples
    nexus_capture_sample_t out[10];
    uint32_t out_count = 0;
    err = nexus_capture_get_samples(0, out, 10, &out_count);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_capture_get_samples returned %d\n", err);
        return 1;
    }
    if (out_count != 10) {
        printf("  FAIL: out_count = %u, expected 10\n", out_count);
        return 1;
    }
    printf("  PASS: nexus_capture_get_samples\n");

    // Test 6: Check trigger (should not be triggered yet - no rising edge past threshold)
    bool triggered = nexus_capture_is_triggered(0);
    printf("  INFO: triggered = %s\n", triggered ? "true" : "false");

    // Test 7: Store more samples to trigger
    for (int i = 50; i < 150; i++) {
        nexus_capture_sample_t sample = {
            .timestamp_ms = (uint32_t)i,
            .value = (uint16_t)(i * 10),
            .flags = 0,
        };
        err = nexus_capture_store_sample(0, &sample);
        if (err != NEXUS_OK) {
            printf("  FAIL: nexus_capture_store_sample(%d) returned %d\n", i, err);
            return 1;
        }
    }
    printf("  PASS: nexus_capture_store_sample (100 more)\n");

    // Test 8: Disarm channel
    err = nexus_capture_disarm(0);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_capture_disarm returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_capture_disarm\n");

    // Test 9: Clear channel
    err = nexus_capture_clear(0);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_capture_clear returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_capture_clear\n");

    // Test 10: Invalid channel
    err = nexus_capture_arm(255);
    if (err != NEXUS_ERR_INVALID_PARAM) {
        printf("  FAIL: nexus_capture_arm(255) returned %d, expected %d\n", err, NEXUS_ERR_INVALID_PARAM);
        return 1;
    }
    printf("  PASS: nexus_capture_arm invalid channel\n");

    return 0;
}
