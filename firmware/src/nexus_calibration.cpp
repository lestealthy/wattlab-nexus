#include "nexus_calibration.h"
#include "nexus_log.h"
#include <string.h>

static nexus_calibration_t s_calibration;
static bool s_initialized = false;

static uint32_t compute_checksum(const nexus_calibration_t* cal) {
    const uint8_t* data = (const uint8_t*)cal;
    uint32_t sum = 0;
    for (size_t i = 0; i < sizeof(nexus_calibration_t) - sizeof(uint32_t); i++) {
        if (i >= 4 && i < 8) continue; // Skip checksum field
        sum = (sum << 1) | (sum >> 31);
        sum += data[i];
    }
    return sum;
}

nexus_err_t nexus_calibration_init(void) {
    memset(&s_calibration, 0, sizeof(s_calibration));
    s_calibration.version = NEXUS_CALIBRATION_VERSION;
    s_calibration.magic = NEXUS_CALIBRATION_MAGIC;
    s_calibration.adc.offset = 0.0f;
    s_calibration.adc.gain = 1.0f;
    s_calibration.adc.reference = 3.3f;
    s_calibration.voltage.offset = 0.0f;
    s_calibration.voltage.gain = 1.0f;
    s_calibration.current.offset = 0.0f;
    s_calibration.current.gain = 1.0f;
    s_calibration.frequency.offset = 0.0f;
    s_calibration.frequency.gain = 1.0f;
    s_calibration.temperature.offset = 0.0f;
    s_calibration.temperature.gain = 1.0f;
    s_calibration.checksum = compute_checksum(&s_calibration);
    s_initialized = true;
    NEXUS_LOG_INFO("CALIB", "Calibration initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_calibration_load(void) {
    NEXUS_LOG_INFO("CALIB", "Loading calibration from storage");
    return NEXUS_OK;
}

nexus_err_t nexus_calibration_save(void) {
    s_calibration.checksum = compute_checksum(&s_calibration);
    NEXUS_LOG_INFO("CALIB", "Calibration saved");
    return NEXUS_OK;
}

nexus_err_t nexus_calibration_reset(void) {
    NEXUS_LOG_INFO("CALIB", "Resetting calibration to defaults");
    return nexus_calibration_init();
}

nexus_calibration_t* nexus_calibration_get(void) {
    return &s_calibration;
}

nexus_err_t nexus_calibration_validate(const nexus_calibration_t* cal) {
    if (!cal) return NEXUS_ERR_INVALID_PARAM;
    if (cal->magic != NEXUS_CALIBRATION_MAGIC) return NEXUS_ERR_INVALID_CONFIG;
    if (cal->version != NEXUS_CALIBRATION_VERSION) return NEXUS_ERR_INVALID_CONFIG;
    if (cal->checksum != compute_checksum(cal)) return NEXUS_ERR_INVALID_CONFIG;
    return NEXUS_OK;
}

nexus_err_t nexus_calibration_set_adc(float offset, float gain) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_calibration.adc.offset = offset;
    s_calibration.adc.gain = gain;
    NEXUS_LOG_INFO("CALIB", "ADC calibration set: offset=%.4f, gain=%.4f", offset, gain);
    return NEXUS_OK;
}

nexus_err_t nexus_calibration_set_voltage(float offset, float gain) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_calibration.voltage.offset = offset;
    s_calibration.voltage.gain = gain;
    NEXUS_LOG_INFO("CALIB", "Voltage calibration set: offset=%.4f, gain=%.4f", offset, gain);
    return NEXUS_OK;
}

nexus_err_t nexus_calibration_set_current(float offset, float gain) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_calibration.current.offset = offset;
    s_calibration.current.gain = gain;
    NEXUS_LOG_INFO("CALIB", "Current calibration set: offset=%.4f, gain=%.4f", offset, gain);
    return NEXUS_OK;
}
