#ifndef NEXUS_CONFIG_H
#define NEXUS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

#define NEXUS_CONFIG_VERSION 1
#define NEXUS_CONFIG_MAGIC 0x4346

typedef struct {
    uint16_t version;
    uint16_t magic;
    uint32_t checksum;

    struct {
        uint8_t brightness;
        uint8_t theme;
        bool sound_enabled;
        uint8_t log_level;
        uint32_t log_outputs;
    } display;

    struct {
        uint32_t default_uart_baud;
        uint8_t default_uart_bits;
        char default_uart_parity;
        uint8_t default_uart_stop;
    } uart;

    struct {
        uint32_t default_i2c_freq;
    } i2c;

    struct {
        uint32_t default_spi_freq;
        uint8_t default_spi_mode;
        uint8_t default_spi_bits;
    } spi;

    struct {
        uint32_t default_can_bitrate;
    } can;

    struct {
        bool auto_discover;
        uint32_t discover_timeout_ms;
    } discovery;

    struct {
        bool demo_mode;
        bool virtual_dut;
    } system;

} nexus_config_t;

nexus_err_t nexus_config_init(void);
nexus_err_t nexus_config_load(void);
nexus_err_t nexus_config_save(void);
nexus_err_t nexus_config_reset(void);
nexus_config_t* nexus_config_get(void);
nexus_err_t nexus_config_validate(const nexus_config_t* config);

#endif
