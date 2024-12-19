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

#include "bsp/board.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const uint8_t button_rgb_map[BUTTON_RGB_NUM] = BUTTON_RGB_MAP;

static tt_effect_t effects[7] = { };
static size_t effect_num = 0;
#define EFFECT_MAX count_of(effects)

static unsigned current_effect = 0;

#define _MAP_LED(x) _MAKE_MAPPER(x)
#define _MAKE_MAPPER(x) MAP_LED_##x
#define MAP_LED_RGB { c1 = r; c2 = g; c3 = b; }
#define MAP_LED_GRB { c1 = g; c2 = r; c3 = b; }

#define REMAP_BUTTON_RGB _MAP_LED(BUTTON_RGB_ORDER)
#define REMAP_TT_RGB _MAP_LED(TT_RGB_ORDER)

#define HID_EXPIRE_DURATION 1000000ULL
static uint64_t hid_light_button_expire = 0;
static uint64_t hid_light_tt_expire = 0;

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    c1 = c1 * iidx_cfg->level / 255;
    c2 = c2 * iidx_cfg->level / 255;
    c3 = c3 * iidx_cfg->level / 255;

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

static uint8_t hid_lights[BUTTON_RGB_NUM + 3];
static uint8_t *tt_hid = hid_lights + BUTTON_RGB_NUM;

uint8_t rgb_hid_light_num()
{
    return sizeof(hid_lights);
}

uint32_t tt_led_buf[128] = {0};
uint32_t tt_led_angle = 0;

static uint32_t button_led_buf[BUTTON_RGB_NUM] = {0};

static void fn_nop() {};
static void fn_tt_led_off()
{
    memset(tt_led_buf, 0, sizeof(tt_led_buf));
}

static void effect_reset()
{
    for (int i = 0; i < EFFECT_MAX; i++) {
        effects[i].init = fn_nop;
        effects[i].set_angle = fn_nop;
        effects[i].update = fn_tt_led_off;
        effects[i].context = 0;
    }
}

static void set_effect(uint32_t index)
{
    if (index < EFFECT_MAX) {
        if (current_effect != index) {
            current_effect = index;
            effects[current_effect].init(effects[current_effect].context);
        }
    }
}

static void drive_led()
{
    for (int i = 0; i < ARRAY_SIZE(button_led_buf); i++) {
        pio_sm_put_blocking(pio0, 0, button_led_buf[i] << 8);
    }

    if (iidx_cfg->tt_led.mode == 2) {
        return;
    }

    for (int i = 0; i < iidx_cfg->tt_led.start; i++) {
        pio_sm_put_blocking(pio1, 0, 0);
    }

    for (int i = 0; i < TT_LED_NUM; i++) {
        bool reversed = iidx_cfg->tt_led.mode & 0x01;
        uint8_t id = reversed ? TT_LED_NUM - i - 1 : i;
        pio_sm_put_blocking(pio1, 0, tt_led_buf[id] << 8);
    }

    for (int i = 0; i < 8; i++) { // a few more to wipe out the last leds
        pio_sm_put_blocking(pio1, 0, 0);
    }
}

static uint32_t rgb32_from_hsv(hsv_t hsv)
{
    uint32_t region, remainder, p, q, t;

    if (hsv.s == 0) {
        return hsv.v << 16 | hsv.v << 8 | hsv.v;
    }

    region = hsv.h / 43;
    remainder = (hsv.h % 43) * 6;

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            return hsv.v << 16 | t << 8 | p;
        case 1:
            return q << 16 | hsv.v << 8 | p;
        case 2:
            return p << 16 | hsv.v << 8 | t;
        case 3:
            return p << 16 | q << 8 | hsv.v;
        case 4:
            return t << 16 | p << 8 | hsv.v;
        default:
            return hsv.v << 16 | p << 8 | q;
    }
}

uint32_t button_hsv(hsv_t hsv)
{
    uint32_t rgb = rgb32_from_hsv(hsv);
    uint32_t r = (rgb >> 16) & 0xff;
    uint32_t g = (rgb >> 8) & 0xff;
    uint32_t b = (rgb >> 0) & 0xff;
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, false);
#else
    return _rgb32(r, g, b, false);
#endif
}

uint32_t tt_hsv(hsv_t hsv)
{
    uint32_t rgb = rgb32_from_hsv(hsv);
    uint32_t r = (rgb >> 16) & 0xff;
    uint32_t g = (rgb >> 8) & 0xff;
    uint32_t b = (rgb >> 0) & 0xff;
#if TT_RGB_ORDER == GRB
    return _rgb32(g, r, b, false);
#else
    return _rgb32(r, g, b, false);
#endif
}

void rgb_set_angle(uint32_t angle)
{
    tt_led_angle = angle;
    effects[current_effect].set_angle(angle);
}

void rgb_set_button_light(uint16_t buttons)
{
    if (time_us_64() < hid_light_button_expire) {
        return;
    }
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        uint16_t flag = 1 << i;
        hid_lights[i] = (buttons & flag) > 0 ? 0xff : 0;
    }
}

void rgb_set_hid_light(uint8_t const *lights, uint8_t num)
{
    if (num < sizeof(hid_lights)) {
        return;
    }

    memcpy(hid_lights, lights, sizeof(hid_lights));

    uint64_t now = time_us_64();
    
    hid_light_button_expire = now + HID_EXPIRE_DURATION;

    if (tt_hid[0] || tt_hid[1] || tt_hid[2]) {
        hid_light_tt_expire = now + HID_EXPIRE_DURATION;
    }
}

#define OVERRIDE_EXPIRE 100000ULL
static uint64_t override_tt_expire_time = 0;
static uint64_t override_button_expire_time = 0;
uint32_t *override_tt_buf = NULL;
uint32_t *override_button_buf = NULL;

static void tt_lights_update()
{
    uint64_t now = time_us_64();

    if (now < override_tt_expire_time) {
        memcpy(tt_led_buf, override_tt_buf, TT_LED_NUM * sizeof(uint32_t));
        return;
    }

    if (now < hid_light_tt_expire) {
        /* Higher priority for the HID lights */
        uint32_t color = tt_rgb32(tt_hid[0], tt_hid[1], tt_hid[2], false);
        for (int i = 0; i < TT_LED_NUM; i++) {
            tt_led_buf[i] = color;
        }
        return;
    }

    set_effect(iidx_cfg->tt_led.effect);
    /* Lower priority for the local effects */
    effects[current_effect].update(effects[current_effect].context);
}

static void button_lights_update()
{
    uint64_t now = time_us_64();
    if (now < override_button_expire_time) {
        for (int i = 0; i < BUTTON_RGB_NUM; i++) {
            int led = button_rgb_map[i];
            button_led_buf[led] = override_button_buf[i];
        }
        return;
    }

    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        int led = button_rgb_map[i];
        if (hid_lights[i] > 0) {
            button_led_buf[led] = button_hsv(iidx_cfg->key_on[i]);
        } else {
            button_led_buf[led] = button_hsv(iidx_cfg->key_off[i]);
        }
    }
}

void rgb_override_tt(uint32_t *tt)
{
    override_tt_buf = tt;
    override_tt_expire_time = time_us_64() + OVERRIDE_EXPIRE;
}

void rgb_override_button(uint32_t *button)
{
    override_button_buf = button;
    override_button_expire_time = time_us_64() + OVERRIDE_EXPIRE;
}

static void wipe_out_tt_led()
{
    sleep_ms(5);
    for (int i = 0; i < 128; i++) {
        pio_sm_put_blocking(pio1, 0, 0);
    }
    sleep_ms(5);
}

static uint pio1_offset;
static bool pio1_running = false;

static void pio1_run()
{
    gpio_set_drive_strength(TT_RGB_PIN, GPIO_DRIVE_STRENGTH_8MA);
    ws2812_program_init(pio1, 0, pio1_offset, TT_RGB_PIN, 800000, false);
}

static void pio1_stop()
{
    wipe_out_tt_led();
    pio_sm_set_enabled(pio1, 0, false);

    gpio_set_function(TT_RGB_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(TT_RGB_PIN, GPIO_IN);
    gpio_disable_pulls(TT_RGB_PIN);
}

void rgb_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);
    pio1_offset = pio_add_program(pio1, &ws2812_program);

    gpio_set_drive_strength(BUTTON_RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, BUTTON_RGB_PIN, 800000, false);

    effect_reset();
}

static void follow_mode_change()
{
    bool pio1_should_run = (iidx_cfg->tt_led.mode != 2);
    if (pio1_should_run == pio1_running) {
        return;
    }
    pio1_running = pio1_should_run;
    if (pio1_should_run) {
        pio1_run();
    } else {
        pio1_stop();
    }
}

void rgb_update()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 4000) { // no faster than 250Hz
        return;
    }
    last = now;

    follow_mode_change();

    tt_lights_update();
    button_lights_update();

    drive_led();
}

static void trap()
{
    return;
}

void rgb_reg_tt_effect(tt_effect_t effect)
{
    if (effect_num >= EFFECT_MAX) {
        return;
    }
    effects[effect_num] = effect;
    effect_num++;
}
