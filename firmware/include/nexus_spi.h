#ifndef NEXUS_SPI_H
#define NEXUS_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

typedef struct {
    uint32_t frequency;
    uint8_t mode;
    uint8_t bits;
    bool cs_active_low;
} nexus_spi_config_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t data[32];
    uint8_t len;
    bool is_tx;
} nexus_spi_event_t;

nexus_err_t nexus_spi_init(void);
nexus_err_t nexus_spi_configure(const nexus_spi_config_t* config);
nexus_err_t nexus_spi_transfer(const uint8_t* tx, uint8_t* rx, uint32_t len);
nexus_err_t nexus_spi_send(const uint8_t* data, uint32_t len);
nexus_err_t nexus_spi_receive(uint8_t* data, uint32_t len);
nexus_err_t nexus_spi_cs_assert(void);
nexus_err_t nexus_spi_cs_deassert(void);

#endif
