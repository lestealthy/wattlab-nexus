#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nexus_resource.h"
#include "nexus_errors.h"

int test_resource(void) {
    printf("\n");

    // Test 1: Initialize resource manager
    nexus_err_t err = nexus_resource_init();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_resource_init returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_resource_init\n");

    // Test 2: Acquire resource
    err = nexus_resource_acquire(1, NEXUS_RES_TYPE_GPIO, "TestModule", NULL);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_resource_acquire returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_resource_acquire\n");

    // Test 3: Check resource is not available
    if (nexus_resource_is_available(1, NEXUS_RES_TYPE_GPIO)) {
        printf("  FAIL: resource still available after acquire\n");
        return 1;
    }
    printf("  PASS: resource not available after acquire\n");

    // Test 4: Check owner
    const char* owner = nexus_resource_owner(1, NEXUS_RES_TYPE_GPIO);
    if (owner == NULL || strcmp(owner, "TestModule") != 0) {
        printf("  FAIL: owner = %s\n", owner ? owner : "NULL");
        return 1;
    }
    printf("  PASS: resource owner = TestModule\n");

    // Test 5: Query resource
    nexus_resource_t res;
    err = nexus_resource_query(1, NEXUS_RES_TYPE_GPIO, &res);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_resource_query returned %d\n", err);
        return 1;
    }
    if (res.state != NEXUS_RES_ACQUIRED) {
        printf("  FAIL: resource state = %d, expected ACQUIRED\n", res.state);
        return 1;
    }
    printf("  PASS: nexus_resource_query\n");

    // Test 6: Release resource
    err = nexus_resource_release(1, NEXUS_RES_TYPE_GPIO, "TestModule");
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_resource_release returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_resource_release\n");

    // Test 7: Check resource is available again
    if (!nexus_resource_is_available(1, NEXUS_RES_TYPE_GPIO)) {
        printf("  FAIL: resource not available after release\n");
        return 1;
    }
    printf("  PASS: resource available after release\n");

    // Test 8: Release non-existent resource
    err = nexus_resource_release(999, NEXUS_RES_TYPE_GPIO, "TestModule");
    if (err != NEXUS_ERR_NOT_FOUND) {
        printf("  FAIL: release non-existent returned %d, expected %d\n", err, NEXUS_ERR_NOT_FOUND);
        return 1;
    }
    printf("  PASS: release non-existent resource\n");

    // Test 9: Acquire multiple resources
    for (int i = 0; i < 5; i++) {
        err = nexus_resource_acquire(i, NEXUS_RES_TYPE_UART, "MultiTest", NULL);
        if (err != NEXUS_OK) {
            printf("  FAIL: acquire %d returned %d\n", i, err);
            return 1;
        }
    }
    printf("  PASS: acquire multiple resources\n");

    // Test 10: Release with wrong owner
    err = nexus_resource_release(0, NEXUS_RES_TYPE_UART, "WrongOwner");
    if (err != NEXUS_ERR_SAFETY) {
        printf("  FAIL: release wrong owner returned %d, expected %d\n", err, NEXUS_ERR_SAFETY);
        return 1;
    }
    printf("  PASS: release wrong owner\n");

    // Cleanup
    for (int i = 0; i < 5; i++) {
        nexus_resource_release(i, NEXUS_RES_TYPE_UART, "MultiTest");
    }

    return 0;
}
