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
} tt_style_t;

void rgb_reg_tt_style(tt_style_t style);
uint8_t rgb_tt_led_num();

extern uint32_t tt_led_buf[];

#define RGB32(r, g, b) (((r) << 16) | ((g) << 8) | (b))
uint32_t rgb_from_hsv(hsv_t hsv);
uint32_t rgb_gamma_fix(uint32_t rgb);

#define RED RGB32(99, 0, 0)
#define BLUE RGB32(0, 0, 128)
#define GREEN RGB32(0, 99, 0)
#define CYAN RGB32(0, 40, 99)
#define YELLOW RGB32(99, 99, 0)
#define SILVER RGB32(60, 60, 60)

#endif
