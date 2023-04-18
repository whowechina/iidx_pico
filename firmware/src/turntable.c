/*
 * IIDX Controller Turntable
 * WHowe <github.com/whowechina>
 * 
 */

#include "turntable.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#include "board_defs.h"
#include "config.h"

static uint16_t angle = 0;

static uint8_t current_mode = 0;

static void init_i2c()
{
    i2c_init(TT_AS5600_I2C, 573 * 1000);
    gpio_set_function(TT_AS5600_SCL, GPIO_FUNC_I2C);
    gpio_set_function(TT_AS5600_SDA, GPIO_FUNC_I2C);
    gpio_set_drive_strength(TT_AS5600_SCL, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(TT_AS5600_SDA, GPIO_DRIVE_STRENGTH_8MA);
    gpio_pull_up(TT_AS5600_SCL);
    gpio_pull_up(TT_AS5600_SDA);
}

static void init_analog()
{
    adc_init();
    adc_gpio_init(TT_AS5600_ANALOG);
    adc_select_input(TT_AS5600_ANALOG - 26);
}

static void follow_mode_change()
{
    if (current_mode != iidx_cfg->tt_sensor.mode) {
        turntable_init();
    }
}

void turntable_init()
{
    current_mode = iidx_cfg->tt_sensor.mode;
    if (current_mode & 0x02) {
        init_i2c();
    } else {
        init_analog();
    }
}

static uint32_t min_adc = 0;  /* idealy [0..3740] */
static uint32_t max_adc = 3740;
static bool min_touched = false;
static bool max_touched = false;

static inline void adjust_max(uint32_t value)
{
    if (value > max_adc) {
        max_adc += (value - max_adc + 1) / 2;
        printf("Auto adc max: %4lu %4lu\n", min_adc, max_adc);
    }
    max_touched = true;
}

static inline void adjust_min(uint32_t value)
{
    if (value < min_adc) {
        min_adc -= (min_adc - value + 1) / 2;
        printf("Auto adc min: %4lu %4lu\n", min_adc, max_adc);
    }
    min_touched = true;
}

static void auto_adjust_adc()
{
    if (!min_touched || !max_touched) {
        return;
    }
    min_touched = false;
    max_touched = false;

    if (max_adc > 3540) {
        max_adc--;
    }
    if (min_adc < 200) {
        min_adc++;
    }
    printf("Auto adc adj: %4lu %4lu\n", min_adc, max_adc);
}

static uint16_t read_average(uint16_t size)
{
    uint32_t large_cnt = 0;
    uint32_t small_cnt = 0;
    uint32_t large = 0;
    uint32_t small = 0;
    uint32_t medium = 0;

    for (int i = 0; i < size; i++) {
        uint32_t sample = adc_read();
        if (sample > 3540) {
            large_cnt++;
            large += sample;
        } else if (sample < 200) {
            small_cnt++;
            small += sample;
        } else {
            medium += sample;
        }
    }

    if (large_cnt > 50) {
        adjust_max(large / large_cnt);
    }

    if (small_cnt > 50) {
        adjust_min(small / small_cnt);
    }

    uint32_t all = large + small + medium;

    if (large_cnt && small_cnt) {
        all += small_cnt * max_adc;
    }

    return (all / size) % max_adc;
}

static void update_analog()
{
    static uint16_t sample = 0;

    auto_adjust_adc();

    uint16_t deadzone = (iidx_cfg->tt_sensor.deadzone + 1) * 16;

    int new_value = read_average(200);
    int delta = abs(new_value - sample);
    if ((abs(delta) < deadzone) || (abs(delta) > 4096 - deadzone)) {
        return;
    }
    sample = new_value;
    if (sample < min_adc) {
        angle = 0;
        return;
    }
    angle = (sample - min_adc) * 4095 / (max_adc - min_adc);
}

static void update_i2c()
{
    const uint8_t as5600_addr = 0x36;
    uint8_t buf[2] = {0x0c, 0x00};
    i2c_write_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 1, true,
                             make_timeout_time_ms(1));
    i2c_read_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 2, false,
                            make_timeout_time_ms(1));

    angle = ((uint16_t)buf[0] & 0x0f) << 8 | buf[1];
}

void turntable_update()
{
    follow_mode_change();
    if (current_mode) {
        update_analog();
    } else {
        update_i2c();
    }
}

uint16_t turntable_read()
{
    bool reversed = iidx_cfg->tt_sensor.mode & 0x01;
    return reversed ? 4095 - angle : angle; // 12bit
}
