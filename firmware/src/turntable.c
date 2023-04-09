/*
 * IIDX Controller Turntable
 * WHowe <github.com/whowechina>
 * 
 */

#include "turntable.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "board_defs.h"
#include "config.h"

static uint8_t as5600_addr = 0x36;
static uint16_t angle = 0;

void turntable_init()
{
    i2c_init(TT_AS5600_I2C, 800 * 1000);
    gpio_set_function(TT_AS5600_SCL, GPIO_FUNC_I2C);
    gpio_set_function(TT_AS5600_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(TT_AS5600_SCL);
    gpio_pull_up(TT_AS5600_SDA);
}

void turntable_update()
{ 
    uint8_t buf[2] = {0x0c, 0x00};
    i2c_write_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 1, true,
                             make_timeout_time_ms(1));
    i2c_read_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 2, false,
                            make_timeout_time_ms(1));

    angle = ((uint16_t)buf[0] & 0x0f) << 8 | buf[1];
}

uint16_t turntable_read()
{
    return iidx_cfg->tt_sensor_reversed ? 4095 - angle : angle; // 12bit
}
