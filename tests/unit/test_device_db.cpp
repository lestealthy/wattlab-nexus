#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nexus_device_db.h"
#include "nexus_errors.h"

int test_device_db(void) {
    printf("\n");

    // Test 1: Initialize device database
    nexus_err_t err = nexus_device_db_init();
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_device_db_init returned %d\n", err);
        return 1;
    }
    printf("  PASS: nexus_device_db_init\n");

    // Test 2: Get all devices
    uint32_t count = 0;
    const nexus_device_info_t* devices = nexus_device_db_get_all(&count);
    if (devices == NULL || count == 0) {
        printf("  FAIL: nexus_device_db_get_all returned NULL or count=0\n");
        return 1;
    }
    printf("  PASS: nexus_device_db_get_all (count=%u)\n", count);

    // Test 3: Find BME280 by ID
    const nexus_device_info_t* bme280 = nexus_device_db_find_by_id("bme280");
    if (bme280 == NULL) {
        printf("  FAIL: nexus_device_db_find_by_id(bme280) returned NULL\n");
        return 1;
    }
    if (strcmp(bme280->name, "Bosch BME280") != 0) {
        printf("  FAIL: BME280 name = %s\n", bme280->name);
        return 1;
    }
    printf("  PASS: find BME280 by ID\n");

    // Test 4: Find MPU6050 by ID
    const nexus_device_info_t* mpu6050 = nexus_device_db_find_by_id("mpu6050");
    if (mpu6050 == NULL) {
        printf("  FAIL: nexus_device_db_find_by_id(mpu6050) returned NULL\n");
        return 1;
    }
    printf("  PASS: find MPU6050 by ID\n");

    // Test 5: Find INA219 by ID
    const nexus_device_info_t* ina219 = nexus_device_db_find_by_id("ina219");
    if (ina219 == NULL) {
        printf("  FAIL: nexus_device_db_find_by_id(ina219) returned NULL\n");
        return 1;
    }
    printf("  PASS: find INA219 by ID\n");

    // Test 6: Identify BME280 by register value
    const nexus_device_info_t* identified = nexus_device_db_identify(NEXUS_BUS_I2C, 0x76, 0xD0, 0x60);
    if (identified == NULL) {
        printf("  FAIL: identify BME280 returned NULL\n");
        return 1;
    }
    if (strcmp(identified->id, "bme280") != 0) {
        printf("  FAIL: identified as %s, expected bme280\n", identified->id);
        return 1;
    }
    printf("  PASS: identify BME280 by register\n");

    // Test 7: Identify MPU6050
    identified = nexus_device_db_identify(NEXUS_BUS_I2C, 0x68, 0x75, 0x68);
    if (identified == NULL) {
        printf("  FAIL: identify MPU6050 returned NULL\n");
        return 1;
    }
    if (strcmp(identified->id, "mpu6050") != 0) {
        printf("  FAIL: identified as %s, expected mpu6050\n", identified->id);
        return 1;
    }
    printf("  PASS: identify MPU6050 by register\n");

    // Test 8: Unknown device
    identified = nexus_device_db_identify(NEXUS_BUS_I2C, 0x99, 0xD0, 0xFF);
    if (identified != NULL) {
        printf("  FAIL: unknown device identified as %s\n", identified->id);
        return 1;
    }
    printf("  PASS: unknown device returns NULL\n");

    // Test 9: Add custom device
    nexus_device_info_t custom = {
        .id = "custom_sensor",
        .name = "Custom Sensor",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 1,
        .addresses = {0x50},
        .identity_register = 0x00,
        .identity_value = 0xAB,
        .num_registers = 1,
        .registers = {
            {"DATA", 0x01, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
        },
        .capabilities = "custom"
    };
    err = nexus_device_db_add(&custom);
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_device_db_add returned %d\n", err);
        return 1;
    }
    printf("  PASS: add custom device\n");

    // Test 10: Find custom device
    const nexus_device_info_t* found = nexus_device_db_find_by_id("custom_sensor");
    if (found == NULL) {
        printf("  FAIL: find custom_sensor returned NULL\n");
        return 1;
    }
    printf("  PASS: find custom device\n");

    // Test 11: Remove device
    err = nexus_device_db_remove("custom_sensor");
    if (err != NEXUS_OK) {
        printf("  FAIL: nexus_device_db_remove returned %d\n", err);
        return 1;
    }
    printf("  PASS: remove custom device\n");

    // Test 12: Verify removal
    found = nexus_device_db_find_by_id("custom_sensor");
    if (found != NULL) {
        printf("  FAIL: custom_sensor still found after removal\n");
        return 1;
    }
    printf("  PASS: verify removal\n");

    return 0;
}
