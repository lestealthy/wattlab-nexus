#ifndef NEXUS_CAPTURE_H
#define NEXUS_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

#define NEXUS_CAPTURE_MAX_CHANNELS 8
#define NEXUS_CAPTURE_MAX_SAMPLES 4096

typedef enum {
    NEXUS_CAPTURE_DIGITAL = 0,
    NEXUS_CAPTURE_ANALOG,
} nexus_capture_type_t;

typedef enum {
    NEXUS_TRIG_RISING = 0,
    NEXUS_TRIG_FALLING,
    NEXUS_TRIG_THRESHOLD,
    NEXUS_TRIG_PROTOCOL,
    NEXUS_TRIG_EXTERNAL,
} nexus_trigger_type_t;

typedef struct {
    nexus_capture_type_t type;
    uint8_t channel;
    uint32_t sample_rate_hz;
    uint32_t pre_trigger_samples;
    uint32_t post_trigger_samples;
    nexus_trigger_type_t trigger_type;
    uint8_t trigger_channel;
    uint16_t trigger_threshold;
    bool armed;
} nexus_capture_config_t;

typedef struct {
    uint32_t timestamp_ms;
    uint16_t value;
    uint8_t flags;
} nexus_capture_sample_t;

typedef struct {
    nexus_capture_config_t config;
    nexus_capture_sample_t* buffer;
    uint32_t buffer_size;
    uint32_t write_index;
    uint32_t read_index;
    uint32_t count;
    bool triggered;
    bool overflow;
    uint32_t trigger_index;
} nexus_capture_channel_t;

nexus_err_t nexus_capture_init(void);
nexus_err_t nexus_capture_configure_channel(uint8_t ch, const nexus_capture_config_t* config);
nexus_err_t nexus_capture_arm(uint8_t ch);
nexus_err_t nexus_capture_disarm(uint8_t ch);
nexus_err_t nexus_capture_store_sample(uint8_t ch, const nexus_capture_sample_t* sample);
nexus_err_t nexus_capture_get_samples(uint8_t ch, nexus_capture_sample_t* out, uint32_t max_samples, uint32_t* out_count);
nexus_err_t nexus_capture_clear(uint8_t ch);
bool nexus_capture_is_triggered(uint8_t ch);
nexus_err_t nexus_capture_process_trigger(uint8_t ch);

#endif
