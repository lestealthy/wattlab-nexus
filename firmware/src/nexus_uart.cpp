#include "nexus_uart.h"
#include "nexus_log.h"
#include <string.h>

static bool s_initialized = false;
static nexus_uart_config_t s_config;

nexus_err_t nexus_uart_init(void) {
    memset(&s_config, 0, sizeof(s_config));
    s_config.baud_rate = 115200;
    s_config.data_bits = 8;
    s_config.parity = 'N';
    s_config.stop_bits = 1;
    s_config.flow_control = false;
    s_initialized = true;
    NEXUS_LOG_INFO("UART", "UART engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_uart_configure(const nexus_uart_config_t* config) {
    if (!s_initialized || !config) return NEXUS_ERR_INVALID_PARAM;
    s_config = *config;
    NEXUS_LOG_INFO("UART", "UART configured: %u baud", config->baud_rate);
    return NEXUS_OK;
}

nexus_err_t nexus_uart_send(const uint8_t* data, uint32_t len) {
    if (!s_initialized || !data) return NEXUS_ERR_INVALID_PARAM;
    NEXUS_LOG_DEBUG("UART", "TX %u bytes", len);
    return NEXUS_OK;
}

nexus_err_t nexus_uart_receive(uint8_t* data, uint32_t max_len, uint32_t* received) {
    if (!s_initialized || !data || !received) return NEXUS_ERR_INVALID_PARAM;
    *received = 0;
    return NEXUS_OK;
}

nexus_err_t nexus_uart_flush(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    return NEXUS_OK;
}

bool nexus_uart_is_busy(void) {
    return false;
}

uint32_t nexus_uart_get_baud_rate(void) {
    return s_config.baud_rate;
}
