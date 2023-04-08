/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 * 
 */

#include "rgb.h"

#include "buttons.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ws2812.pio.h"

#include "board_defs.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const uint8_t button_rgb_map[BUTTON_RGB_NUM] = BUTTON_RGB_MAP;
static const uint8_t level_val[] = {0, 32, 64, 96, 128, 160, 192, 224, 255};
uint32_t rgb_level = 0;

static void trap() {}
static tt_effect_t effects[10] = { {trap, trap, trap, trap, 0} };
static size_t effect_num = 0;
static size_t current_effect = 0;

#define _MAP_LED(x) _MAKE_MAPPER(x)
#define _MAKE_MAPPER(x) MAP_LED_##x
#define MAP_LED_RGB { c1 = r; c2 = g; c3 = b; }
#define MAP_LED_GRB { c1 = g; c2 = r; c3 = b; }

#define REMAP_BUTTON_RGB _MAP_LED(BUTTON_RGB_ORDER)
#define REMAP_TT_RGB _MAP_LED(TT_RGB_ORDER)

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    c1 = c1 * level_val[rgb_level] / 255;
    c2 = c2 * level_val[rgb_level] / 255;
    c3 = c3 * level_val[rgb_level] / 255;

    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }
    
    return (c1 << 16) | (c2 << 8) | (c3 << 0);    
}

uint32_t button_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint32_t tt_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if TT_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint8_t rgb_button_num()
{
    return BUTTON_RGB_NUM;
}

void rgb_set_level(uint8_t level)
{
    if (rgb_level == level) {
        return;
    }
    rgb_level = level;
    effects[current_effect].set_level(level);
}

uint8_t button_lights[BUTTON_RGB_NUM];

static uint32_t tt_led_buf[128] = {0};
uint32_t *tt_ring_buf = &tt_led_buf[0];
uint32_t tt_ring_start = 0;
uint32_t tt_ring_size = 24;
bool tt_ring_reversed = false;
uint32_t tt_ring_angle = 0;

static uint32_t button_led_buf[BUTTON_RGB_NUM] = {0};

void set_effect(uint32_t index)
{
    if (index < effect_num) {
        current_effect = index;
        effects[current_effect].init(effects[current_effect].context);
    }
}

void drive_led()
{
    for (int i = 0; i < ARRAY_SIZE(button_led_buf); i++) {
        pio_sm_put_blocking(pio0, 0, button_led_buf[i] << 8u);
    }

    for (int i = 0; i < ARRAY_SIZE(tt_led_buf); i++) {
        pio_sm_put_blocking(pio1, 0, tt_led_buf[i] << 8u);
    }
}

#define HID_EXPIRE_DURATION 1000000ULL
static uint64_t hid_expire_time = 0;

static void button_lights_update()
{
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        int led = button_rgb_map[i];
        if (button_lights[i] > 0) {
            button_led_buf[led] = button_rgb32(0x80, 0x80, 0x80, true);
        } else {
            button_led_buf[led] = 0;
        }
    }
}

void rgb_set_angle(uint32_t angle)
{
    tt_ring_angle = angle;
    effects[current_effect].set_angle(angle);
}

void rgb_set_button_light(uint16_t buttons)
{
    if (time_us_64() < hid_expire_time) {
        return;
    }
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        uint16_t flag = 1 << i;
        button_lights[i] = (buttons & flag) > 0 ? 0xff : 0;
    }
}

void rgb_set_hid_light(uint8_t const *lights, uint8_t num)
{
    memcpy(button_lights, lights, num);
    hid_expire_time = time_us_64() + HID_EXPIRE_DURATION;
}

static void effect_update()
{
    effects[current_effect].update(effects[current_effect].context);
}

#define FORCE_EXPIRE_DURATION 100000ULL
static uint64_t force_expire_time = 0;

void rgb_update()
{
    if (time_us_64() > force_expire_time) {
        effect_update();
        button_lights_update();
    }
    drive_led();
}

void rgb_force_display(uint32_t *keyboard, uint32_t *tt)
{
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        int led = button_rgb_map[i];
        button_led_buf[led] = keyboard[i];
    }

    memset(tt_led_buf, 0, tt_ring_start * sizeof(uint32_t));
    memcpy(tt_led_buf + tt_ring_start, tt, tt_ring_size * sizeof(uint32_t));

    force_expire_time = time_us_64() + FORCE_EXPIRE_DURATION;
}

void rgb_init()
{
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, BUTTON_RGB_PIN, 800000, false);

    offset = pio_add_program(pio1, &ws2812_program);
    ws2812_program_init(pio1, 0, offset, TT_RGB_PIN, 800000, false);
    rgb_set_level(8);
    set_effect(1);
}

void rgb_reg_tt_effect(tt_effect_t effect)
{
    effects[effect_num] = effect;
    effect_num++;
}

void rgb_set_hardware(uint16_t tt_start, uint16_t tt_num, bool tt_reversed)
{
    tt_ring_start = tt_start;
    tt_ring_size = tt_num;
    tt_ring_reversed = tt_reversed;
    tt_ring_buf = &tt_led_buf[tt_start];
}
