#ifndef NEXUS_CAN_H
#define NEXUS_CAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

typedef struct {
    uint32_t bitrate;
    bool extended_id;
    uint32_t filter_id;
    uint32_t filter_mask;
} nexus_can_config_t;

typedef struct {
    uint32_t timestamp_ms;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    bool extended;
    bool rtr;
} nexus_can_frame_t;

nexus_err_t nexus_can_init(void);
nexus_err_t nexus_can_configure(const nexus_can_config_t* config);
nexus_err_t nexus_can_send(const nexus_can_frame_t* frame);
nexus_err_t nexus_can_receive(nexus_can_frame_t* frame, uint32_t timeout_ms);
nexus_err_t nexus_can_set_filter(uint32_t id, uint32_t mask);
nexus_err_t nexus_can_flush(void);
bool nexus_can_is_busy(void);

#endif
