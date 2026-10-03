#ifndef NEXUS_UART_H
#define NEXUS_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

typedef struct {
    uint32_t baud_rate;
    uint8_t data_bits;
    char parity;
    uint8_t stop_bits;
    bool flow_control;
} nexus_uart_config_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t data[16];
    uint8_t len;
    bool is_tx;
} nexus_uart_event_t;

nexus_err_t nexus_uart_init(void);
nexus_err_t nexus_uart_configure(const nexus_uart_config_t* config);
nexus_err_t nexus_uart_send(const uint8_t* data, uint32_t len);
nexus_err_t nexus_uart_receive(uint8_t* data, uint32_t max_len, uint32_t* received);
nexus_err_t nexus_uart_flush(void);
bool nexus_uart_is_busy(void);
uint32_t nexus_uart_get_baud_rate(void);

#endif
