#include "nexus_gpio.h"
#include "nexus_log.h"
#include <string.h>

static bool s_initialized = false;

nexus_err_t nexus_gpio_init(void) {
    s_initialized = true;
    NEXUS_LOG_INFO("GPIO", "GPIO engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_gpio_set_mode(uint8_t pin, nexus_gpio_mode_t mode) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    NEXUS_LOG_DEBUG("GPIO", "Pin %u mode=%d", pin, mode);
    return NEXUS_OK;
}

nexus_err_t nexus_gpio_write(uint8_t pin, uint8_t state) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    NEXUS_LOG_DEBUG("GPIO", "Pin %u = %u", pin, state);
    return NEXUS_OK;
}

nexus_err_t nexus_gpio_read(uint8_t pin, uint8_t* state) {
    if (!s_initialized || !state) return NEXUS_ERR_INVALID_PARAM;
    *state = 0;
    return NEXUS_OK;
}

nexus_err_t nexus_gpio_toggle(uint8_t pin) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    NEXUS_LOG_DEBUG("GPIO", "Pin %u toggled", pin);
    return NEXUS_OK;
}

nexus_err_t nexus_gpio_enable_interrupt(uint8_t pin, bool rising, bool falling) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    NEXUS_LOG_DEBUG("GPIO", "Pin %u interrupt: rising=%d falling=%d", pin, rising, falling);
    return NEXUS_OK;
}

nexus_err_t nexus_gpio_disable_interrupt(uint8_t pin) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    NEXUS_LOG_DEBUG("GPIO", "Pin %u interrupt disabled", pin);
    return NEXUS_OK;
}
