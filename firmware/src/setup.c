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

#include "rgb.h"
#include "config.h"

iidx_cfg_t iidx_cfg;
static iidx_cfg_t cfg_save;

uint32_t setup_led_tt[128];
uint32_t setup_led_button[BUTTON_RGB_NUM];

static void cfg_loaded()
{
    /* configuration validation */
}

void setup_init()
{
    config_alloc(sizeof(iidx_cfg), &iidx_cfg, cfg_loaded);
}

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
    uint16_t last_angle;
    uint16_t keys;
    int16_t angle;
    uint16_t just_pressed;
    uint16_t just_released;
    int16_t rotate;
} input = { 0 };

#define KEY_1   0x0001
#define KEY_2   0x0002
#define KEY_3   0x0004      
#define KEY_4   0x0008 
#define KEY_5   0x0010 
#define KEY_6   0x0020
#define KEY_7   0x0040 
#define START   0x0080 
#define EFFECT  0x0100 
#define VEFX    0x0200
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
#define LED_START 7
#define LED_EFFECT 8
#define LED_VEFX 9
#define LED_E4 10


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

    uint16_t pressed = PRESSED_ANY(START | EFFECT | VEFX | E4);
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
    uint16_t start_angle;
    uint8_t counter;
} tt_ctx;

void mode_tt_enter()
{
    tt_ctx.start_angle = input.angle;
}

void mode_tt_op()
{
    if (JUST_PRESSED(START)) {
        tt_ctx.adjust_led_start = true;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(EFFECT)) {
        tt_ctx.adjust_led_start = false;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(VEFX)) {
        iidx_cfg.tt_led.reversed = !iidx_cfg.tt_led.reversed;
        tt_ctx.counter = iidx_cfg.tt_led.num;
    } else if (JUST_PRESSED(E4)) {
        iidx_cfg.tt_sensor_reversed = !iidx_cfg.tt_sensor_reversed;
        tt_ctx.counter = iidx_cfg.tt_led.num;
    }

    int16_t delta = input.angle - tt_ctx.start_angle;
    if (abs(delta) < 4) {
        return;
    }

    tt_ctx.start_angle = input.angle;

    if (tt_ctx.adjust_led_start) {
        if ((delta > 0) && (iidx_cfg.tt_led.start < 8)) {
            iidx_cfg.tt_led.start++;
        } else if ((delta < -8) && (iidx_cfg.tt_led.start > 0)) {
            iidx_cfg.tt_led.start--;
        }
    } else {
        if ((delta < 0) && (iidx_cfg.tt_led.num < 128)) {
            iidx_cfg.tt_led.num++;
        } else if ((delta < -8) && (iidx_cfg.tt_led.num > 0)) {
            iidx_cfg.tt_led.num--;
        }
    }

    if (iidx_cfg.tt_led.start + iidx_cfg.tt_led.num > 128) {
        iidx_cfg.tt_led.num = 128 - iidx_cfg.tt_led.start;
    }
    check_exit();
}

void mode_tt_loop()
{
    for (int i = 0; i < iidx_cfg.tt_led.num; i++) {
        int index = iidx_cfg.tt_led.start + i;
        setup_led_tt[index] = tt_rgb32(10, 10, 10, false);
    }

    setup_led_tt[iidx_cfg.tt_led.start] = tt_rgb32(0xa0, 0, 0, false);
    setup_led_tt[iidx_cfg.tt_led.start + iidx_cfg.tt_led.num -1 ] = tt_rgb32(0, 0xa0, 0, false);

    if (tt_ctx.adjust_led_start) {
        setup_led_button[LED_START] = tt_rgb32(0x80, 12, 12, false);
        setup_led_button[LED_EFFECT] = 0;
    } else {
        setup_led_button[LED_START] = 0;
        setup_led_button[LED_EFFECT] = tt_rgb32(12, 0x80, 12, false);
    }

    if (iidx_cfg.tt_led.reversed) {
        setup_led_button[LED_VEFX] = tt_rgb32(0x80, 12, 12, false);
    } else {
        setup_led_button[LED_VEFX] = tt_rgb32(12, 0x80, 12, false);
    }

    if (iidx_cfg.tt_sensor_reversed) {
        setup_led_button[LED_E4] = tt_rgb32(0x80, 12, 12, false);
    } else {
        setup_led_button[LED_E4] = tt_rgb32(12, 0x80, 12, false);
    }
}

static struct {
    mode_func operate;
    mode_func loop;
    mode_func enter;
} mode_defs[] = {
    [MODE_NONE] = { nop, mode_none_loop, nop},
    [MODE_TURNTABLE] = { mode_tt_op, mode_tt_loop, mode_tt_enter},
    [MODE_ANALOG] = { check_exit, nop, nop},
    [MODE_TT_EFFECT] = { check_exit, nop, nop},
    [MODE_KEY_COLOR] = { check_exit, nop, nop},
};

static void join_mode(uint8_t new_mode)
{
    cfg_save = iidx_cfg;
    memset(&setup_led_tt, 0, sizeof(setup_led_tt));
    memset(&setup_led_button, 0, sizeof(setup_led_button));
    current_mode = new_mode;
    mode_defs[current_mode].enter();
    printf("Entering setup %d\n", new_mode);
}

static void quit_mode(bool apply)
{
    if (apply) {
        iidx_cfg = iidx_cfg;
        config_request_save();
    }
    current_mode = MODE_NONE;
    printf("Quit setup\n");
}

bool setup_run(uint16_t keys, uint16_t angle)
{
    input.keys = keys;
    input.angle = angle;
    input.just_pressed = keys & ~input.last_keys;
    input.just_released = ~keys & input.last_keys;
    input.rotate = angle - input.last_angle;

    if (input.just_pressed || input.just_released) {
        printf("%4x %4x\n", input.just_pressed, input.just_released);
    }

    if (input.just_pressed || input.just_released || input.rotate) {
        mode_defs[current_mode].operate();
    }

    mode_defs[current_mode].loop();

    input.last_keys = keys;
    input.last_angle = angle;

    return current_mode != MODE_NONE;    
}
