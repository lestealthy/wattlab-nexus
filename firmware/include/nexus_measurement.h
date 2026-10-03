#ifndef NEXUS_MEASUREMENT_H
#define NEXUS_MEASUREMENT_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

typedef struct {
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
} nexus_power_measurement_t;

typedef struct {
    float frequency;
    float period;
    float duty_cycle;
    float pulse_width_high;
    float pulse_width_low;
    uint32_t edge_count;
    uint32_t sample_count;
} nexus_signal_measurement_t;

typedef struct {
    float min;
    float max;
    float avg;
    float rms;
    float peak_to_peak;
    uint32_t sample_count;
} nexus_adc_statistics_t;

nexus_err_t nexus_measurement_init(void);
nexus_err_t nexus_measurement_calibrate_adc(float offset, float gain);
nexus_err_t nexus_measurement_adc_to_voltage(uint16_t raw, float* voltage);
nexus_err_t nexus_measurement_compute_frequency(uint32_t* timestamps, uint32_t count, float* frequency);
nexus_err_t nexus_measurement_compute_duty_cycle(uint32_t high_time, uint32_t low_time, float* duty);
nexus_err_t nexus_measurement_compute_rms(float* samples, uint32_t count, float* rms);
nexus_err_t nexus_measurement_compute_statistics(float* samples, uint32_t count, nexus_adc_statistics_t* stats);
nexus_err_t nexus_measurement_power_update(float voltage, float current, uint32_t delta_ms, nexus_power_measurement_t* power);

#endif
