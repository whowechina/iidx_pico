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

static i2c_inst_t *sensor_i2c = TT_SENSOR_I2C;
static bool sensor_is_as5600 = true;

static void init_port(bool use_primary)
{
    if (use_primary) {
        gpio_init(TT_SENSOR_SCL);
        gpio_init(TT_SENSOR_SDA);
        gpio_set_function(TT_SENSOR_SCL, GPIO_FUNC_I2C);
        gpio_set_function(TT_SENSOR_SDA, GPIO_FUNC_I2C);
        gpio_pull_up(TT_SENSOR_SCL);
        gpio_pull_up(TT_SENSOR_SDA);
    } else {
        gpio_init(TT_SENSOR_SCL_2);
        gpio_init(TT_SENSOR_SDA_2);
        gpio_set_function(TT_SENSOR_SCL_2, GPIO_FUNC_I2C);
        gpio_set_function(TT_SENSOR_SDA_2, GPIO_FUNC_I2C);
        gpio_pull_up(TT_SENSOR_SCL_2);
        gpio_pull_up(TT_SENSOR_SDA_2);
    }
    i2c_init(sensor_i2c, TT_SENSOR_I2C_FREQ);
}

static bool identify_sensor()
{
    tmag5273_init(0, sensor_i2c);
    if (tmag5273_is_present(0)) {
        tmag5273_use(0);
        tmag5273_init_sensor();
        sensor_is_as5600 = false;
        return true;
    }
    as5600_init(sensor_i2c);
    if (as5600_is_present(sensor_i2c)) {
        sensor_is_as5600 = true;
        return true;
    }
    return false;
}

bool turntable_init()
{
    sensor_i2c = TT_SENSOR_I2C;
    init_port(true);
    if (identify_sensor()) {
        return true;
    }
    sensor_i2c = TT_SENSOR_I2C_2;
    init_port(false);
    return identify_sensor();
}

bool turntable_is_alternative()
{
    return sensor_i2c == TT_SENSOR_I2C_2;
}

static uint16_t raw_angle = 0;

static int read_angle()
{
    if (sensor_is_as5600) {
        static int cache = 0;
        int angle = as5600_read_angle();
        if (angle >= 0) {
            cache = angle;
        }
        return cache;
    }

    return tmag5273_read_angle() * 0x1000 / 360 / 16;
}

static const int average_count = 4;
void turntable_update()
{
    int sum = read_angle();
    int count = 1;
    for (int i = 0; i < average_count - 1; i++) {
        int angle = read_angle();
        int average = sum / count;
        if (abs(angle - average) < 64) {
            sum += angle;
            count++;
        }
    }

    raw_angle = sum / count;
}

uint16_t turntable_raw()
{
    return iidx_cfg->sensor.reversed ? 4095 - raw_angle : raw_angle; // 12bit
}

uint16_t turntable_read()
{
    static int16_t old_angle = 0;

    int16_t delta = raw_angle - old_angle;
    if (delta > 2048) {
        delta -= 4096;
    } else if (delta < -2048) {
        delta += 4096;
    }

    const uint16_t divs[8] = { 200, 128, 64, 32, 256, 160, 96, 48};
    uint16_t step = 4096 / divs[iidx_cfg->sensor.ppr & 7];

    if (abs(delta) >= step) {
        if (delta > 0) {
            old_angle += step;
        } else {
            old_angle -= step;
        }
        if (old_angle > 4096) {
            old_angle -= 4096;
        } else if (old_angle < 0) {
            old_angle += 4096;
        }
    }

    return old_angle;
}