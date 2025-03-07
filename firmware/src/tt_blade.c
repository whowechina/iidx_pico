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
    int total_led = rgb_tt_led_num();
    pos = (pos % 4096) * total_led / 16; // *256/4096, zoom in by 256x

    // calc brightness share between left and right LEDs
    uint32_t index_left = pos >> 8;
    uint32_t index_right = (index_left + 1) % total_led;
    uint32_t dis_left = pos & 0xff;
    uint32_t dis_right = 255 - dis_left;

    blade_color_mix(index_left, color, dis_left, level);
    blade_color_mix(index_right, color, dis_right, level);
}

static uint32_t spectrum[3][24] = {
    {
        0x808080, 0xc0c040, 0xffff00, 0xfff000,
        0xffe000, 0xffd000, 0xffc000, 0xffb000,
        0xffa000, 0xff9000, 0xff8000, 0xff7000,
        0xff6000, 0xff5000, 0xff4000, 0xff3000,
        0xff2000, 0xff1000, 0xff0000, 0xc00000,
        0x800000, 0x400000, 0x200000, 0x100000,
    },
    {
        0x808080, 0xc040c0, 0xff00ff, 0xf000ff,
        0xe000ff, 0xd000ff, 0xc000ff, 0xb000ff,
        0xa000ff, 0x9000ff, 0x8000ff, 0x7000ff,
        0x6000ff, 0x5000ff, 0x4000ff, 0x3000ff,
        0x2000ff, 0x1000ff, 0x0000ff, 0x0000c0,
        0x000080, 0x000040, 0x000020, 0x000010,
    },
    {
        0x808080, 0xc0c040, 0xffff00, 0xf0ff00,
        0xe0ff00, 0xd0ff00, 0xc0ff00, 0xb0ff00,
        0xa0ff00, 0x90ff00, 0x80ff00, 0x70ff00,
        0x60ff00, 0x50ff00, 0x40ff00, 0x30ff00,
        0x20ff00, 0x10ff00, 0x00ff00, 0x00c000,
        0x008000, 0x004000, 0x002000, 0x001000,
    },
};

#define SNAKE_SIZE (count_of(spectrum[0]))
static uint32_t snake[SNAKE_SIZE] = {0};
static uint32_t life[SNAKE_SIZE] = {0};

static uint32_t led_angle = 0;
static void set_angle(uint32_t context, uint32_t angle)
{
    led_angle = angle;
}

static void update(uint32_t context)
{
    uint32_t delta = led_angle > snake[0] ? led_angle - snake[0] : snake[0] - led_angle;

    snake[0] = led_angle;
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
        blade_put_pixel(snake[i], spectrum[context % 3][i], life[i]);
        if (life[i] > 0) {
            life[i]--;
        }
    }
    for (int i = 0; i < rgb_tt_led_num(); i++) {
        tt_led_buf[i] = blade_buf[i];
    }
}

void tt_blade_init()
{
    tt_effect_t blade = {
        .init = NULL,
        .set_angle = set_angle,
        .set_button = NULL,
        .update = update,
    };

    rgb_reg_tt_effect(blade);
}
