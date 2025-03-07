/*
 * Turntable Rainbow Effect
 * WHowe <github.com/whowechina>
 * 
 */

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
            color_wheel[i] = RGB32(incr, 0, 255);
        } else if (sector == 1) {
            color_wheel[i] = RGB32(255, 0, decr);
        } else if (sector == 2) {
            color_wheel[i] = RGB32(255, incr, 0);
        } else if (sector == 3) {
            color_wheel[i] = RGB32(decr, 255, 0);
        } else if (sector == 4) {
            color_wheel[i] = RGB32(0, 255, incr);
        } else {
            color_wheel[i] = RGB32(0, decr, 255);
        }
        color_wheel[i] = rgb_gamma_fix(color_wheel[i]);
    }
}

static void init(uint32_t context)
{
    generate_color_wheel();
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
    int total_led = rgb_tt_led_num();
    unsigned cycle = 1 << (context % 3);
    for (int i = 0; i < total_led; i++) {
        uint32_t pitch = COLOR_WHEEL_SIZE * cycle * i;
        uint32_t index = (phase + pitch / total_led) % COLOR_WHEEL_SIZE;
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
