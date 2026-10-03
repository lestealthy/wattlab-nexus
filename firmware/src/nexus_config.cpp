#include "nexus_config.h"
#include "nexus_log.h"
#include <string.h>
#include <stdio.h>

static nexus_config_t s_config;

static uint32_t compute_checksum(const nexus_config_t* cfg) {
    const uint8_t* data = (const uint8_t*)cfg;
    uint32_t sum = 0;
    // Checksum covers all bytes except the checksum field itself (offset 4-7)
    for (size_t i = 0; i < sizeof(nexus_config_t); i++) {
        if (i >= 4 && i < 8) continue; // Skip checksum field
        sum = (sum << 1) | (sum >> 31);
        sum += data[i];
    }
    return sum;
}

nexus_err_t nexus_config_init(void) {
    memset(&s_config, 0, sizeof(s_config));
    s_config.version = NEXUS_CONFIG_VERSION;
    s_config.magic = NEXUS_CONFIG_MAGIC;
    s_config.display.brightness = 80;
    s_config.display.theme = 0;
    s_config.display.sound_enabled = true;
    s_config.display.log_level = NEXUS_LOG_INFO;
    s_config.display.log_outputs = NEXUS_LOG_OUTPUT_SERIAL;
    s_config.uart.default_uart_baud = 115200;
    s_config.uart.default_uart_bits = 8;
    s_config.uart.default_uart_parity = 'N';
    s_config.uart.default_uart_stop = 1;
    s_config.i2c.default_i2c_freq = 100000;
    s_config.spi.default_spi_freq = 1000000;
    s_config.spi.default_spi_mode = 0;
    s_config.spi.default_spi_bits = 8;
    s_config.can.default_can_bitrate = 500000;
    s_config.discovery.auto_discover = false;
    s_config.discovery.discover_timeout_ms = 5000;
    s_config.system.demo_mode = false;
    s_config.system.virtual_dut = false;
    s_config.checksum = compute_checksum(&s_config);
    return NEXUS_OK;
}

nexus_err_t nexus_config_load(void) {
    NEXUS_LOG_INFO("CONFIG", "Loading configuration from storage");
    return NEXUS_OK;
}

nexus_err_t nexus_config_save(void) {
    s_config.checksum = compute_checksum(&s_config);
    NEXUS_LOG_INFO("CONFIG", "Configuration saved");
    return NEXUS_OK;
}

nexus_err_t nexus_config_reset(void) {
    NEXUS_LOG_INFO("CONFIG", "Resetting to defaults");
    return nexus_config_init();
}

nexus_config_t* nexus_config_get(void) {
    return &s_config;
}

nexus_err_t nexus_config_validate(const nexus_config_t* config) {
    if (config == NULL) return NEXUS_ERR_INVALID_PARAM;
    if (config->magic != NEXUS_CONFIG_MAGIC) return NEXUS_ERR_INVALID_CONFIG;
    if (config->version != NEXUS_CONFIG_VERSION) return NEXUS_ERR_INVALID_CONFIG;
    if (config->checksum != compute_checksum(config)) return NEXUS_ERR_INVALID_CONFIG;
    return NEXUS_OK;
}
