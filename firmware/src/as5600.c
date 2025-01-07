/*
 * AS5600 Angular Hall Sensor
 * WHowe <github.com/whowechina>
 * 
 */

#include "turntable.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "board_defs.h"
#include "config.h"

#define AS5600_ADDR 0x36
#define AS5600_REG_ANGLE 0x0c

static i2c_inst_t *as5600_i2c = i2c0;
static uint16_t raw_angle = 0;

void as5600_init(i2c_inst_t *i2c_port)
{
    as5600_i2c = i2c_port;
}

bool as5600_init_sensor()
{
    return true;
}

static bool as5600_is_present()
{
    uint8_t buf[1] = {0x0c};
    int ret = i2c_write_blocking_until(as5600_i2c, AS5600_ADDR, buf, 1, true,
                             make_timeout_time_ms(1));
    return ret == 1;
}

static int as5600_read_reg16(uint8_t reg)
{
    uint8_t buf[2] = {reg, 0x00};
    if (i2c_write_blocking_until(as5600_i2c, AS5600_ADDR, buf, 1, true,
                             make_timeout_time_ms(1)) != 1) {
        return -1;
    }

    if (i2c_read_blocking_until(as5600_i2c, AS5600_ADDR, buf, 2, false,
                            make_timeout_time_ms(1)) != 2) {
        return -1;
    }

    return (buf[0] << 8) | buf[1];
}

int as5600_read_angle()
{
    return as5600_read_reg16(AS5600_REG_ANGLE);
}
