/*
 * Turntable Heatbar Style
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

static uint32_t gauge = 0; // indicates the heat level

static uint32_t tt_angle;
static void set_angle(uint32_t context, uint32_t angle)
{
    tt_angle = angle >> 4;
}

static uint32_t tt_button;
static void set_button(uint32_t context, uint16_t buttons)
{
    static uint16_t old_button = 0;
    if (buttons == old_button) {
        return;
    }
    uint16_t just_pressed = buttons & ~old_button;
    int count = __builtin_popcount(just_pressed) * 3;
    gauge += count;
    if (gauge > 255) {
        gauge = 255;
    }

    old_button = buttons;
}

static uint32_t spectrum[256];

static void prepare_spectrum()
{
    for (int i = 0; i < 256; i++) {
        spectrum[i] = rgb_from_hsv((hsv_t){ i, 255, 255 });
    }
}

static void render(uint32_t context)
{
    prepare_spectrum();

    context = context % 3 + 1;
    int total_led = rgb_tt_led_num();
    int led_num = total_led / context; 
    uint32_t gauge_pos = gauge * led_num / context;

    memset(tt_led_buf, 0, total_led * sizeof(uint32_t));
    for (int i = 0; i < led_num ; i++) {
        for (int ctx = 0; ctx < context; ctx++) {
            int offset = total_led * ctx / context;
            int pos = (offset + tt_angle * total_led / 256) % total_led;
            if (i <= gauge_pos) {
                tt_led_buf[pos] = spectrum[i * 255 / led_num];
            }
        }
    }
}

#define DROP_RATE_MS 20
static void update(uint32_t context)
{
    static uint64_t next_drop = 0;
    uint64_t now = time_us_64();
    if (now >= next_drop) {
        next_drop = now + DROP_RATE_MS * 1000;
        gauge = gauge > 0 ? gauge - 1 : 0;
    }

    render(context);
}

void tt_heatbar_init()
{
    tt_style_t heatbar = {
        .init = NULL,
        .set_angle = set_angle,
        .set_button = set_button,
        .update = update,
    };

    rgb_reg_tt_style(heatbar);
}
