/*
 * MT6701 Angular Hall Sensor
 * 14-bit, I2C Interface
 * WHowe <github.com/whowechina>
 */

#include "mt6701.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

#define MT6701_ADDR 0x06
#define MT6701_REG_ANGLE 0x03

static i2c_inst_t *mt6701_i2c = i2c0;

void mt6701_init(i2c_inst_t *i2c_port)
{
    mt6701_i2c = i2c_port;
}

bool mt6701_is_present()
{
    uint8_t buf[1] = {MT6701_REG_ANGLE};
    int ret = i2c_write_blocking_until(mt6701_i2c, MT6701_ADDR, buf, 1, true,
                                       make_timeout_time_ms(1));
    return ret == 1;
}

static int mt6701_read_raw_angle()
{
    uint8_t reg = MT6701_REG_ANGLE;
    uint8_t buf[2] = {0};

    if (i2c_write_blocking_until(mt6701_i2c, MT6701_ADDR, &reg, 1, true,
                                 make_timeout_time_ms(1)) != 1) {
        return -1;
    }

    if (i2c_read_blocking_until(mt6701_i2c, MT6701_ADDR, buf, 2, false,
                                make_timeout_time_ms(1)) != 2) {
        return -1;
    }

    return (buf[0] << 6) | (buf[1] >> 2);
}

int mt6701_read()
{
    int angle = mt6701_read_raw_angle();
    if (angle < 0) {
        return -1;
    }

    return angle;
}
