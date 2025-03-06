/*
 * Turntable Rainbow Effect
 * WHowe <github.com/whowechina>
 * 
 */

#include "buttons.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/timer.h"
#include "rgb.h"

#define RGB_RING_CYCLE 2

/* 6 segment regular hsv color wheel, better color cycle
 * https://www.arnevogel.com/rgb-rainbow/
 * https://www.instructables.com/How-to-Make-Proper-Rainbow-and-Random-Colors-With-/
 */
#define COLOR_WHEEL_SIZE (256 * 6)
static uint32_t color_wheel[COLOR_WHEEL_SIZE];
static void generate_color_wheel()
{
    for (int i = 0; i < COLOR_WHEEL_SIZE; i++) {
        uint32_t sector = i / 256 % 6;
        uint8_t incr = i % 256;
        uint8_t decr = 255 - incr;
        if (sector == 0) {
            color_wheel[i] = rgb_mix(RGB_TT, incr, 0, 255, true);
        } else if (sector == 1) {
            color_wheel[i] = rgb_mix(RGB_TT, 255, 0, decr, true);
        } else if (sector == 2) {
            color_wheel[i] = rgb_mix(RGB_TT, 255, incr, 0, true);
        } else if (sector == 3) {
            color_wheel[i] = rgb_mix(RGB_TT, decr, 255, 0, true);
        } else if (sector == 4) {
            color_wheel[i] = rgb_mix(RGB_TT, 0, 255, incr, true);
        } else {
            color_wheel[i] = rgb_mix(RGB_TT, 0, decr, 255, true);
        }
    }
}

static uint8_t old_tt_level = 0;

static void init(uint32_t context)
{
    generate_color_wheel();
    old_tt_level = PROFILE.level.tt;
}

static uint32_t phase = 0;

static void set_angle(uint32_t context, uint32_t angle)
{
    angle >>= 4;
    unsigned cycle = 1 << (context % 3);
    phase = COLOR_WHEEL_SIZE * cycle * (256 - angle) / 256;
}

static void update(uint32_t context)
{
    if (old_tt_level != PROFILE.level.tt) {
        old_tt_level = PROFILE.level.tt;
        generate_color_wheel();
        return;
    }

    unsigned cycle = 1 << (context % 3);
    for (int i = 0; i < TT_LED_NUM; i++) {
        uint32_t pitch = COLOR_WHEEL_SIZE * cycle * i;
        uint32_t index = (phase + pitch / TT_LED_NUM) % COLOR_WHEEL_SIZE;
        tt_led_buf[i] = color_wheel[index];
    }
}

void tt_rainbow_init()
{
    tt_effect_t rainbow = {
        .init = init,
        .set_angle = set_angle,
        .set_button = NULL,
        .update = update,
    };
    rgb_reg_tt_effect(rainbow);
}
