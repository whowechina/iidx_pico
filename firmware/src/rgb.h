/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 */

#ifndef RGB_H
#define RGB_H

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

void rgb_init();
void rgb_set_hardware(uint16_t tt_start, uint16_t tt_num, bool tt_reversed);

uint8_t rgb_button_num();
uint8_t rgb_hid_light_num();

void rgb_update();

void rgb_set_angle(uint32_t angle);
void rgb_set_button(uint16_t buttons);

void rgb_set_button_light(uint16_t buttons);
void rgb_set_hid_light(uint8_t const *lights, uint8_t num);

void rgb_override_tt(uint32_t *tt);
void rgb_override_button(uint32_t *button);
void rgb_force_light(int id, uint32_t color);

void rgb_set_effect(int id, uint32_t color);

typedef struct {
    void (*init)(uint32_t context);
    void (*set_angle)(uint32_t context, uint32_t angle);
    void (*set_button)(uint32_t context, uint16_t buttons);
    void (*update)(uint32_t context);
} tt_effect_t;

void rgb_reg_tt_effect(tt_effect_t effect);

extern uint32_t tt_led_buf[];
#define TT_LED_NUM (iidx_cfg->rgb.tt.num)

typedef enum {
    RGB_MAIN,
    RGB_TT,
    RGB_EFFECT,
} rgb_type;

uint32_t rgb_mix(rgb_type type, uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t rgb_from_hsv(rgb_type type, hsv_t hsv);

uint32_t rgb_hsv_raw(hsv_t hsv);
uint32_t rgb_fix_order(rgb_type type, uint32_t rgb);

// TODO: unify the color_t and rgb_t, tt and button
//uint32_t get_color(color_t color, rgb_type type);

#endif
