#ifndef NEXUS_I2C_H
#define NEXUS_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

typedef struct {
    uint32_t frequency;
    bool use_pullups;
    uint16_t timeout_ms;
} nexus_i2c_config_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t address;
    uint8_t reg;
    uint8_t data[16];
    uint8_t len;
    bool is_read;
    bool ack;
} nexus_i2c_event_t;

nexus_err_t nexus_i2c_init(void);
nexus_err_t nexus_i2c_configure(const nexus_i2c_config_t* config);
nexus_err_t nexus_i2c_scan(uint8_t* addresses, uint32_t max_addrs, uint32_t* found);
nexus_err_t nexus_i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, uint32_t len);
nexus_err_t nexus_i2c_write(uint8_t addr, uint8_t reg, const uint8_t* data, uint32_t len);
nexus_err_t nexus_i2c_read_byte(uint8_t addr, uint8_t reg, uint8_t* value);
nexus_err_t nexus_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t value);
bool nexus_i2c_detect(uint8_t addr);

#endif
