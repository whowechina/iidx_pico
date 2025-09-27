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

static tt_style_t styles[4] = { };
static size_t style_num = 0;
#define STYLE_MAX count_of(styles)

static unsigned current_style_id = 0;
#define CURRENT_STYLE (styles[(current_style_id & 0x0f) % 4])
#define CURRENT_CONTEXT ((current_style_id >> 4) % 3)

#define HID_EXPIRE_DURATION 1000000ULL
static uint64_t hid_light_button_expire = 0;
static uint64_t hid_light_tt_expire = 0;

uint32_t rgb_gamma_fix(uint32_t rgb)
{
    int r = (rgb >> 16) & 0xff;
    int g = (rgb >> 8) & 0xff;
    int b = (rgb >> 0) & 0xff;

    r = ((r + 1) * (r + 1) - 1) >> 8;
    g = ((g + 1) * (g + 1) - 1) >> 8;
    b = ((b + 1) * (b + 1) - 1) >> 8;

    return RGB32(r, g, b);
}

uint32_t rgb_apply_level(uint32_t rgb, uint8_t level)
{
    int r = (rgb >> 16) & 0xff;
    int g = (rgb >> 8) & 0xff;
    int b = (rgb >> 0) & 0xff;

    r = r * level / 255;
    g = g * level / 255;
    b = b * level / 255;

    return RGB32(r, g, b);
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

static void style_reset()
{
    for (int i = 0; i < STYLE_MAX; i++) {
        styles[i].init = fn_nop;
        styles[i].set_angle = fn_nop;
        styles[i].update = fn_tt_led_off;
    }
}

static void set_style(uint32_t style_id)
{
    current_style_id = style_id;
    if (CURRENT_STYLE.init) {
        CURRENT_STYLE.init(CURRENT_CONTEXT);
    }
}

static uint32_t fix_order(uint8_t order, uint32_t rgb)
{
    uint8_t c1 = (rgb >> 16) & 0xff;
    uint8_t c2 = (rgb >> 8) & 0xff;
    uint8_t b = (rgb >> 0) & 0xff;

    return order ? rgb : RGB32(c2, c1, b);
}

static inline uint32_t mix_level(uint32_t rgb, uint8_t level)
{
    unsigned r = (rgb >> 16) & 0xff;
    unsigned g = (rgb >> 8) & 0xff;
    unsigned b = (rgb >> 0) & 0xff;

    r = r * level / 255;
    g = g * level / 255;
    b = b * level / 255;

    return RGB32(r, g, b);  
}

static uint32_t final_key_buf[BUTTON_RGB_NUM];
static uint32_t final_tt_buf[128];

static void prepare_buf()
{
    for (int i = 0; i < 7; i++) {
        uint32_t c = mix_level(button_led_buf[i], PROFILE.level.keys);
        final_key_buf[i] =  fix_order(iidx_cfg->rgb.format.main, c) << 8;
    }
    for (int i = 7; i < 11; i++) {
        uint32_t c = mix_level(button_led_buf[i], PROFILE.level.keys);
        final_key_buf[i] = fix_order(iidx_cfg->rgb.format.effect, c) << 8;
    }

    for (int i = 0; i < iidx_cfg->rgb.tt.num; i++) {
        uint8_t id = iidx_cfg->rgb.tt.reversed ? iidx_cfg->rgb.tt.num - i - 1 : i;
        uint32_t c = mix_level(tt_led_buf[id], PROFILE.level.tt);
        final_tt_buf[i] = fix_order(iidx_cfg->rgb.format.tt, c) << 8;
    }
}

static void drive_led()
{
    for (int i = 0; i < count_of(final_key_buf); i++) {
        pio_sm_put_blocking(pio0, 0, final_key_buf[i]);
    }

    for (int i = 0; i < iidx_cfg->rgb.tt.start; i++) {
        pio_sm_put_blocking(pio0, 1, 0);
    }

    for (int i = 0; i < iidx_cfg->rgb.tt.num; i++) {
        pio_sm_put_blocking(pio0, 1, final_tt_buf[i]);
    }

    for (int i = 0; i < 8; i++) {
        // a few more to wipe out the last leds
        pio_sm_put_blocking(pio0, 1, 0);
    }
}

uint32_t rgb_from_hsv(hsv_t hsv)
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

static void set_angle(uint32_t angle)
{
    if (CURRENT_STYLE.set_angle) {
        CURRENT_STYLE.set_angle(CURRENT_CONTEXT, angle);
    }
}

static void set_button(uint16_t buttons)
{
    if (CURRENT_STYLE.set_button) {
        CURRENT_STYLE.set_button(CURRENT_CONTEXT, buttons);
    }
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
    if (num > sizeof(hid_lights)) {
        return;
    }

    memcpy(hid_lights, lights, num);

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
        memcpy(tt_led_buf, override_tt_buf, iidx_cfg->rgb.tt.num * sizeof(uint32_t));
        return;
    }

    if (now < hid_light_tt_expire) {
        /* Higher priority for the HID lights */
        uint32_t color = RGB32(tt_hid[0], tt_hid[1], tt_hid[2]);
        for (int i = 0; i < iidx_cfg->rgb.tt.num; i++) {
            tt_led_buf[i] = color;
        }
        return;
    }

    set_style(PROFILE.tt_style);

    /* Lower priority for the local styles */
    if (CURRENT_STYLE.update) {
        CURRENT_STYLE.update(CURRENT_CONTEXT);
    }
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
        hsv_t hsv = hid_lights[i] ? PROFILE.key_on[i].hsv
                                  : PROFILE.key_off[i].hsv;
        button_led_buf[led] = rgb_from_hsv(hsv);
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

void rgb_init(bool alternative_gpio)
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);

    uint button_led_pin = BUTTON_RGB_PIN;
    gpio_set_drive_strength(button_led_pin, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, button_led_pin, 800000, false);

    uint tt_led_pin = alternative_gpio ? TT_RGB_PIN_2 : TT_RGB_PIN;
    gpio_set_drive_strength(tt_led_pin, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 1, pio0_offset, tt_led_pin, 800000, false);

    if (alternative_gpio) {
        gpio_set_function(TT_RGB_PIN, GPIO_FUNC_NULL);
        gpio_set_dir(TT_RGB_PIN, GPIO_IN);
        gpio_disable_pulls(TT_RGB_PIN);
    }

    style_reset();
}

static bool forced[11];
static uint32_t forced_color[11];

void rgb_force_light(int id, uint32_t color)
{
    if (id >= 11) {
        return;
    }

    forced[id] = true;
    forced_color[id] = color;
}

static void forced_lights()
{
    for (int i = 0; i < 11; i++) {
        if (forced[i]) {
            button_led_buf[button_rgb_map[i]] = forced_color[i];
            forced[i] = false;
        }
    }
}

void rgb_update(uint32_t angle, uint16_t buttons)
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 4000) { // no faster than 250Hz
        return;
    }
    last = now;

    set_angle(angle);
    set_button(buttons);

    tt_lights_update();
    button_lights_update();

    forced_lights();

    prepare_buf();
    drive_led();
}

static void trap()
{
    return;
}

void rgb_reg_tt_style(tt_style_t style)
{
    if (style_num >= STYLE_MAX) {
        return;
    }
    styles[style_num] = style;
    style_num++;
}

uint8_t rgb_tt_led_num()
{
    return iidx_cfg->rgb.tt.num;
}
