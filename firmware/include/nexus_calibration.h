#ifndef NEXUS_CALIBRATION_H
#define NEXUS_CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

#define NEXUS_CALIBRATION_VERSION 1
#define NEXUS_CALIBRATION_MAGIC 0x4342  // "CB"

typedef struct {
    uint16_t version;
    uint16_t magic;
    uint32_t checksum;

    struct {
        float offset;
        float gain;
        float reference;
    } adc;

    struct {
        float offset;
        float gain;
    } voltage;

    struct {
        float offset;
        float gain;
    } current;

    struct {
        float offset;
        float gain;
    } frequency;

    struct {
        float offset;
        float gain;
    } temperature;

} nexus_calibration_t;

nexus_err_t nexus_calibration_init(void);
nexus_err_t nexus_calibration_load(void);
nexus_err_t nexus_calibration_save(void);
nexus_err_t nexus_calibration_reset(void);
nexus_calibration_t* nexus_calibration_get(void);
nexus_err_t nexus_calibration_validate(const nexus_calibration_t* cal);
nexus_err_t nexus_calibration_set_adc(float offset, float gain);
nexus_err_t nexus_calibration_set_voltage(float offset, float gain);
nexus_err_t nexus_calibration_set_current(float offset, float gain);

#endif
