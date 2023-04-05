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

#include "config.h"
#include "board_defs.h"
#include "rgb.h"

static uint8_t as5600_addr = 0x36;

typedef struct {
    uint16_t ignore_this;
} tt_cfg_t;

static tt_cfg_t *cfg;

static void cfg_loaded()
{
}

static void init_cfg()
{
    tt_cfg_t def = {0};
    cfg = (tt_cfg_t *)config_alloc(sizeof(def), &def, cfg_loaded);
}

static void init_tt_i2c()
{
    i2c_init(TT_AS5600_I2C, 800 * 1000);
    gpio_set_function(TT_AS5600_SCL, GPIO_FUNC_I2C);
    gpio_set_function(TT_AS5600_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(TT_AS5600_SCL);
    gpio_pull_up(TT_AS5600_SDA);
}

void turntable_init()
{
    init_cfg();
    init_tt_i2c();
}

static uint16_t angle = 0;

void turntable_update()
{
    uint8_t buf[2] = {0x0c, 0x00};
    i2c_write_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 1, true,
                             time_us_64() + 1000);
    i2c_read_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 2, false,
                            time_us_64() + 1000);

    angle = ((uint16_t)buf[0]) << 8 | buf[1];
}

uint16_t turntable_read()
{
    return angle; // 12bit
}
