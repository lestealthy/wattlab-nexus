#ifndef VIRTUAL_DUT_H
#define VIRTUAL_DUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    VDUT_I2C = 0,
    VDUT_SPI,
    VDUT_UART,
    VDUT_CAN,
    VDUT_GPIO,
} vdut_bus_t;

typedef struct {
    uint8_t address;
    uint8_t reg;
    uint8_t value;
} vdut_i2c_device_t;

typedef struct {
    uint32_t baud;
    const char* tx_data;
    uint32_t tx_len;
} vdut_uart_config_t;

typedef struct {
    uint32_t freq_hz;
    float duty_cycle;
    uint8_t pin;
} vdut_gpio_config_t;

typedef struct {
    char name[32];
    vdut_bus_t bus;
    union {
        vdut_i2c_device_t i2c;
        vdut_uart_config_t uart;
        vdut_gpio_config_t gpio;
    } config;
    bool enabled;
} vdut_device_t;

typedef struct {
    vdut_device_t* devices;
    uint32_t num_devices;
    bool power_on;
    float voltage;
    float current;
} vdut_context_t;

int vdut_init(vdut_context_t* ctx);
int vdut_add_i2c_device(vdut_context_t* ctx, uint8_t addr, uint8_t reg, uint8_t val);
int vdut_add_uart_device(vdut_context_t* ctx, uint32_t baud, const char* tx_data);
int vdut_add_gpio_device(vdut_context_t* ctx, uint8_t pin, uint32_t freq, float duty);
int vdut_power_on(vdut_context_t* ctx, float voltage);
int vdut_power_off(vdut_context_t* ctx);
int vdut_scan_i2c(vdut_context_t* ctx, uint8_t* addresses, uint32_t max_addrs, uint32_t* found);
int vdut_read_i2c(vdut_context_t* ctx, uint8_t addr, uint8_t reg, uint8_t* value);
int vdut_write_i2c(vdut_context_t* ctx, uint8_t addr, uint8_t reg, uint8_t value);
int vdut_uart_tx(vdut_context_t* ctx, uint8_t* data, uint32_t len);
int vdut_uart_rx(vdut_context_t* ctx, uint8_t* data, uint32_t max_len, uint32_t* received);
int vdut_gpio_read(vdut_context_t* ctx, uint8_t pin, uint8_t* state);
int vdut_gpio_write(vdut_context_t* ctx, uint8_t pin, uint8_t state);
uint8_t vdut_get_identity_reg(vdut_context_t* ctx, uint8_t addr);
void vdut_cleanup(vdut_context_t* ctx);

#endif
