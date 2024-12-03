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

#include "as5600.h"
#include "tmag5273.h"

#include "board_defs.h"
#include "config.h"

static uint16_t raw_angle = 0;
static bool use_as5600 = true;

void turntable_init()
{
    i2c_init(TT_SENSOR_I2C, 400 * 1000);
    gpio_init(TT_SENSOR_SCL);
    gpio_init(TT_SENSOR_SDA);
    gpio_set_function(TT_SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_set_function(TT_SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(TT_SENSOR_SCL);
    gpio_pull_up(TT_SENSOR_SDA);

    tmag5273_init(0, TT_SENSOR_I2C);
    if (tmag5273_is_present(0)) {
        tmag5273_use(0);
        tmag5273_init_sensor();
        use_as5600 = false;
        return;
    }

    as5600_init(TT_SENSOR_I2C);
    as5600_init_sensor();
    use_as5600 = true;
}

static int read_angle()
{
    if (use_as5600) {
        return as5600_read_angle();
    }

    return tmag5273_read_angle() * 0x1000 / 360 / 16;
}

void turntable_update()
{
    int candidate = read_angle();
    if (candidate < 0) {
        return;
    }

    for (int i = 0; i < 2; i++) {
        int update = read_angle();

        if (abs(update - candidate) > 2) {
            return;
        }
    
        if (update > candidate) {
            candidate++;
        } else if (update < candidate) {
            candidate--;
        }
    }

    raw_angle = candidate;
}

uint16_t turntable_raw()
{
    return iidx_cfg->tt_sensor.reversed ? 4095 - raw_angle : raw_angle; // 12bit
}

uint8_t turntable_read()
{
    static uint8_t counter = 0;
    static int16_t old_angle = 0;

    uint16_t step;
    if (iidx_cfg->tt_sensor.ppr == 1) {
        step = 4096 / 128;
    } else if (iidx_cfg->tt_sensor.ppr == 2) {
        step = 4096 / 96;
    } else if (iidx_cfg->tt_sensor.ppr == 3) {
        step = 4096 / 64;
    } else {
        return raw_angle >> 4;
    }

    int16_t delta = raw_angle - old_angle;
    if (delta == 0) {
        return counter;
    } else if (delta > 2048) {
        delta -= 4096;
    } else if (delta < -2048) {
        delta += 4096;
    }

    if (abs(delta) >= step) {
        if (delta > 0) {
            counter++;
            old_angle += step;
        } else {
            counter--;
            old_angle -= step;
        }
        if (old_angle > 4096) {
            old_angle -= 4096;
        } else if (old_angle < 0) {
            old_angle += 4096;
        }
    }

    return counter;
}