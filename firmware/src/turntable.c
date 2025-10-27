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
static uint8_t i2c_scl = TT_SENSOR_SCL;
static uint8_t i2c_sda = TT_SENSOR_SDA;

static bool sensor_identified = false;
static bool sensor_is_as5600 = true;

static void init_port(bool use_primary)
{
    if (use_primary) {
        sensor_i2c = TT_SENSOR_I2C;
        i2c_scl = TT_SENSOR_SCL;
        i2c_sda = TT_SENSOR_SDA;
    } else {
        sensor_i2c = TT_SENSOR_I2C_2;
        i2c_scl = TT_SENSOR_SCL_2;
        i2c_sda = TT_SENSOR_SDA_2;
    }

    gpio_init(i2c_scl);
    gpio_init(i2c_sda);
    gpio_set_function(i2c_scl, GPIO_FUNC_I2C);
    gpio_set_function(i2c_sda, GPIO_FUNC_I2C);
    gpio_pull_up(i2c_scl);
    gpio_pull_up(i2c_sda);

    i2c_init(sensor_i2c, TT_SENSOR_I2C_FREQ);
}

static void deinit_port()
{
    gpio_deinit(i2c_scl);
    gpio_deinit(i2c_sda);
    i2c_deinit(sensor_i2c);
}

static bool identify_sensor()
{
    sensor_identified = false;

    tmag5273_init(0, sensor_i2c);
    if (tmag5273_is_present(0)) {
        tmag5273_use(0);
        tmag5273_init_sensor();
        sensor_is_as5600 = false;
        sensor_identified = true;
        return true;
    }

    as5600_init(sensor_i2c);
    if (as5600_is_present(sensor_i2c)) {
        sensor_is_as5600 = true;
        sensor_identified = true;
        return true;
    }
    return false;
}

bool turntable_init()
{
    init_port(false);
    if (identify_sensor()) {
        return true;
    }
    deinit_port();

    init_port(true);
    if (identify_sensor()) {
        return true;
    }
    deinit_port();

    return false;
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

uint8_t turntable_read()
{
    static uint8_t counter = 0;
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

const char *turntable_sensor_name()
{
    if (!sensor_identified) {
        return "Unknown";
    }
    return sensor_is_as5600 ? "AS5600" : "TMAG5273";
}
