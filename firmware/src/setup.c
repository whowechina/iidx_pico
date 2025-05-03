/*
 * Controller Setup Menu
 * WHowe <github.com/whowechina>
 * 
 * Setup is a mode, so one can change settings live
 */

#include "setup.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "pico/bootrom.h"

#include "rgb.h"
#include "config.h"

static iidx_cfg_t cfg_save;

#define PROFILE_SAVE cfg_save.profiles[cfg_save.profile]

static uint64_t setup_tick_ms = 0;
#define CONCAT(a, b) a ## b
#define TVAR(line) CONCAT(a, line)
#define RUN_EVERY_N_MS(a, ms) { static uint64_t TVAR(__LINE__) = 0; \
    if (setup_tick_ms - TVAR(__LINE__) >= ms) { a; TVAR(__LINE__) = setup_tick_ms; } }

static uint32_t blink_rapid = 0xffffffff;
static uint32_t blink_fast = 0xffffffff;
static uint32_t blink_slow = 0xffffffff;

uint32_t setup_led_tt[128];
uint32_t setup_led_button[BUTTON_RGB_NUM];

typedef enum {
    MODE_NONE,
    MODE_HWSET,
    MODE_LEVEL,
    MODE_TT_STYLE,
    MODE_KEY_THEME,
    MODE_KEY_OFF,
    MODE_KEY_ON,
} setup_mode_t;
static setup_mode_t current_mode = MODE_NONE;

static struct {
    uint16_t last_keys;
    uint16_t keys;
    uint16_t just_pressed;
    uint16_t just_released;

    int16_t last_angle;
    int16_t angle;
    int16_t rotate;
} input = { 0 };

#define KEY_1 0x0001
#define KEY_2 0x0002
#define KEY_3 0x0004      
#define KEY_4 0x0008 
#define KEY_5 0x0010 
#define KEY_6 0x0020
#define KEY_7 0x0040 
#define E1    0x0080 
#define E2    0x0100 
#define E3    0x0200
#define E4    0x0400 
#define AUX_NO  0x0800 
#define AUX_YES 0x1000

#define LED_KEY_1 0
#define LED_KEY_2 1
#define LED_KEY_3 2
#define LED_KEY_4 3
#define LED_KEY_5 4
#define LED_KEY_6 5
#define LED_KEY_7 6
#define LED_E1 7
#define LED_E2 8
#define LED_E3 9
#define LED_E4 10

#define PRESSED_ALL(k) ((input.keys & (k)) == (k))
#define PRESSED_ANY(k) (input.keys & (k))
#define JUST_PRESSED(k) (input.just_pressed & (k))
#define JUST_RELEASED(k) (input.just_released & (k))

typedef void (*mode_func)();

static void join_mode(setup_mode_t new_mode);
static void quit_mode(bool apply);

static void nop()
{
}

static void check_exit()
{
    if (JUST_PRESSED(AUX_YES)) {
        quit_mode(true);
    } else if (JUST_PRESSED(AUX_NO)) {
        quit_mode(false);
    }
}

static int16_t input_delta(int16_t start_angle)
{
    int16_t delta = input.angle - start_angle;
    if (delta > 128) {
        delta -= 256;
    }
    if (delta < -128) {
        delta += 256;
    }

    return delta;
}

static setup_mode_t key_to_mode[7] = {
    MODE_KEY_THEME, MODE_TT_STYLE, MODE_KEY_ON, MODE_KEY_OFF,
    MODE_NONE, MODE_NONE, MODE_NONE,
};

static struct {
    bool escaped;
    uint64_t escape_time;
    uint16_t start_angle;
} none_ctx = { 0 };

static void none_rotate()
{
    if (!none_ctx.escaped) {
        return;
    }

    int16_t delta = input_delta(none_ctx.start_angle);
    if (abs(delta) > 50) {
        join_mode(MODE_LEVEL);
        none_ctx.escaped = false;
    }
}

static void none_disp_profile()
{
    for (int i = 0; i < 4; i++) {
        hsv_t hsv = { i * 64, 240, i == iidx_cfg->profile ? 100 : 30 };
        uint32_t color = rgb_from_hsv(hsv);
        color &= (i == iidx_cfg->profile) ? blink_rapid : 0xffffffff;
        rgb_force_light(LED_E1 + i, color);
    }
}

static void none_loop()
{
    if (PRESSED_ALL(AUX_YES | AUX_NO)) {
        if (!none_ctx.escaped) {
            none_ctx.escaped = true;
            none_ctx.escape_time = time_us_64();
            none_ctx.start_angle = input.angle;
        }
        none_disp_profile();
    } else {
        none_ctx.escaped = false;
    }

    if (!none_ctx.escaped) {
        return;
    }

    for (int i = 0; i < count_of(key_to_mode); i++) {
        if (PRESSED_ANY(KEY_1 << i)) {
            none_ctx.escaped = false;
            join_mode(key_to_mode[i]);
            return;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (JUST_PRESSED(E1 << i)) {
            iidx_cfg->profile = i;
            none_ctx.escaped = false;
            config_changed();
            return;
        }
    }

    if (time_us_64() - none_ctx.escape_time > 5000000) {
        none_ctx.escaped = false;
        join_mode(MODE_HWSET);
        return;
    }
}

static struct {
    uint8_t adjust_led; /* 0: nothing, 1: adjust start, 2: adjust stop */
    int16_t start_angle;
} tt_ctx;

static void hwset_enter()
{
    tt_ctx.adjust_led = 0;
    tt_ctx.start_angle = input.angle;
}

#define TOGGLE_PPR(ppr, n) ppr = (ppr != n) ? n : n + 4;
static void hwset_key_change()
{
    if (JUST_PRESSED(E1)) {
        tt_ctx.adjust_led = (tt_ctx.adjust_led == 1) ? 0 : 1;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E2)) {
        tt_ctx.adjust_led = (tt_ctx.adjust_led == 2) ? 0 : 2;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E3)) {
        iidx_cfg->rgb.tt.reversed ^= 1;
    } else if (JUST_PRESSED(E4)) {
        iidx_cfg->sensor.reversed ^= 1;
    } else if (JUST_PRESSED(KEY_1)) {
        TOGGLE_PPR(iidx_cfg->sensor.ppr, 0);
    } else if (JUST_PRESSED(KEY_3)) {
        TOGGLE_PPR(iidx_cfg->sensor.ppr, 1);
    } else if (JUST_PRESSED(KEY_5)) {
        TOGGLE_PPR(iidx_cfg->sensor.ppr, 2);
    } else if (JUST_PRESSED(KEY_7)) {
        TOGGLE_PPR(iidx_cfg->sensor.ppr, 3);
    } else if (JUST_PRESSED(KEY_2)) {
        iidx_cfg->rgb.format.main = (iidx_cfg->rgb.format.main + 1) % 2;
    } else if (JUST_PRESSED(KEY_4)) {
        iidx_cfg->rgb.format.tt = (iidx_cfg->rgb.format.tt + 1) % 2;
    } else if (JUST_PRESSED(KEY_6)) {
        iidx_cfg->rgb.format.effect = (iidx_cfg->rgb.format.effect + 1) % 2;
    }

    if (JUST_PRESSED(AUX_YES | AUX_NO)) {
        PROFILE.level = PROFILE_SAVE.level;
    }

    check_exit();
}

static void hwset_rotate()
{
    int16_t delta = input_delta(tt_ctx.start_angle);
    if (abs(delta) > 8) {
        tt_ctx.start_angle = input.angle;

        #define LED_START iidx_cfg->rgb.tt.start
        #define LED_NUM iidx_cfg->rgb.tt.num

        if (tt_ctx.adjust_led == 1) {
            if ((delta > 0) & (LED_START < 8)) {
                LED_START++;
                if (LED_NUM > 1) {
                    LED_NUM--;
                }
            } else if ((delta < 0) & (LED_START > 0)) {
                LED_START--;
                LED_NUM++;
            }
        } else if (tt_ctx.adjust_led == 2) {
            if ((delta > 0) & (LED_NUM + LED_START < 128)) {
                LED_NUM++;
            } else if ((delta < 0) & (LED_NUM > 1)) { // at least 1 led
                LED_NUM--;
            }
        }
    }
}

static void hwset_loop()
{
    int total_led = iidx_cfg->rgb.tt.num;
    for (int i = 1; i < total_led - 1; i++) {
        setup_led_tt[i] = RGB32(10, 10, 10);
    }

    int head = iidx_cfg->rgb.tt.reversed ? total_led - 1 : 0;
    int tail = total_led - 1 - head;

    setup_led_tt[head] = RGB32(0xc0, 0, 0);
    setup_led_tt[tail] = RGB32(0, 0, 0xc0);
    setup_led_button[LED_E1] = RED;
    setup_led_button[LED_E2] = BLUE;

    if (tt_ctx.adjust_led == 1) {
        setup_led_tt[head] &= blink_fast;
        setup_led_button[LED_E1] &= blink_fast;
    } else if (tt_ctx.adjust_led == 2) {
        setup_led_tt[tail] &= blink_fast;
        setup_led_button[LED_E2] &= blink_fast;
    }

    setup_led_button[LED_E3] = iidx_cfg->rgb.tt.reversed ? CYAN : YELLOW;
    setup_led_button[LED_E4] = iidx_cfg->sensor.reversed ? CYAN : YELLOW;

    uint32_t ppr_color = (iidx_cfg->sensor.ppr > 3) ? BLUE : SILVER;
    int ppr = iidx_cfg->sensor.ppr & 3;
    setup_led_button[LED_KEY_1] = ppr == 0 ? ppr_color : 0;
    setup_led_button[LED_KEY_3] = ppr == 1 ? ppr_color : 0;
    setup_led_button[LED_KEY_5] = ppr == 2 ? ppr_color : 0;
    setup_led_button[LED_KEY_7] = ppr == 3 ? ppr_color : 0;

    setup_led_button[LED_KEY_2] = (iidx_cfg->rgb.format.main & 1) ? RED : BLUE;
    setup_led_button[LED_KEY_4] = (iidx_cfg->rgb.format.tt & 1) ? RED : BLUE;
    setup_led_button[LED_KEY_6] = (iidx_cfg->rgb.format.effect & 1) ? RED : BLUE;
}

static struct {
    bool adjust_tt;
    bool adjust_keys;
    uint8_t value;
} level_ctx;

static void level_update()
{
    PROFILE.level.tt = level_ctx.adjust_tt ? level_ctx.value : PROFILE_SAVE.level.tt;
    PROFILE.level.keys = level_ctx.adjust_keys ? level_ctx.value : PROFILE_SAVE.level.keys;
}

static void level_rotate()
{
    int16_t new_value = level_ctx.value;

    new_value += input.rotate > 0 ? 1 : -1;
    if (new_value < 0) {
        new_value = 0;
    } else if (new_value > 255) {
        new_value = 255;
    }
    level_ctx.value = new_value;

    level_update();
}

static void level_key_change()
{
    int short_cut = -1;
    if (JUST_PRESSED(KEY_1)) {
        short_cut = 0;
    } else if (JUST_PRESSED(KEY_2)) {
        short_cut = 20;
    } else if (JUST_PRESSED(KEY_3)) {
        short_cut = 50;
    } else if (JUST_PRESSED(KEY_4)) {
        short_cut = 85;
    } else if (JUST_PRESSED(KEY_5)) {
        short_cut = 130;
    } else if (JUST_PRESSED(KEY_6)) {
        short_cut = 190;
    } else if (JUST_PRESSED(KEY_7)) {
        short_cut = 255;
    } else if (JUST_PRESSED(E1 | E2)) {
        level_ctx.adjust_tt = PRESSED_ANY(E1);
        level_ctx.adjust_keys = PRESSED_ANY(E2);
    }
    if (short_cut >= 0) {
        level_ctx.value = short_cut;
    }
    level_update();

    check_exit();
}

static void level_loop()
{
    for (int i = 0; i < 7; i++) {
        hsv_t color = {i * 255 / 7, 255, 255};
        setup_led_button[i] = rgb_from_hsv(color);
    }

    setup_led_button[LED_E1] = level_ctx.adjust_tt ? (0x404040 & blink_rapid) : 0;
    setup_led_button[LED_E2] = level_ctx.adjust_keys ? (0x404040 & blink_rapid) : 0;

    for (unsigned i = 0; i < iidx_cfg->rgb.tt.num; i++) {
        hsv_t color = { i * 255 / iidx_cfg->rgb.tt.num, 255, 255 };
        setup_led_tt[i] = rgb_from_hsv(color);
    }
}

static void level_enter()
{
    level_ctx.adjust_keys = true;
    level_ctx.adjust_tt = true;
    level_ctx.value = 128;
}

static struct {
    uint8_t phase; /* 0:H, 1:S, 2:V */
    hsv_t hsv;
    uint8_t *value;
    int16_t start_angle;
    uint16_t keys;
    color_t *leds;
} key_ctx;

static void key_apply()
{
    for (int i = 0; i < 11; i++) {
        if (key_ctx.keys & (1 << i)) {
            key_ctx.leds[i].mode = 0;
            key_ctx.leds[i].hsv = key_ctx.hsv;
        }
    }
}

static void key_change()
{
    if (JUST_PRESSED(AUX_NO)) {
        quit_mode(false);
        return;
    }

    if (JUST_PRESSED(AUX_YES)) {
        key_ctx.phase++;
        if (key_ctx.phase == 3) {
            key_apply();
            quit_mode(true);
            return;
        }

        if (key_ctx.phase == 1) {
            key_ctx.value = &key_ctx.hsv.s;
        } else {
            key_ctx.value = &key_ctx.hsv.v;
        }
        return;
    }

    key_ctx.keys ^= input.just_pressed;
}

static void key_rotate()
{
    int16_t new_value = *key_ctx.value;
    new_value += input.rotate;
    if (key_ctx.phase > 0) {
        if (new_value < 0) {
            new_value = 0;
        } else if (new_value > 255) {
            new_value = 255;
        }
    }
    *key_ctx.value = (uint8_t)new_value;
}

static void key_loop()
{
    for (int i = 0; i < 11; i ++) {
        uint32_t rgb = rgb_from_hsv(key_ctx.hsv);
        if (key_ctx.keys == 0) {
            setup_led_button[i] = rgb & blink_slow;
        } else if (key_ctx.keys & (1 << i)) {
            setup_led_button[i] = rgb;
        } else {
            setup_led_button[i] = 0;
        }
    }

    uint16_t pos = *key_ctx.value * iidx_cfg->rgb.tt.num / 256;
    for (unsigned i = 0; i < iidx_cfg->rgb.tt.num; i++) {
        setup_led_tt[i] = (i == pos) ? RGB32(90, 90, 90) : 0;
    }
}

static void key_enter()
{
    key_ctx = (typeof(key_ctx)) {
        .phase = 0,
        .hsv = { .h = 200, .s = 255, .v = 128 },
        .value = &key_ctx.hsv.h,
        .start_angle = input.angle,
        .keys = 0,
        .leds = PROFILE.key_on,
    };

    if (current_mode == MODE_KEY_OFF) {
        key_ctx.hsv = (hsv_t) { .h = 60, .s = 255, .v = 100 };
        key_ctx.leds = PROFILE.key_off;
    }
}

#define K0_WHITE {.v = 5}
#define K0_SKY {.h = 215, .s = 255, .v = 20}
#define K0_RED {.h = 87, .s = 255, .v = 20}
#define K0_GREEN {.h = 0, .s = 255, .v = 20}

#define K1_WHITE {.v = 200}
#define K1_SKY {.h = 215, .s = 255, .v = 230}
#define K1_RED {.h = 87, .s = 255, .v = 230}
#define K1_GREEN {.h = 0, .s = 255, .v = 230}

#define K0_RAINBOW(x) {.h = 23 * x, .s = 255, .v = 20}
#define K1_RAINBOW(x) {.h = 23 * x, .s = 255, .v = 230}

static struct {
    hsv_t key_off[11];
    hsv_t key_on[11];
} themes[7] = {
    {{ { 0 }, },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE },
    },
    {{ { 0 }, },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_RED, K1_GREEN, K1_GREEN, K1_SKY },
    },
    {{ { 0 }, },
     { K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, 
       K1_RED, K1_GREEN, K1_GREEN, K1_GREEN },
    },
    {{ K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, 
       K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE },
    },
    {{ K0_RED, K0_SKY, K0_RED, K0_SKY, K0_RED, K0_SKY, K0_RED, 
       K0_RED, K0_GREEN, K0_GREEN, K0_GREEN },
     { K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, 
       K1_RED, K1_GREEN, K1_GREEN, K1_GREEN },
    },
    {{ K0_RAINBOW(0), K0_RAINBOW(1), K0_RAINBOW(2), K0_RAINBOW(3),
       K0_RAINBOW(4), K0_RAINBOW(5), K0_RAINBOW(6),
       K0_RAINBOW(7), K0_RAINBOW(8), K0_RAINBOW(9), K0_RAINBOW(10)
     },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE },
    },
    {{ K0_RAINBOW(0), K0_RAINBOW(1), K0_RAINBOW(2), K0_RAINBOW(3),
       K0_RAINBOW(4), K0_RAINBOW(5), K0_RAINBOW(6),
       K0_RAINBOW(7), K0_RAINBOW(8), K0_RAINBOW(9), K0_RAINBOW(10)
     },
     { K1_RAINBOW(0), K1_RAINBOW(1), K1_RAINBOW(2), K1_RAINBOW(3),
       K1_RAINBOW(4), K1_RAINBOW(5), K1_RAINBOW(6),
       K1_RAINBOW(7), K1_RAINBOW(8), K1_RAINBOW(9), K1_RAINBOW(10)
     }
    },
};

static void key_theme_key_change()
{
    int theme = -1;
    for (int i = 0; i < 7; i++) {
        if (JUST_PRESSED(KEY_1 << i)) {
            theme = i;
            break;
        }
    }

    if (theme >= 0) {
        for (int i = 0; i < 11; i++) {
            PROFILE.key_on[i].mode = COLOR_MODE_HSV;
            PROFILE.key_off[i].mode = COLOR_MODE_HSV;
            PROFILE.key_on[i].hsv = themes[theme].key_on[i];
            PROFILE.key_off[i].hsv = themes[theme].key_off[i];
        }
    }

    check_exit();
}

static void key_theme_loop()
{
    for (int i = 0; i < 11; i++) {
        hsv_t hsv = blink_slow ? PROFILE.key_on[i].hsv
                               : PROFILE.key_off[i].hsv;
        setup_led_button[i] = rgb_from_hsv(hsv);
    }
}

static void tt_style_key_change()
{
    for (int i = 0; i < 4; i++) {
        if (JUST_PRESSED(KEY_1 << (i * 2))) {
            PROFILE.tt_style = PROFILE.tt_style & 0xf0 | i;
            break;
        }
    }

    for (int i = 0; i < 3; i++) {
        if (JUST_PRESSED(KEY_2 << (i * 2))) {
            PROFILE.tt_style = PROFILE.tt_style & 0x0f | (i << 4);
            break;
        }
    }
    check_exit();
}

static void tt_style_loop()
{
    int style = (PROFILE.tt_style & 0x0f) % 4;
    int context = (PROFILE.tt_style >> 4) % 3;

    for (int i = 0; i < 7; i++) {
        if (i == style * 2) {
            setup_led_button[i] = SILVER;
        } else if (i == context * 2 + 1) {
            setup_led_button[i] = CYAN;
        } else {
            setup_led_button[i] = 0;
        }
    }
}

static struct {
    mode_func key_change;
    mode_func rotate;
    mode_func loop;
    mode_func enter;
    bool tt_led;
    bool button_led;
    bool auto_brightness;
} mode_defs[] = {
    [MODE_NONE] = { nop, none_rotate, none_loop, nop, false, false, false },
    [MODE_HWSET] = { hwset_key_change, hwset_rotate, hwset_loop, hwset_enter, true, true, true },
    [MODE_LEVEL] = { level_key_change, level_rotate, level_loop, level_enter, true, true, false },
    [MODE_TT_STYLE] = { tt_style_key_change, nop, tt_style_loop, nop, false, true, true },
    [MODE_KEY_THEME] = { key_theme_key_change, nop, key_theme_loop, nop, false, true, true },
    [MODE_KEY_OFF] = { key_change, key_rotate, key_loop, key_enter, true, true, true },
    [MODE_KEY_ON] = { key_change, key_rotate, key_loop, key_enter, true, true, true },
};

static void join_mode(setup_mode_t new_mode)
{
    cfg_save = *iidx_cfg;
    memset(&setup_led_tt, 0, sizeof(setup_led_tt));
    memset(&setup_led_button, 0, sizeof(setup_led_button));
    current_mode = new_mode;
    mode_defs[current_mode].enter();
    if (mode_defs[current_mode].auto_brightness) {
        PROFILE.level.tt = 100;
        PROFILE.level.keys = 100;
    }
    printf("Entering setup mode %d\n", new_mode);
}

static void quit_mode(bool apply)
{
    printf("Setup %s\n", apply ? "accepted." : "discarded.");

    if (apply) {
        if (mode_defs[current_mode].auto_brightness) {
            PROFILE.level = PROFILE_SAVE.level;
        }
        config_changed();
    } else {
        *iidx_cfg = cfg_save;
    }
    current_mode = MODE_NONE;
}
 
void setup_run(uint16_t keys, uint16_t angle)
{
    int16_t rotate = input_delta(input.last_angle);

    setup_tick_ms = time_us_64() / 1000;
    input.keys = keys;
    input.angle = angle;
    input.just_pressed = keys & ~input.last_keys;
    input.just_released = ~keys & input.last_keys;

    input.rotate = input_delta(input.last_angle);
    if (input.rotate != 0) {
        mode_defs[current_mode].rotate();
    }

    if (input.just_pressed || input.just_released) {
        mode_defs[current_mode].key_change();
    }

    RUN_EVERY_N_MS(blink_rapid ^= 0xffffffff, 75);
    RUN_EVERY_N_MS(blink_fast ^= 0xffffffff, 100);
    RUN_EVERY_N_MS(blink_slow ^= 0xffffffff, 500);

    mode_defs[current_mode].loop();

    input.last_keys = keys;
    input.last_angle = angle;
}

bool setup_needs_tt_led()
{
    return mode_defs[current_mode].tt_led;
}

bool setup_needs_button_led()
{
    return mode_defs[current_mode].button_led;
}

void setup_init()
{
}