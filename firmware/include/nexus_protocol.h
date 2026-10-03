#ifndef NEXUS_PROTOCOL_H
#define NEXUS_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/* Protocol event types for the unified timeline */
typedef enum {
    NEXUS_EVT_UART_RX = 0,
    NEXUS_EVT_UART_TX,
    NEXUS_EVT_I2C_START,
    NEXUS_EVT_I2C_ADDR,
    NEXUS_EVT_I2C_DATA,
    NEXUS_EVT_I2C_STOP,
    NEXUS_EVT_SPI_CS_LOW,
    NEXUS_EVT_SPI_CS_HIGH,
    NEXUS_EVT_SPI_DATA,
    NEXUS_EVT_GPIO_CHANGE,
    NEXUS_EVT_CAN_RX,
    NEXUS_EVT_CAN_TX,
    NEXUS_EVT_ADC_SAMPLE,
    NEXUS_EVT_TRIGGER,
    NEXUS_EVT_POWER,
} nexus_event_type_t;

typedef struct {
    nexus_event_type_t type;
    uint32_t timestamp_ms;
    uint32_t timestamp_us;
    uint8_t pin;
    uint8_t channel;
    uint16_t data_len;
    uint8_t data[16];
    uint32_t flags;
} nexus_event_t;

/* Protocol analyzer interface */
typedef struct nexus_analyzer_s nexus_analyzer_t;

typedef struct {
    nexus_err_t (*init)(nexus_analyzer_t* self);
    nexus_err_t (*process)(nexus_analyzer_t* self, const nexus_event_t* event);
    nexus_err_t (*reset)(nexus_analyzer_t* self);
    void* (*get_state)(nexus_analyzer_t* self);
} nexus_analyzer_vtable_t;

struct nexus_analyzer_s {
    const nexus_analyzer_vtable_t* vtable;
    void* state;
};

/* Protocol generator interface */
typedef struct nexus_generator_s nexus_generator_t;

typedef struct {
    nexus_err_t (*init)(nexus_generator_t* self);
    nexus_err_t (*start)(nexus_generator_t* self);
    nexus_err_t (*stop)(nexus_generator_t* self);
    nexus_err_t (*set_config)(nexus_generator_t* self, const void* config);
} nexus_generator_vtable_t;

struct nexus_generator_s {
    const nexus_generator_vtable_t* vtable;
    void* state;
};

#endif
