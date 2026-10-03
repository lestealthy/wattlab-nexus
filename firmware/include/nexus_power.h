#ifndef NEXUS_POWER_H
#define NEXUS_POWER_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

typedef struct {
    bool enabled;
    float voltage;
    float current;
    float power;
    float energy;
    float peak_current;
    float avg_current;
    float min_voltage;
    float max_voltage;
    uint32_t sample_count;
    uint32_t duration_ms;
    bool overcurrent;
    bool overvoltage;
    bool undervoltage;
} nexus_power_status_t;

typedef struct {
    float voltage_setpoint;
    float current_limit;
    bool enable_on_startup;
    uint32_t ramp_time_ms;
    bool overcurrent_protection;
    bool overvoltage_protection;
    bool undervoltage_protection;
} nexus_power_config_t;

nexus_err_t nexus_power_init(void);
nexus_err_t nexus_power_configure(const nexus_power_config_t* config);
nexus_err_t nexus_power_enable(float voltage);
nexus_err_t nexus_power_disable(void);
nexus_err_t nexus_power_get_status(nexus_power_status_t* status);
nexus_err_t nexus_power_set_voltage(float voltage);
nexus_err_t nexus_power_set_current_limit(float limit);
bool nexus_power_is_enabled(void);
float nexus_power_get_voltage(void);
float nexus_power_get_current(void);

#endif
