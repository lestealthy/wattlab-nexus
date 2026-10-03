#ifndef NEXUS_DEVICE_DB_H
#define NEXUS_DEVICE_DB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

#define NEXUS_DEVICE_DB_MAX_DEVICES 32
#define NEXUS_DEVICE_DB_MAX_REGISTERS 16
#define NEXUS_DEVICE_DB_MAX_ADDRESSES 4
#define NEXUS_DEVICE_DB_NAME_LEN 32
#define NEXUS_DEVICE_DB_ID_LEN 16

typedef enum {
    NEXUS_BUS_I2C = 0,
    NEXUS_BUS_SPI,
    NEXUS_BUS_UART,
    NEXUS_BUS_CAN,
    NEXUS_BUS_ONEWIRE,
} nexus_bus_t;

typedef enum {
    NEXUS_REG_UINT8 = 0,
    NEXUS_REG_INT8,
    NEXUS_REG_UINT16,
    NEXUS_REG_INT16,
    NEXUS_REG_UINT32,
    NEXUS_REG_INT32,
    NEXUS_REG_FLOAT,
    NEXUS_REG_BYTES,
} nexus_reg_type_t;

typedef struct {
    char name[16];
    uint8_t address;
    nexus_reg_type_t type;
    uint8_t len;
    float scale;
    float offset;
    char unit[8];
} nexus_device_reg_t;

typedef struct {
    char id[NEXUS_DEVICE_DB_ID_LEN];
    char name[NEXUS_DEVICE_DB_NAME_LEN];
    nexus_bus_t bus;
    uint8_t num_addresses;
    uint8_t addresses[NEXUS_DEVICE_DB_MAX_ADDRESSES];
    uint8_t identity_register;
    uint8_t identity_value;
    uint8_t num_registers;
    nexus_device_reg_t registers[NEXUS_DEVICE_DB_MAX_REGISTERS];
    char capabilities[64];
} nexus_device_info_t;

nexus_err_t nexus_device_db_init(void);
nexus_err_t nexus_device_db_load(void);
nexus_err_t nexus_device_db_save(void);
const nexus_device_info_t* nexus_device_db_find_by_id(const char* id);
const nexus_device_info_t* nexus_device_db_identify(nexus_bus_t bus, uint8_t address, uint8_t reg, uint8_t value);
const nexus_device_info_t* nexus_device_db_get_all(uint32_t* count);
nexus_err_t nexus_device_db_add(const nexus_device_info_t* device);
nexus_err_t nexus_device_db_remove(const char* id);

#endif
