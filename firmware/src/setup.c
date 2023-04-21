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

static uint64_t setup_tick_ms = 0;
#define CONCAT(a, b) a ## b
#define TVAR(line) CONCAT(a, line)
#define RUN_EVERY_N_MS(a, ms) { static uint64_t TVAR(__LINE__) = 0; \
    if (setup_tick_ms - TVAR(__LINE__) >= ms) { a; TVAR(__LINE__) = setup_tick_ms; } }

uint32_t setup_led_tt[128];
uint32_t setup_led_button[BUTTON_RGB_NUM];

typedef enum {
    MODE_NONE,
    MODE_TURNTABLE,
    MODE_ANALOG,
    MODE_TT_EFFECT,
    MODE_KEY_COLOR,
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
#define E1  0x0080 
#define E2 0x0100 
#define E3   0x0200
#define E4      0x0400 
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

#define RED button_rgb32(99, 0, 0, false)
#define GREEN button_rgb32(0, 99, 0, false)
#define CYAN button_rgb32(0, 40, 99, false)
#define YELLOW button_rgb32(99, 99, 0, false)
#define SILVER button_rgb32(60, 60, 60, false)

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

static void none_loop()
{
    static bool escaped = false;
    static uint64_t escape_time = 0;

    if (PRESSED_ALL(AUX_NO | AUX_YES | KEY_1 | KEY_3 | KEY_5 | KEY_7)) {
        reset_usb_boot(0, 2); // usb boot to flash
    }

    if (PRESSED_ALL(AUX_YES | AUX_NO)) {
        if (!escaped) {
            escaped = true;
            escape_time = time_us_64();
        }
    } else {
        escaped = false;
    }

    if (!escaped) {
        return;
    }

    if (PRESSED_ANY(E1 | E2 | E3 | E4)) {
        escaped = false;
        join_mode(MODE_ANALOG);
        return;
    }

    if (time_us_64() - escape_time > 5000000) {
        escaped = false;
        join_mode(MODE_TURNTABLE);
    }
}

static struct {
    uint8_t adjust_led; /* 0: nothing, 1: adjust start, 2: adjust stop */
    int16_t start_angle;
} tt_ctx;

static void tt_enter()
{
    tt_ctx.start_angle = input.angle;
}

static void tt_key_change()
{
    if (JUST_PRESSED(E1)) {
        tt_ctx.adjust_led = (tt_ctx.adjust_led == 1) ? 0 : 1;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E2)) {
        tt_ctx.adjust_led = (tt_ctx.adjust_led == 2) ? 0 : 2;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E3)) {
        iidx_cfg->tt_led.mode = (iidx_cfg->tt_led.mode + 1) % 3;
    } else if (JUST_PRESSED(E4)) {
        iidx_cfg->tt_sensor.mode = (iidx_cfg->tt_sensor.mode + 1) % 4;
    } else if (JUST_PRESSED(KEY_2)) {
        iidx_cfg->tt_sensor.deadzone = 0;
    } else if (JUST_PRESSED(KEY_4)) {
        iidx_cfg->tt_sensor.deadzone = 1;
    } else if (JUST_PRESSED(KEY_6)) {
        iidx_cfg->tt_sensor.deadzone = 2;
    }

    check_exit();
}

static void tt_rotate()
{
    int16_t delta = input.angle - tt_ctx.start_angle;
    if (abs(delta) > 8) {
        tt_ctx.start_angle = input.angle;

        #define LED_START iidx_cfg->tt_led.start
        #define LED_NUM iidx_cfg->tt_led.num

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

static void tt_loop()
{
    static uint32_t mask = 0xffffff;

    RUN_EVERY_N_MS(mask = ~mask, 50);

    for (int i = 1; i < iidx_cfg->tt_led.num - 1; i++) {
        setup_led_tt[i] = tt_rgb32(10, 10, 10, false);
    }

    bool led_reversed = (iidx_cfg->tt_led.mode == 1);
    int head = led_reversed ? TT_LED_NUM - 1 : 0;
    int tail = led_reversed ? 0 : TT_LED_NUM - 1;

    setup_led_tt[head] = tt_rgb32(0xa0, 0, 0, false);
    setup_led_tt[tail] = tt_rgb32(0, 0xa0, 0, false);
    setup_led_button[LED_E2] = tt_rgb32(0, 10, 0, false);
    setup_led_button[LED_E1] = tt_rgb32(10, 0, 0, false);

    if (tt_ctx.adjust_led == 1) {
        setup_led_tt[head] &= mask;
        setup_led_button[LED_E1] = tt_rgb32(128, 0, 0, false) & mask;
    } else if (tt_ctx.adjust_led == 2) {
        setup_led_tt[tail] &= mask;
        setup_led_button[LED_E2] = tt_rgb32(0, 128, 0, false) & mask;
    }

    switch (iidx_cfg->tt_led.mode) {
        case 0:
            setup_led_button[LED_E3] = GREEN;
            break;
        case 1:
            setup_led_button[LED_E3] = RED;
            break;
        default:
            setup_led_button[LED_E3] = 0;
            break;
    }

    switch (iidx_cfg->tt_sensor.mode) {
        case 0:
            setup_led_button[LED_E4] = GREEN;
            break;
        case 1:
            setup_led_button[LED_E4] = RED;
            break;
        case 2:
            setup_led_button[LED_E4] = CYAN;
            break;
        default:
            setup_led_button[LED_E4] = YELLOW;
            break;
    }

    setup_led_button[LED_KEY_2] = iidx_cfg->tt_sensor.deadzone == 0 ? SILVER : 0;
    setup_led_button[LED_KEY_4] = iidx_cfg->tt_sensor.deadzone == 1 ? SILVER : 0;
    setup_led_button[LED_KEY_6] = iidx_cfg->tt_sensor.deadzone == 2 ? SILVER : 0;
}

static struct {
    uint8_t channel; /* 0:E1(Start), 1:E2(Effect), 2:E3(VEFX), 3:E4 */
    uint8_t *value;
    int16_t start_angle;
} analog_ctx;

static void analog_key_change()
{
    if (JUST_PRESSED(E1)) {
        analog_ctx.channel = 0;
        analog_ctx.value = &iidx_cfg->effects.e1;
    } else if (JUST_PRESSED(E2)) {
        analog_ctx.channel = 1;
        analog_ctx.value = &iidx_cfg->effects.e2;
    } else if (JUST_PRESSED(E3)) {
        analog_ctx.channel = 2;
        analog_ctx.value = &iidx_cfg->effects.e3;
    } else if (JUST_PRESSED(E4)) {
        analog_ctx.channel = 3;
        analog_ctx.value = &iidx_cfg->effects.e4;
    }
    check_exit();
}

static void analog_enter()
{
    analog_key_change();
}

static void analog_rotate()
{
    int16_t new_value = *analog_ctx.value;
    new_value += input.rotate;
    if (new_value < 0) {
        new_value = 0;
    } else if (new_value > 255) {
        new_value = 255;
    }
    *analog_ctx.value = new_value;
}

static void analog_loop()
{
    static uint32_t mask = 0xffffff;

    RUN_EVERY_N_MS(mask = ~mask, 50);

    setup_led_button[LED_E1] = RED;
    setup_led_button[LED_E2] = GREEN;
    setup_led_button[LED_E3] = CYAN;
    setup_led_button[LED_E4] = YELLOW;

    uint32_t color;
    if (analog_ctx.channel == 1) {
        color = GREEN;
        setup_led_button[LED_E2] &= mask;
    } else if (analog_ctx.channel == 2) {
        color = CYAN;
        setup_led_button[LED_E3] &= mask;
    } else if (analog_ctx.channel == 3) {
        color = YELLOW;
        setup_led_button[LED_E4] &= mask;
    } else {
        color = RED;
        setup_led_button[LED_E1] &= mask;
    }

    int split = (int)*analog_ctx.value * iidx_cfg->tt_led.num / 255;

    for (int i = 1; i < iidx_cfg->tt_led.num - 1; i++) {
        setup_led_tt[i] = i < split ? color : 0;
    }
}

static struct {
    mode_func key_change;
    mode_func rotate;
    mode_func loop;
    mode_func enter;
} mode_defs[] = {
    [MODE_NONE] = { nop, nop, none_loop, nop},
    [MODE_TURNTABLE] = { tt_key_change, tt_rotate, tt_loop, tt_enter},
    [MODE_ANALOG] = { analog_key_change, analog_rotate, analog_loop, analog_enter},
    [MODE_TT_EFFECT] = { nop, nop, check_exit, nop},
    [MODE_KEY_COLOR] = { nop, nop, check_exit, nop},
};

static void join_mode(setup_mode_t new_mode)
{
    cfg_save = *iidx_cfg;
    memset(&setup_led_tt, 0, sizeof(setup_led_tt));
    memset(&setup_led_button, 0, sizeof(setup_led_button));
    current_mode = new_mode;
    mode_defs[current_mode].enter();
    printf("Entering setup %d\n", new_mode);
}

static void quit_mode(bool apply)
{
    if (apply) {
        config_changed();
    } else {
        *iidx_cfg = cfg_save;
    }
    current_mode = MODE_NONE;
    printf("Quit setup %s\n", apply ? "saved." : "discarded.");
}
 
bool setup_run(uint16_t keys, uint16_t angle)
{
    setup_tick_ms = time_us_64() / 1000;
    input.keys = keys;
    input.angle = angle;
    input.just_pressed = keys & ~input.last_keys;
    input.just_released = ~keys & input.last_keys;
    input.rotate = input.angle - input.last_angle;
    if (input.rotate > 128) {
        input.rotate -= 256;
    } else if (input.rotate < -128) {
        input.rotate += 256;
    }

    if (input.rotate != 0) {
        printf("@ %3d %2x\n", input.rotate, input.angle);
        mode_defs[current_mode].rotate();
    }
    if (input.just_pressed) {
        printf("+ %04x\n", input.just_pressed);
    }
    if (input.just_released) {
        printf("- %04x\n", input.just_released);
    }

    if (input.just_pressed || input.just_released) {
        mode_defs[current_mode].key_change();
    }

    mode_defs[current_mode].loop();

    input.last_keys = keys;
    input.last_angle = angle;

    return current_mode != MODE_NONE;    
}

void setup_init()
{
}