/*
 * Controller Setup
 * WHowe <github.com/whowechina>
 */

#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include <stdbool.h>
#include "board_defs.h"

typedef struct __attribute ((packed)) {
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
} key_color_t;

typedef struct __attribute ((packed)) {
    key_color_t key_off[11];
    key_color_t key_on[11];
    struct {
        uint8_t start;
        uint8_t num;
        uint8_t effect;
        uint8_t param;
        uint8_t brightness;
        bool reversed;
    } tt_led;
    bool tt_sensor_reversed;
    struct {
        uint8_t play_vol;
        uint8_t filter;
        uint8_t eq_low;
        uint8_t eq_hi;
    } effects;
} iidx_cfg_t;

extern iidx_cfg_t iidx_cfg;

void setup_init();
bool setup_run(uint16_t key_flag, uint16_t tt_angle);

extern uint32_t setup_led_tt[];
extern uint32_t setup_led_button[BUTTON_RGB_NUM];

#endif
