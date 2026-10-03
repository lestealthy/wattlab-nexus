#ifndef NEXUS_RESOURCE_H
#define NEXUS_RESOURCE_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

typedef enum {
    NEXUS_RES_FREE = 0,
    NEXUS_RES_RESERVED,
    NEXUS_RES_ACQUIRED,
} nexus_resource_state_t;

typedef enum {
    NEXUS_RES_TYPE_GPIO = 0,
    NEXUS_RES_TYPE_UART,
    NEXUS_RES_TYPE_I2C,
    NEXUS_RES_TYPE_SPI,
    NEXUS_RES_TYPE_CAN,
    NEXUS_RES_TYPE_ADC,
    NEXUS_RES_TYPE_DAC,
    NEXUS_RES_TYPE_TIMER,
    NEXUS_RES_TYPE_DMA,
    NEXUS_RES_TYPE_EXTI,
} nexus_resource_type_t;

typedef struct {
    uint32_t id;
    nexus_resource_type_t type;
    nexus_resource_state_t state;
    const char* owner;
    void* config;
} nexus_resource_t;

nexus_err_t nexus_resource_init(void);
nexus_err_t nexus_resource_acquire(uint32_t id, nexus_resource_type_t type, const char* owner, void* config);
nexus_err_t nexus_resource_release(uint32_t id, nexus_resource_type_t type, const char* owner);
nexus_err_t nexus_resource_query(uint32_t id, nexus_resource_type_t type, nexus_resource_t* out);
bool nexus_resource_is_available(uint32_t id, nexus_resource_type_t type);
const char* nexus_resource_owner(uint32_t id, nexus_resource_type_t type);

#endif
