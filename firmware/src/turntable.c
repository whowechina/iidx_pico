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
#include "mt6701.h"
#include "tmag5273.h"

#include "board_defs.h"
#include "config.h"

static i2c_inst_t *sensor_i2c = TT_SENSOR_I2C;
static uint8_t i2c_scl = TT_SENSOR_SCL;
static uint8_t i2c_sda = TT_SENSOR_SDA;

typedef enum {
    SENSOR_UNKNOWN,
    SENSOR_AS5600,
    SENSOR_MT6701,
    SENSOR_TMAG5273
} sensor_type_t;
static sensor_type_t sensor_type = SENSOR_UNKNOWN;

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
    sensor_type = SENSOR_UNKNOWN;

    as5600_init(sensor_i2c);
    if (as5600_is_present(sensor_i2c)) {
        sensor_type = SENSOR_AS5600;
        return true;
    }

    mt6701_init(sensor_i2c);
    if (mt6701_is_present()) {
        sensor_type = SENSOR_MT6701;
        return true;
    }

    tmag5273_init(0, sensor_i2c);
    if (tmag5273_is_present(0)) {
        tmag5273_use(0);
        tmag5273_init_sensor();
        sensor_type = SENSOR_TMAG5273;
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

#define ANGLE_BITS 14
#define ANGLE_RANGE (1 << ANGLE_BITS)
#define ANGLE_MAX (ANGLE_RANGE - 1)
#define ANGLE_CENTER (ANGLE_RANGE / 2)

static uint16_t raw_angle = 0;

static int read_angle()
{
    static int cache = 0;
    if (sensor_type == SENSOR_AS5600) {
        int angle = as5600_read();
        if (angle >= 0) {
            cache = angle << 2;
        }
        return cache;
    }

    if (sensor_type == SENSOR_MT6701) {
        int angle = mt6701_read();
        if (angle >= 0) {
            cache = angle;
        }
        return cache;
    }

    if (sensor_type == SENSOR_TMAG5273) {
        int angle = tmag5273_read();
        return angle * ANGLE_RANGE / 360 / 16;
    }

    return 0;
}

static const int average_count = 4;
void turntable_update()
{
    int ref = read_angle();
    int sum = 0;
    for (int i = 1; i < average_count; i++) {
        int d = read_angle() - ref;
        if (d > ANGLE_CENTER) {
             d -= ANGLE_RANGE;
        }
        else if (d < -ANGLE_CENTER) {
            d += ANGLE_RANGE;
        }
        sum += d;
    }

    int angle = (ref + sum / average_count + ANGLE_RANGE) % ANGLE_RANGE;
    
    raw_angle = iidx_cfg->sensor.reversed ? (ANGLE_MAX - angle) : angle;
}

uint16_t turntable_read_abs(uint8_t bits)
{
    if (bits >= ANGLE_BITS) {
        return raw_angle;
    }
    return raw_angle >> (ANGLE_BITS - bits);
}

uint32_t turntable_read(uint8_t bits)
{
    static uint32_t counter = 0;
    static int16_t old_angle = 0;

    int16_t delta = raw_angle - old_angle;
    if (delta > ANGLE_CENTER) {
        delta -= ANGLE_RANGE;
    } else if (delta < -ANGLE_CENTER) {
        delta += ANGLE_RANGE;
    }

    const uint16_t divs[8] = { 200, 128, 64, 32, 256, 160, 96, 48};
    uint16_t step = ANGLE_RANGE / divs[iidx_cfg->sensor.ppr & 7];

    if (abs(delta) >= step) {
        if (delta > 0) {
            counter++;
            old_angle += step;
        } else {
            counter--;
            old_angle -= step;
        }
        if (old_angle > ANGLE_MAX) {
            old_angle -= ANGLE_RANGE;
        } else if (old_angle < 0) {
            old_angle += ANGLE_RANGE;
        }
    }

    if (bits < 32) {
        return counter & ((1UL << bits) - 1);
    }
    return counter;
}

const char *turntable_sensor_name()
{
    switch (sensor_type) {
        case SENSOR_AS5600:
            return "AS5600";
        case SENSOR_MT6701:
            return "MT6701";
        case SENSOR_TMAG5273:
            return "TMAG5273";
        default:
            return "Unknown";
    }
}
