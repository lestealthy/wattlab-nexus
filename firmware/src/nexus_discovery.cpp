#include "nexus_discovery.h"
#include "nexus_log.h"
#include <string.h>
#include <stdio.h>

static bool s_initialized = false;
static nexus_discovery_result_t s_last_result;

nexus_err_t nexus_discovery_init(void) {
    memset(&s_last_result, 0, sizeof(s_last_result));
    s_initialized = true;
    NEXUS_LOG_INFO("DISCOVERY", "Discovery engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_discovery_scan_all(nexus_discovery_result_t* result) {
    if (!s_initialized || !result) return NEXUS_ERR_INVALID_PARAM;

    memset(result, 0, sizeof(nexus_discovery_result_t));
    result->timestamp_ms = 0; // Would use millis() on hardware

    NEXUS_LOG_INFO("DISCOVERY", "Starting full bus scan...");

    // Scan I2C bus
    nexus_discovery_scan_bus(NEXUS_DISC_BUS_I2C, result);

    // Scan other buses
    // TODO: Implement SPI, UART, CAN, GPIO scanning

    NEXUS_LOG_INFO("DISCOVERY", "Scan complete: %u candidates found", result->num_candidates);
    s_last_result = *result;
    return NEXUS_OK;
}

nexus_err_t nexus_discovery_scan_bus(nexus_disc_bus_t bus, nexus_discovery_result_t* result) {
    if (!s_initialized || !result) return NEXUS_ERR_INVALID_PARAM;

    NEXUS_LOG_INFO("DISCOVERY", "Scanning bus %d...", bus);

    if (bus == NEXUS_DISC_BUS_I2C) {
        // I2C scan
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            // Try to read identity register
            uint8_t value = 0;
            // Simulate I2C read
            // In real implementation: nexus_i2c_read_byte(addr, 0xD0, &value);

            // Check against device database
            const nexus_device_info_t* device = nexus_device_db_identify(NEXUS_BUS_I2C, addr, 0xD0, value);
            if (device && result->num_candidates < NEXUS_DISCOVERY_MAX_CANDIDATES) {
                nexus_discovery_candidate_t* cand = &result->candidates[result->num_candidates++];
                cand->bus = bus;
                cand->address = addr;
                cand->identity_reg = 0xD0;
                cand->identity_val = value;
                cand->device = device;
                cand->confidence = 1.0f;
                snprintf(cand->description, sizeof(cand->description), "%s at 0x%02X", device->name, addr);
                NEXUS_LOG_INFO("DISCOVERY", "Found: %s at 0x%02X", device->name, addr);
            }
        }
    }

    return NEXUS_OK;
}

nexus_err_t nexus_discovery_identify(nexus_disc_bus_t bus, uint8_t addr, uint8_t reg, uint8_t val, const nexus_device_info_t** device) {
    if (!s_initialized || !device) return NEXUS_ERR_INVALID_PARAM;

    *device = nexus_device_db_identify(NEXUS_BUS_I2C, addr, reg, val);
    return NEXUS_OK;
}

nexus_err_t nexus_discovery_set_power(bool on, float voltage) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;

    if (on) {
        NEXUS_LOG_INFO("DISCOVERY", "Power ON: %.2fV", voltage);
    } else {
        NEXUS_LOG_INFO("DISCOVERY", "Power OFF");
    }

    return NEXUS_OK;
}

nexus_err_t nexus_discovery_get_last_result(nexus_discovery_result_t* result) {
    if (!s_initialized || !result) return NEXUS_ERR_INVALID_PARAM;
    *result = s_last_result;
    return NEXUS_OK;
}
