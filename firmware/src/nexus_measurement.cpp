#include "nexus_measurement.h"
#include "nexus_log.h"
#include <math.h>
#include <string.h>

static float s_adc_offset = 0.0f;
static float s_adc_gain = 1.0f;
static bool s_initialized = false;

nexus_err_t nexus_measurement_init(void) {
    s_adc_offset = 0.0f;
    s_adc_gain = 1.0f;
    s_initialized = true;
    NEXUS_LOG_INFO("MEASURE", "Measurement engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_measurement_calibrate_adc(float offset, float gain) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_adc_offset = offset;
    s_adc_gain = gain;
    NEXUS_LOG_INFO("MEASURE", "ADC calibrated: offset=%.4f, gain=%.4f", offset, gain);
    return NEXUS_OK;
}

nexus_err_t nexus_measurement_adc_to_voltage(uint16_t raw, float* voltage) {
    if (!s_initialized || !voltage) return NEXUS_ERR_INVALID_PARAM;
    *voltage = ((float)raw * s_adc_gain) + s_adc_offset;
    return NEXUS_OK;
}

nexus_err_t nexus_measurement_compute_frequency(uint32_t* timestamps, uint32_t count, float* frequency) {
    if (!s_initialized || !timestamps || !frequency || count < 2) return NEXUS_ERR_INVALID_PARAM;

    uint32_t total_time = timestamps[count - 1] - timestamps[0];
    if (total_time == 0) {
        *frequency = 0.0f;
        return NEXUS_OK;
    }

    *frequency = (float)(count - 1) * 1000.0f / (float)total_time;
    return NEXUS_OK;
}

nexus_err_t nexus_measurement_compute_duty_cycle(uint32_t high_time, uint32_t low_time, float* duty) {
    if (!s_initialized || !duty) return NEXUS_ERR_INVALID_PARAM;

    uint32_t total = high_time + low_time;
    if (total == 0) {
        *duty = 0.0f;
        return NEXUS_OK;
    }

    *duty = (float)high_time * 100.0f / (float)total;
    return NEXUS_OK;
}

nexus_err_t nexus_measurement_compute_rms(float* samples, uint32_t count, float* rms) {
    if (!s_initialized || !samples || !rms || count == 0) return NEXUS_ERR_INVALID_PARAM;

    float sum_sq = 0.0f;
    for (uint32_t i = 0; i < count; i++) {
        sum_sq += samples[i] * samples[i];
    }

    *rms = sqrtf(sum_sq / (float)count);
    return NEXUS_OK;
}

nexus_err_t nexus_measurement_compute_statistics(float* samples, uint32_t count, nexus_adc_statistics_t* stats) {
    if (!s_initialized || !samples || !stats || count == 0) return NEXUS_ERR_INVALID_PARAM;

    float min = samples[0];
    float max = samples[0];
    float sum = 0.0f;
    float sum_sq = 0.0f;

    for (uint32_t i = 0; i < count; i++) {
        if (samples[i] < min) min = samples[i];
        if (samples[i] > max) max = samples[i];
        sum += samples[i];
        sum_sq += samples[i] * samples[i];
    }

    stats->min = min;
    stats->max = max;
    stats->avg = sum / (float)count;
    stats->rms = sqrtf(sum_sq / (float)count);
    stats->peak_to_peak = max - min;
    stats->sample_count = count;

    return NEXUS_OK;
}

nexus_err_t nexus_measurement_power_update(float voltage, float current, uint32_t delta_ms, nexus_power_measurement_t* power) {
    if (!s_initialized || !power) return NEXUS_ERR_INVALID_PARAM;

    power->voltage = voltage;
    power->current = current;
    power->power = voltage * current;
    power->energy += power->power * (float)delta_ms / 3600000.0f; // Wh

    if (current > power->peak_current) {
        power->peak_current = current;
    }

    // Running average
    power->sample_count++;
    power->avg_current = ((power->avg_current * (power->sample_count - 1)) + current) / power->sample_count;
    power->duration_ms += delta_ms;

    return NEXUS_OK;
}
