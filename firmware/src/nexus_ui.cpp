#include "nexus_ui.h"
#include "nexus_log.h"
#include <string.h>

static bool s_initialized = false;
static nexus_screen_t s_current_screen = NEXUS_SCREEN_SPLASH;

nexus_err_t nexus_ui_init(void) {
    s_initialized = true;
    s_current_screen = NEXUS_SCREEN_SPLASH;
    NEXUS_LOG_INFO("UI", "UI framework initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_ui_show_screen(nexus_screen_t screen) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_current_screen = screen;
    NEXUS_LOG_DEBUG("UI", "Showing screen %d", screen);
    return NEXUS_OK;
}

nexus_err_t nexus_ui_handle_event(const nexus_ui_event_t* event) {
    if (!s_initialized || !event) return NEXUS_ERR_INVALID_PARAM;
    NEXUS_LOG_DEBUG("UI", "Event type=%d x=%u y=%u", event->type, event->x, event->y);
    return NEXUS_OK;
}

nexus_err_t nexus_ui_update(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    return NEXUS_OK;
}

nexus_screen_t nexus_ui_get_current_screen(void) {
    return s_current_screen;
}

nexus_err_t nexus_ui_show_notification(const char* msg, uint32_t duration_ms) {
    if (!s_initialized || !msg) return NEXUS_ERR_INVALID_PARAM;
    NEXUS_LOG_INFO("UI", "Notification: %s (%u ms)", msg, duration_ms);
    return NEXUS_OK;
}

nexus_err_t nexus_ui_set_status(const char* status) {
    if (!s_initialized || !status) return NEXUS_ERR_INVALID_PARAM;
    NEXUS_LOG_DEBUG("UI", "Status: %s", status);
    return NEXUS_OK;
}
