#ifndef NEXUS_DISCOVERY_H
#define NEXUS_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"
#include "nexus_device_db.h"

#define NEXUS_DISCOVERY_MAX_CANDIDATES 16

typedef enum {
    NEXUS_DISC_BUS_I2C = 0,
    NEXUS_DISC_BUS_SPI,
    NEXUS_DISC_BUS_UART,
    NEXUS_DISC_BUS_CAN,
    NEXUS_DISC_BUS_GPIO,
} nexus_disc_bus_t;

typedef struct {
    nexus_disc_bus_t bus;
    uint8_t address;
    uint8_t identity_reg;
    uint8_t identity_val;
    const nexus_device_info_t* device;
    float confidence;
    char description[64];
} nexus_discovery_candidate_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t num_candidates;
    nexus_discovery_candidate_t candidates[NEXUS_DISCOVERY_MAX_CANDIDATES];
    bool power_on;
    float voltage;
    float current;
} nexus_discovery_result_t;

nexus_err_t nexus_discovery_init(void);
nexus_err_t nexus_discovery_scan_all(nexus_discovery_result_t* result);
nexus_err_t nexus_discovery_scan_bus(nexus_disc_bus_t bus, nexus_discovery_result_t* result);
nexus_err_t nexus_discovery_identify(nexus_disc_bus_t bus, uint8_t addr, uint8_t reg, uint8_t val, const nexus_device_info_t** device);
nexus_err_t nexus_discovery_set_power(bool on, float voltage);
nexus_err_t nexus_discovery_get_last_result(nexus_discovery_result_t* result);

#endif
