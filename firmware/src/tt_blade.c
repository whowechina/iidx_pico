/*
 * Turntable Blade Effect
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

static uint32_t blade_buf[128];

static void blade_clear()
{
    memset(blade_buf, 0, sizeof(blade_buf));
}

static inline uint32_t apply_rate(uint32_t c, uint32_t distance, uint32_t level)
{
    return c * (255 - distance) * level / 255 / 255;
}

static inline void blade_color_mix(int index, uint32_t color, uint32_t distance, uint32_t level)
{
    uint32_t c = blade_buf[index];
    uint32_t b = (c & 0xff) + apply_rate(color & 0xff, distance, level);
    uint32_t g = ((c >> 8) & 0xff) + apply_rate((color >> 8) & 0xff, distance, level);
    uint32_t r = ((c >> 16) & 0xff) + apply_rate((color >> 16) & 0xff, distance, level);
    b = (b < 256) ? b : 255;
    g = (g < 256) ? g : 255;
    r = (r < 256) ? r : 255;

    blade_buf[index] = (r << 16) | (g << 8) | (b << 0);
}

/* pos: 0..4095 */
static void blade_put_pixel(uint32_t pos, uint32_t color, uint32_t level)
{
    pos = (pos % 4096) * TT_LED_NUM / 16; // *256/4096, zoom in by 256x

    // calc brightness share between left and right LEDs
    uint32_t index_left = pos >> 8;
    uint32_t index_right = (index_left + 1) % TT_LED_NUM;
    uint32_t dis_left = pos & 0xff;
    uint32_t dis_right = 255 - dis_left;

    blade_color_mix(index_left, color, dis_left, level);
    blade_color_mix(index_right, color, dis_right, level);
}

static uint32_t color[] = {
    //rrggbb,
    0x808080,
    0xc0c040,
    0xffff00,
    0xfff000,
    0xffe000,
    0xffd000,
    0xffc000,
    0xffb000,
    0xffa000,
    0xff9000,
    0xff8000,
    0xff7000,
    0xff6000,
    0xff5000,
    0xff4000,
    0xff3000,
    0xff2000,
    0xff1000,

    0xff0000,
    0xc00000,
    0x800000,
    0x400000,
};

#define SNAKE_SIZE (sizeof(color) / sizeof(color[0]))
uint32_t snake[SNAKE_SIZE] = {0};
uint32_t life[SNAKE_SIZE] = {0};

static void init(uint32_t context)
{
}

static void set_level(uint32_t level)
{
}

static void set_angle(uint32_t angle)
{
}

static uint32_t apply_level(uint32_t color)
{
    uint32_t r = (color >> 16) & 0xff;
    uint32_t g = (color >> 8) & 0xff;
    uint32_t b = color  & 0xff;
    return tt_rgb32(r, g, b, false);
}

static void update(uint32_t context)
{
    uint32_t delta = tt_led_angle > snake[0] ? tt_led_angle - snake[0] : snake[0] - tt_led_angle;

    snake[0] = tt_led_angle;
    life[0] = 255;
    life[1] = delta > 7 ? 255: delta * 8;

    static uint64_t next_run = 0;
    uint64_t now = time_us_64();
    if (now > next_run) {
        next_run = now + 12000;
        for (int i = SNAKE_SIZE - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
            if (i > 1) {
                life[i] = life[i - 1];
            }
        }
    }
    blade_clear();
    for (int i = 0; i < SNAKE_SIZE; i++) {
        blade_put_pixel(snake[i], color[i], life[i]);
        if (life[i] > 0) {
            life[i]--;
        }
    }
    for (int i = 0; i < TT_LED_NUM; i++) {
        tt_led_buf[i] = apply_level(blade_buf[i]);
    }
}

void tt_blade_init()
{
    tt_effect_t blade = {
        init,
        set_level,
        set_angle,
        update,
        0,
    };

    rgb_reg_tt_effect(blade);
}
