#include "nexus_can.h"
#include "nexus_log.h"
#include <string.h>

static bool s_initialized = false;
static nexus_can_config_t s_config;

nexus_err_t nexus_can_init(void) {
    memset(&s_config, 0, sizeof(s_config));
    s_config.bitrate = 500000;
    s_config.extended_id = false;
    s_config.filter_id = 0;
    s_config.filter_mask = 0;
    s_initialized = true;
    NEXUS_LOG_INFO("CAN", "CAN engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_can_configure(const nexus_can_config_t* config) {
    if (!s_initialized || !config) return NEXUS_ERR_INVALID_PARAM;
    s_config = *config;
    NEXUS_LOG_INFO("CAN", "CAN configured: %u bps", config->bitrate);
    return NEXUS_OK;
}

nexus_err_t nexus_can_send(const nexus_can_frame_t* frame) {
    if (!s_initialized || !frame) return NEXUS_ERR_INVALID_PARAM;
    NEXUS_LOG_DEBUG("CAN", "TX ID=0x%08X DLC=%u", frame->id, frame->dlc);
    return NEXUS_OK;
}

nexus_err_t nexus_can_receive(nexus_can_frame_t* frame, uint32_t timeout_ms) {
    if (!s_initialized || !frame) return NEXUS_ERR_INVALID_PARAM;
    NEXUS_LOG_DEBUG("CAN", "RX timeout=%u ms", timeout_ms);
    return NEXUS_OK;
}

nexus_err_t nexus_can_set_filter(uint32_t id, uint32_t mask) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_config.filter_id = id;
    s_config.filter_mask = mask;
    NEXUS_LOG_DEBUG("CAN", "Filter set: ID=0x%08X Mask=0x%08X", id, mask);
    return NEXUS_OK;
}

nexus_err_t nexus_can_flush(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    return NEXUS_OK;
}

bool nexus_can_is_busy(void) {
    return false;
}
