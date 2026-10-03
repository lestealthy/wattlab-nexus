#include "nexus_power.h"
#include "nexus_log.h"
#include <string.h>

static bool s_initialized = false;
static nexus_power_config_t s_config;
static nexus_power_status_t s_status;

nexus_err_t nexus_power_init(void) {
    memset(&s_config, 0, sizeof(s_config));
    memset(&s_status, 0, sizeof(s_status));
    s_config.voltage_setpoint = 3.3f;
    s_config.current_limit = 0.5f;
    s_config.enable_on_startup = false;
    s_config.ramp_time_ms = 100;
    s_config.overcurrent_protection = true;
    s_config.overvoltage_protection = true;
    s_config.undervoltage_protection = true;
    s_initialized = true;
    NEXUS_LOG_INFO("POWER", "Power management initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_power_configure(const nexus_power_config_t* config) {
    if (!s_initialized || !config) return NEXUS_ERR_INVALID_PARAM;
    s_config = *config;
    NEXUS_LOG_INFO("POWER", "Power configured: %.2fV, limit=%.2fA", config->voltage_setpoint, config->current_limit);
    return NEXUS_OK;
}

nexus_err_t nexus_power_enable(float voltage) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_status.enabled = true;
    s_status.voltage = voltage;
    s_status.current = 0.0f;
    s_status.power = 0.0f;
    s_status.energy = 0.0f;
    s_status.peak_current = 0.0f;
    s_status.avg_current = 0.0f;
    s_status.min_voltage = voltage;
    s_status.max_voltage = voltage;
    s_status.sample_count = 0;
    s_status.duration_ms = 0;
    s_status.overcurrent = false;
    s_status.overvoltage = false;
    s_status.undervoltage = false;
    NEXUS_LOG_INFO("POWER", "Power enabled: %.2fV", voltage);
    return NEXUS_OK;
}

nexus_err_t nexus_power_disable(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_status.enabled = false;
    s_status.voltage = 0.0f;
    s_status.current = 0.0f;
    s_status.power = 0.0f;
    NEXUS_LOG_INFO("POWER", "Power disabled");
    return NEXUS_OK;
}

nexus_err_t nexus_power_get_status(nexus_power_status_t* status) {
    if (!s_initialized || !status) return NEXUS_ERR_INVALID_PARAM;
    *status = s_status;
    return NEXUS_OK;
}

nexus_err_t nexus_power_set_voltage(float voltage) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_config.voltage_setpoint = voltage;
    if (s_status.enabled) {
        s_status.voltage = voltage;
    }
    NEXUS_LOG_INFO("POWER", "Voltage set: %.2fV", voltage);
    return NEXUS_OK;
}

nexus_err_t nexus_power_set_current_limit(float limit) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_config.current_limit = limit;
    NEXUS_LOG_INFO("POWER", "Current limit set: %.2fA", limit);
    return NEXUS_OK;
}

bool nexus_power_is_enabled(void) {
    return s_status.enabled;
}

float nexus_power_get_voltage(void) {
    return s_status.voltage;
}

float nexus_power_get_current(void) {
    return s_status.current;
}
