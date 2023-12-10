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

void turntable_init()
{
    init_i2c();
}

void turntable_update()
{
    const uint8_t as5600_addr = 0x36;
    uint8_t buf[2] = {0x0c, 0x00};
    i2c_write_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 1, true,
                             make_timeout_time_ms(1));
    i2c_read_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 2, false,
                            make_timeout_time_ms(1));

    angle = ((uint16_t)buf[0] & 0x0f) << 8 | buf[1];
}

uint16_t turntable_raw()
{
    return iidx_cfg->tt_sensor.reversed ? 4095 - angle : angle; // 12bit
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
        return angle >> 4;
    }

    int16_t delta = angle - old_angle;
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