#include "nexus_capture.h"
#include "nexus_log.h"
#include <string.h>
#include <stdlib.h>

static nexus_capture_channel_t s_channels[NEXUS_CAPTURE_MAX_CHANNELS];
static bool s_initialized = false;

nexus_err_t nexus_capture_init(void) {
    memset(s_channels, 0, sizeof(s_channels));
    s_initialized = true;
    NEXUS_LOG_INFO("CAPTURE", "Capture engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_capture_configure_channel(uint8_t ch, const nexus_capture_config_t* config) {
    if (!s_initialized || !config || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    nexus_capture_channel_t* channel = &s_channels[ch];
    channel->config = *config;
    channel->buffer_size = config->pre_trigger_samples + config->post_trigger_samples;
    if (channel->buffer_size > NEXUS_CAPTURE_MAX_SAMPLES) {
        channel->buffer_size = NEXUS_CAPTURE_MAX_SAMPLES;
    }

    if (channel->buffer == NULL) {
        channel->buffer = (nexus_capture_sample_t*)malloc(channel->buffer_size * sizeof(nexus_capture_sample_t));
        if (channel->buffer == NULL) return NEXUS_ERR_MEMORY;
    }

    channel->write_index = 0;
    channel->read_index = 0;
    channel->count = 0;
    channel->triggered = false;
    channel->overflow = false;
    channel->trigger_index = 0;

    NEXUS_LOG_DEBUG("CAPTURE", "Channel %u configured: %u samples, trigger=%d", ch, channel->buffer_size, config->trigger_type);
    return NEXUS_OK;
}

nexus_err_t nexus_capture_arm(uint8_t ch) {
    if (!s_initialized || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    nexus_capture_channel_t* channel = &s_channels[ch];
    channel->config.armed = true;
    channel->triggered = false;
    channel->write_index = 0;
    channel->count = 0;
    channel->overflow = false;

    NEXUS_LOG_DEBUG("CAPTURE", "Channel %u armed", ch);
    return NEXUS_OK;
}

nexus_err_t nexus_capture_disarm(uint8_t ch) {
    if (!s_initialized || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    s_channels[ch].config.armed = false;
    NEXUS_LOG_DEBUG("CAPTURE", "Channel %u disarmed", ch);
    return NEXUS_OK;
}

nexus_err_t nexus_capture_store_sample(uint8_t ch, const nexus_capture_sample_t* sample) {
    if (!s_initialized || !sample || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    nexus_capture_channel_t* channel = &s_channels[ch];
    if (!channel->config.armed) return NEXUS_ERR_INVALID_PARAM;
    if (channel->buffer == NULL) return NEXUS_ERR_INVALID_PARAM;

    channel->buffer[channel->write_index] = *sample;
    channel->write_index = (channel->write_index + 1) % channel->buffer_size;

    if (channel->count < channel->buffer_size) {
        channel->count++;
    } else {
        channel->overflow = true;
        channel->read_index = channel->write_index;
    }

    // Check trigger
    if (!channel->triggered && channel->config.trigger_type != NEXUS_TRIG_EXTERNAL) {
        nexus_capture_process_trigger(ch);
    }

    return NEXUS_OK;
}

nexus_err_t nexus_capture_get_samples(uint8_t ch, nexus_capture_sample_t* out, uint32_t max_samples, uint32_t* out_count) {
    if (!s_initialized || !out || !out_count || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    nexus_capture_channel_t* channel = &s_channels[ch];
    uint32_t to_read = (channel->count < max_samples) ? channel->count : max_samples;

    for (uint32_t i = 0; i < to_read; i++) {
        uint32_t idx = (channel->read_index + i) % channel->buffer_size;
        out[i] = channel->buffer[idx];
    }

    *out_count = to_read;
    return NEXUS_OK;
}

nexus_err_t nexus_capture_clear(uint8_t ch) {
    if (!s_initialized || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    nexus_capture_channel_t* channel = &s_channels[ch];
    channel->write_index = 0;
    channel->read_index = 0;
    channel->count = 0;
    channel->triggered = false;
    channel->overflow = false;
    return NEXUS_OK;
}

bool nexus_capture_is_triggered(uint8_t ch) {
    if (!s_initialized || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return false;
    return s_channels[ch].triggered;
}

nexus_err_t nexus_capture_process_trigger(uint8_t ch) {
    if (!s_initialized || ch >= NEXUS_CAPTURE_MAX_CHANNELS) return NEXUS_ERR_INVALID_PARAM;

    nexus_capture_channel_t* channel = &s_channels[ch];
    if (channel->count < 2) return NEXUS_OK;

    uint32_t latest_idx = (channel->write_index + channel->buffer_size - 1) % channel->buffer_size;
    uint32_t prev_idx = (channel->write_index + channel->buffer_size - 2) % channel->buffer_size;

    uint16_t latest = channel->buffer[latest_idx].value;
    uint16_t prev = channel->buffer[prev_idx].value;

    bool fire = false;
    switch (channel->config.trigger_type) {
        case NEXUS_TRIG_RISING:
            fire = (prev < channel->config.trigger_threshold && latest >= channel->config.trigger_threshold);
            break;
        case NEXUS_TRIG_FALLING:
            fire = (prev > channel->config.trigger_threshold && latest <= channel->config.trigger_threshold);
            break;
        case NEXUS_TRIG_THRESHOLD:
            fire = (prev < channel->config.trigger_threshold && latest >= channel->config.trigger_threshold) ||
                   (prev > channel->config.trigger_threshold && latest <= channel->config.trigger_threshold);
            break;
        default:
            break;
    }

    if (fire) {
        channel->triggered = true;
        channel->trigger_index = latest_idx;
        NEXUS_LOG_DEBUG("CAPTURE", "Channel %u triggered at index %u", ch, latest_idx);
    }

    return NEXUS_OK;
}
