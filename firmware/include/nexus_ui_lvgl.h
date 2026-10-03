#ifndef NEXUS_UI_LVGL_H
#define NEXUS_UI_LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include "nexus_errors.h"
#include "nexus_ui.h"

// LVGL display and touch integration for STM32F746G-DISCO
// 480x272 RGB LCD with capacitive touch

#define NEXUS_LCD_WIDTH 480
#define NEXUS_LCD_HEIGHT 272
#define NEXUS_LCD_BPP 16

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t* frame_buffer;
    bool double_buffer;
} nexus_lcd_config_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    bool pressed;
} nexus_touch_data_t;

nexus_err_t nexus_ui_lvgl_init(void);
nexus_err_t nexus_ui_lvgl_flush(void);
nexus_err_t nexus_ui_lvgl_read_touch(nexus_touch_data_t* data);
nexus_err_t nexus_ui_lvgl_set_brightness(uint8_t brightness);
nexus_err_t nexus_ui_lvgl_render_screen(nexus_screen_t screen);
nexus_err_t nexus_ui_lvgl_update(void);

#endif
