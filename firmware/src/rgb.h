/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 */

#ifndef RGB_H
#define RGB_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef enum {
    COLOR_MODE_HSV = 0,
    COLOR_MODE_RGB,
} color_mode_t;

typedef struct __attribute ((packed)) {
    color_mode_t mode;
    union {
        rgb_t rgb;
        hsv_t hsv;
    };
} color_t;

void rgb_init();
void rgb_update(uint32_t angle, uint16_t buttons);

void rgb_set_button_light(uint16_t buttons);
void rgb_set_hid_light(uint8_t const *lights, uint8_t num);

void rgb_override_tt(uint32_t *tt);
void rgb_override_button(uint32_t *button);
void rgb_force_light(int id, uint32_t color);

typedef struct {
    void (*init)(uint32_t context);
    void (*set_angle)(uint32_t context, uint32_t angle);
    void (*set_button)(uint32_t context, uint16_t buttons);
    void (*update)(uint32_t context);
} tt_effect_t;

void rgb_reg_tt_effect(tt_effect_t effect);

extern uint32_t tt_led_buf[];

typedef enum {
    RGB_MAIN,
    RGB_TT,
    RGB_EFFECT,
} rgb_type;

uint32_t rgb_mix(rgb_type type, uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t rgb_from_hsv(rgb_type type, hsv_t hsv);

uint32_t rgb_hsv_raw(hsv_t hsv);

#endif
