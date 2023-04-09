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
#define E_START  0x0080 
#define E_EFFECT 0x0100 
#define E_VEFX   0x0200
#define E_4      0x0400 
#define AUX_NO  0x0800 
#define AUX_YES 0x1000

#define LED_KEY_1 0
#define LED_KEY_2 1
#define LED_KEY_3 2
#define LED_KEY_4 3
#define LED_KEY_5 4
#define LED_KEY_6 5
#define LED_KEY_7 6
#define LED_E_START 7
#define LED_E_EFFECT 8
#define LED_E_VEFX 9
#define LED_E_4 10

#define PRESSED_ALL(k) ((input.keys & (k)) == (k))
#define PRESSED_ANY(k) (input.keys & (k))
#define JUST_PRESSED(k) (input.just_pressed & (k))

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

static void mode_none_loop()
{
    static bool escaped = false;
    static uint64_t escape_time = 0;

    if (PRESSED_ALL(AUX_NO | AUX_YES | E_START)) {
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

    uint16_t pressed = PRESSED_ANY(E_START | E_EFFECT | E_VEFX | E_4);
    if (pressed) {
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
    bool adjust_led_start;
    int16_t start_angle;
    uint8_t counter;
} tt_ctx;

void mode_tt_enter()
{
    tt_ctx.start_angle = input.angle;
}

static void mode_tt_key_change()
{
    if (JUST_PRESSED(E_START)) {
        tt_ctx.adjust_led_start = true;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E_EFFECT)) {
        tt_ctx.adjust_led_start = false;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E_VEFX)) {
        iidx_cfg->tt_led.reversed = !iidx_cfg->tt_led.reversed;
    } else if (JUST_PRESSED(E_4)) {
        iidx_cfg->tt_sensor_reversed = !iidx_cfg->tt_sensor_reversed;
    }

    check_exit();
}

static void mode_tt_rotate()
{
    int16_t delta = input.angle - tt_ctx.start_angle;
    if (abs(delta) > 8) {
        tt_ctx.start_angle = input.angle;

        #define LED_START iidx_cfg->tt_led.start
        #define LED_NUM iidx_cfg->tt_led.num

        if (tt_ctx.adjust_led_start) {
            if ((delta > 0) & (LED_START < 8)) {
                LED_START++;
                if (LED_NUM > 1) {
                    LED_NUM--;
                }
            } else if ((delta < 0) & (LED_START > 0)) {
                LED_START--;
                LED_NUM++;
            }
        } else {
            if ((delta > 0) & (LED_NUM + LED_START < 128)) {
                LED_NUM++;
            } else if ((delta < 0) & (LED_NUM > 1)) { // at least 1 led
                LED_NUM--;
            }
        }
    }
}

void mode_tt_loop()
{
    static uint32_t mask = 0xffffff;

    RUN_EVERY_N_MS(mask = ~mask, 50);

    for (int i = 1; i < iidx_cfg->tt_led.num - 1; i++) {
        setup_led_tt[i] = tt_rgb32(10, 10, 10, false);
    }
    int head = iidx_cfg->tt_led.reversed ? TT_LED_NUM - 1 : 0;
    int tail = iidx_cfg->tt_led.reversed ? 0 : TT_LED_NUM - 1;

    setup_led_tt[head] = tt_rgb32(0xa0, 0, 0, false);
    setup_led_tt[tail] = tt_rgb32(0, 0xa0, 0, false);

    if (tt_ctx.adjust_led_start) {
        setup_led_tt[head] &= mask;
        setup_led_button[LED_E_START] = tt_rgb32(128, 0, 0, false) & mask;
        setup_led_button[LED_E_EFFECT] = tt_rgb32(0, 10, 0, false);
    } else {
        setup_led_tt[tail] &= mask;
        setup_led_button[LED_E_START] = tt_rgb32(10, 0, 0, false);
        setup_led_button[LED_E_EFFECT] = tt_rgb32(0, 128, 0, false) & mask;
    }

    uint32_t cyan = button_rgb32(0, 90, 90, false);
    uint32_t yellow = button_rgb32(90, 90, 0, false);

    setup_led_button[LED_E_VEFX] = iidx_cfg->tt_led.reversed ? cyan : yellow;
    setup_led_button[LED_E_4] = iidx_cfg->tt_sensor_reversed ? cyan : yellow;
}

static struct {
    mode_func key_change;
    mode_func rotate;
    mode_func loop;
    mode_func enter;
} mode_defs[] = {
    [MODE_NONE] = { nop, nop, mode_none_loop, nop},
    [MODE_TURNTABLE] = { mode_tt_key_change, mode_tt_rotate, mode_tt_loop, mode_tt_enter},
    [MODE_ANALOG] = { nop, check_exit, nop, nop},
    [MODE_TT_EFFECT] = { nop, check_exit, nop, nop},
    [MODE_KEY_COLOR] = { nop, check_exit, nop, nop},
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