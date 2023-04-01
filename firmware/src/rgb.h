/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 */

#ifndef RGB_H
#define RGB_H

#include <stdint.h>
#include <stdbool.h>

void rgb_init();
uint8_t rgb_button_num();
void rgb_update();
void rgb_set_angle(uint32_t angle);
void rgb_set_level(uint8_t level);

void rgb_set_button_light(uint16_t buttons);
void rgb_set_hid_light(uint8_t const *lights, uint8_t num);

typedef struct {
    void (*init)(uint32_t context);
    void (*set_level)(uint32_t level);
    void (*set_angle)(uint32_t angle);
    void (*update)(uint32_t context);
    uint32_t context;
} tt_effect_t;

void rgb_reg_tt_effect(tt_effect_t effect);

extern uint32_t *rgb_tt_buf;
extern uint32_t rgb_tt_size;
extern uint32_t rgb_tt_angle;
extern bool rgb_tt_reversed;

uint32_t button_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t tt_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);

#endif
