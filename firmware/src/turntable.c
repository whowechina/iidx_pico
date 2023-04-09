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

void turntable_init()
{
#ifdef TT_AS5600_ANALOG
    adc_init();
    adc_gpio_init(TT_AS5600_ANALOG);
    adc_select_input(TT_AS5600_ANALOG - 26);
#else
    i2c_init(TT_AS5600_I2C, 800 * 1000);
    gpio_set_function(TT_AS5600_SCL, GPIO_FUNC_I2C);
    gpio_set_function(TT_AS5600_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(TT_AS5600_SCL);
    gpio_pull_up(TT_AS5600_SDA);
#endif
}

uint32_t max_adc = 3500;
static inline void adjust_max(uint32_t value)
{
    if (value > max_adc) {
        max_adc += (value - max_adc + 1) / 2;
    }
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
        if (sample > 3500) {
            large_cnt++;
            large += sample;
        } else if (sample < 500) {
            small_cnt++;
            small += sample;
        } else {
            medium += sample;
        }
    }

    if (large_cnt > 100) {
        adjust_max(large / large_cnt);
    }

    uint32_t all = large + small + medium;

    if (large_cnt && small_cnt) {
        all += small_cnt * max_adc;
    }

    return (all / size) % max_adc;
}

void turntable_update()
{
#ifdef TT_AS5600_ANALOG
    const uint16_t deadzone = 24;
    static uint16_t sample = 0;
    int new_value = read_average(200);
    int delta = abs(new_value - sample);
    if ((abs(delta) < deadzone) || (abs(delta) > 4096 - deadzone)) {
        return;
    }
    sample = new_value;
    angle = (sample * 4095) / max_adc;
#else
    const uint8_t as5600_addr = 0x36;
    uint8_t buf[2] = {0x0c, 0x00};
    i2c_write_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 1, true,
                             make_timeout_time_ms(1));
    i2c_read_blocking_until(TT_AS5600_I2C, as5600_addr, buf, 2, false,
                            make_timeout_time_ms(1));

    angle = ((uint16_t)buf[0] & 0x0f) << 8 | buf[1];
#endif
}

uint16_t turntable_read()
{
    return iidx_cfg->tt_sensor_reversed ? 4095 - angle : angle; // 12bit
}
