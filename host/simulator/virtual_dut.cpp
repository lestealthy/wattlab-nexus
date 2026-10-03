#include "virtual_dut.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <chrono>

static uint32_t get_timestamp_ms() {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return (uint32_t)(ms.count() % 100000);
}

int vdut_init(vdut_context_t* ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(vdut_context_t));
    ctx->devices = (vdut_device_t*)calloc(16, sizeof(vdut_device_t));
    if (!ctx->devices) return -1;
    ctx->num_devices = 0;
    ctx->power_on = false;
    ctx->voltage = 0.0f;
    ctx->current = 0.0f;
    return 0;
}

int vdut_add_i2c_device(vdut_context_t* ctx, uint8_t addr, uint8_t reg, uint8_t val) {
    if (!ctx || !ctx->devices) return -1;
    if (ctx->num_devices >= 16) return -1;

    vdut_device_t* dev = &ctx->devices[ctx->num_devices++];
    snprintf(dev->name, sizeof(dev->name), "I2C_0x%02X", addr);
    dev->bus = VDUT_I2C;
    dev->config.i2c.address = addr;
    dev->config.i2c.reg = reg;
    dev->config.i2c.value = val;
    dev->enabled = true;
    return 0;
}

int vdut_add_uart_device(vdut_context_t* ctx, uint32_t baud, const char* tx_data) {
    if (!ctx || !ctx->devices) return -1;
    if (ctx->num_devices >= 16) return -1;

    vdut_device_t* dev = &ctx->devices[ctx->num_devices++];
    snprintf(dev->name, sizeof(dev->name), "UART_%u", baud);
    dev->bus = VDUT_UART;
    dev->config.uart.baud = baud;
    dev->config.uart.tx_data = tx_data;
    dev->config.uart.tx_len = tx_data ? strlen(tx_data) : 0;
    dev->enabled = true;
    return 0;
}

int vdut_add_gpio_device(vdut_context_t* ctx, uint8_t pin, uint32_t freq, float duty) {
    if (!ctx || !ctx->devices) return -1;
    if (ctx->num_devices >= 16) return -1;

    vdut_device_t* dev = &ctx->devices[ctx->num_devices++];
    snprintf(dev->name, sizeof(dev->name), "GPIO_D%u", pin);
    dev->bus = VDUT_GPIO;
    dev->config.gpio.pin = pin;
    dev->config.gpio.freq_hz = freq;
    dev->config.gpio.duty_cycle = duty;
    dev->enabled = true;
    return 0;
}

int vdut_power_on(vdut_context_t* ctx, float voltage) {
    if (!ctx) return -1;
    ctx->power_on = true;
    ctx->voltage = voltage;
    ctx->current = 0.183f; // Simulate 183mA
    return 0;
}

int vdut_power_off(vdut_context_t* ctx) {
    if (!ctx) return -1;
    ctx->power_on = false;
    ctx->voltage = 0.0f;
    ctx->current = 0.0f;
    return 0;
}

int vdut_scan_i2c(vdut_context_t* ctx, uint8_t* addresses, uint32_t max_addrs, uint32_t* found) {
    if (!ctx || !addresses || !found) return -1;
    *found = 0;

    for (uint32_t i = 0; i < ctx->num_devices && *found < max_addrs; i++) {
        if (ctx->devices[i].bus == VDUT_I2C && ctx->devices[i].enabled) {
            addresses[(*found)++] = ctx->devices[i].config.i2c.address;
        }
    }
    return 0;
}

int vdut_read_i2c(vdut_context_t* ctx, uint8_t addr, uint8_t reg, uint8_t* value) {
    if (!ctx || !value) return -1;

    for (uint32_t i = 0; i < ctx->num_devices; i++) {
        if (ctx->devices[i].bus == VDUT_I2C && ctx->devices[i].enabled) {
            if (ctx->devices[i].config.i2c.address == addr) {
                // Return the identity value for the identity register
                // For simulation, return the stored identity value regardless of register
                *value = ctx->devices[i].config.i2c.value;
                return 0;
            }
        }
    }
    return -1;
}

// Helper to get the identity register for a device
uint8_t vdut_get_identity_reg(vdut_context_t* ctx, uint8_t addr) {
    if (!ctx) return 0xD0; // default

    for (uint32_t i = 0; i < ctx->num_devices; i++) {
        if (ctx->devices[i].bus == VDUT_I2C && ctx->devices[i].enabled) {
            if (ctx->devices[i].config.i2c.address == addr) {
                return ctx->devices[i].config.i2c.reg;
            }
        }
    }
    return 0xD0; // default
}

int vdut_write_i2c(vdut_context_t* ctx, uint8_t addr, uint8_t reg, uint8_t value) {
    if (!ctx) return -1;

    for (uint32_t i = 0; i < ctx->num_devices; i++) {
        if (ctx->devices[i].bus == VDUT_I2C && ctx->devices[i].enabled) {
            if (ctx->devices[i].config.i2c.address == addr) {
                ctx->devices[i].config.i2c.reg = reg;
                ctx->devices[i].config.i2c.value = value;
                return 0;
            }
        }
    }
    return -1;
}

int vdut_uart_tx(vdut_context_t* ctx, uint8_t* data, uint32_t len) {
    if (!ctx || !data) return -1;
    // Simulate UART TX - just return success
    return 0;
}

int vdut_uart_rx(vdut_context_t* ctx, uint8_t* data, uint32_t max_len, uint32_t* received) {
    if (!ctx || !data || !received) return -1;
    *received = 0;

    for (uint32_t i = 0; i < ctx->num_devices; i++) {
        if (ctx->devices[i].bus == VDUT_UART && ctx->devices[i].enabled) {
            vdut_device_t* dev = &ctx->devices[i];
            uint32_t to_copy = dev->config.uart.tx_len;
            if (to_copy > max_len) to_copy = max_len;
            if (dev->config.uart.tx_data) {
                memcpy(data, dev->config.uart.tx_data, to_copy);
                *received = to_copy;
            }
            return 0;
        }
    }
    return -1;
}

int vdut_gpio_read(vdut_context_t* ctx, uint8_t pin, uint8_t* state) {
    if (!ctx || !state) return -1;

    for (uint32_t i = 0; i < ctx->num_devices; i++) {
        if (ctx->devices[i].bus == VDUT_GPIO && ctx->devices[i].enabled) {
            if (ctx->devices[i].config.gpio.pin == pin) {
                // Simulate 50% duty cycle
                *state = (get_timestamp_ms() % 2 == 0) ? 1 : 0;
                return 0;
            }
        }
    }
    return -1;
}

int vdut_gpio_write(vdut_context_t* ctx, uint8_t pin, uint8_t state) {
    if (!ctx) return -1;
    // Simulate GPIO write
    return 0;
}

void vdut_cleanup(vdut_context_t* ctx) {
    if (ctx && ctx->devices) {
        free(ctx->devices);
        ctx->devices = NULL;
    }
    ctx->num_devices = 0;
}
