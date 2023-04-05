/*
 * Controller Setup
 * WHowe <github.com/whowechina>
 */

#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute ((packed)) {
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
} key_color_t;

typedef struct __attribute ((packed)) {
    key_color_t key_off[11];
    key_color_t key_on[11];
    struct {
        uint8_t led_start;
        uint8_t led_num;
        uint8_t effect;
        uint8_t param;
        uint8_t brightness;
        bool reversed;
    } tt_led;
    bool tt_sensor_reversed;
    struct {
        uint8_t vefx;
        uint8_t eq_low;
        uint8_t eq_hi;
        uint8_t filter;
        uint8_t play_vol;
    } effects;
} iidx_cfg_t;

void report_moves(uint16_t key_flag, uint16_t tt_angle);
bool is_in_setup();

#endif
