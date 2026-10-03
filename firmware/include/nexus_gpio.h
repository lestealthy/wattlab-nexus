#ifndef NEXUS_GPIO_H
#define NEXUS_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

typedef enum {
    NEXUS_GPIO_INPUT = 0,
    NEXUS_GPIO_OUTPUT,
    NEXUS_GPIO_INPUT_PULLUP,
    NEXUS_GPIO_INPUT_PULLDOWN,
} nexus_gpio_mode_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t pin;
    uint8_t state;
} nexus_gpio_event_t;

nexus_err_t nexus_gpio_init(void);
nexus_err_t nexus_gpio_set_mode(uint8_t pin, nexus_gpio_mode_t mode);
nexus_err_t nexus_gpio_write(uint8_t pin, uint8_t state);
nexus_err_t nexus_gpio_read(uint8_t pin, uint8_t* state);
nexus_err_t nexus_gpio_toggle(uint8_t pin);
nexus_err_t nexus_gpio_enable_interrupt(uint8_t pin, bool rising, bool falling);
nexus_err_t nexus_gpio_disable_interrupt(uint8_t pin);

#endif
