#ifndef NEXUS_UI_H
#define NEXUS_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"

typedef enum {
    NEXUS_SCREEN_SPLASH = 0,
    NEXUS_SCREEN_HOME,
    NEXUS_SCREEN_DISCOVER,
    NEXUS_SCREEN_DISCOVERY_RESULTS,
    NEXUS_SCREEN_ANALYZER,
    NEXUS_SCREEN_UART_ANALYZER,
    NEXUS_SCREEN_I2C_ANALYZER,
    NEXUS_SCREEN_SPI_ANALYZER,
    NEXUS_SCREEN_CAN_ANALYZER,
    NEXUS_SCREEN_GPIO_ANALYZER,
    NEXUS_SCREEN_WAVEFORM,
    NEXUS_SCREEN_CAPTURE,
    NEXUS_SCREEN_TERMINAL,
    NEXUS_SCREEN_MEASUREMENT,
    NEXUS_SCREEN_GENERATOR,
    NEXUS_SCREEN_TEST_ENGINE,
    NEXUS_SCREEN_TEST_LIST,
    NEXUS_SCREEN_TEST_EXECUTION,
    NEXUS_SCREEN_TEST_REPORT,
    NEXUS_SCREEN_DEVICE_DB,
    NEXUS_SCREEN_DEVICE_DETAIL,
    NEXUS_SCREEN_POWER_PROFILER,
    NEXUS_SCREEN_FILE_BROWSER,
    NEXUS_SCREEN_CALIBRATION,
    NEXUS_SCREEN_SELF_TEST,
    NEXUS_SCREEN_DIAGNOSTICS,
    NEXUS_SCREEN_SETTINGS,
    NEXUS_SCREEN_ABOUT,
} nexus_screen_t;

typedef enum {
    NEXUS_UI_EVT_TOUCH = 0,
    NEXUS_UI_EVT_BUTTON,
    NEXUS_UI_EVT_GESTURE,
    NEXUS_UI_EVT_TIMEOUT,
} nexus_ui_event_type_t;

typedef struct {
    nexus_ui_event_type_t type;
    uint16_t x;
    uint16_t y;
    uint8_t id;
    uint32_t param;
} nexus_ui_event_t;

nexus_err_t nexus_ui_init(void);
nexus_err_t nexus_ui_show_screen(nexus_screen_t screen);
nexus_err_t nexus_ui_handle_event(const nexus_ui_event_t* event);
nexus_err_t nexus_ui_update(void);
nexus_screen_t nexus_ui_get_current_screen(void);
nexus_err_t nexus_ui_show_notification(const char* msg, uint32_t duration_ms);
nexus_err_t nexus_ui_set_status(const char* status);

#endif
