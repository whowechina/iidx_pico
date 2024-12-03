/*
 * TMAG5273 Angular Hall Sensor
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/i2c.h"
#include "board_defs.h"

#include "tmag5273.h"

#define TMAG5273_DEF_ADDR 0x35

#define IO_TIMEOUT_US 1000

static struct {
    i2c_inst_t *port;
    uint8_t addr;
    uint16_t cache;
    bool present;
} instances[16] = { { i2c0, TMAG5273_DEF_ADDR } };
#define INSTANCE_NUM count_of(instances)

static int current_instance = 0;
#define INSTANCE instances[current_instance]

#define I2C_PORT INSTANCE.port
#define I2C_ADDR INSTANCE.addr

static void write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    i2c_write_blocking_until(I2C_PORT, I2C_ADDR, data, 2, false,
                             time_us_64() + IO_TIMEOUT_US);
}

static uint8_t read_reg(uint8_t reg)
{
    uint8_t value;
    i2c_write_blocking_until(I2C_PORT, I2C_ADDR, &reg, 1, true,
                             time_us_64() + IO_TIMEOUT_US);
    i2c_read_blocking_until(I2C_PORT, I2C_ADDR, &value, 1, false,
                             time_us_64() + IO_TIMEOUT_US);
    return value;
}

static void read_many(uint8_t reg, uint8_t *dst, uint8_t len)
{
    i2c_write_blocking_until(I2C_PORT, I2C_ADDR, &reg, 1, true,
                             time_us_64() + IO_TIMEOUT_US);
    i2c_read_blocking_until(I2C_PORT, I2C_ADDR, dst, len, false,
                             time_us_64() + IO_TIMEOUT_US * len);
}

void tmag5273_init(unsigned instance, i2c_inst_t *i2c_port)
{
    if (instance < INSTANCE_NUM) {
        current_instance = instance;
        INSTANCE.port = i2c_port;
        INSTANCE.addr = TMAG5273_DEF_ADDR;
        INSTANCE.present = tmag5273_change_addr(TMAG5273_DEF_ADDR + 1 + instance);
    }
}

void tmag5273_use(unsigned instance)
{
    if (instance < INSTANCE_NUM) {
        current_instance = instance;
    }
}

bool tmag5273_is_present(unsigned instance)
{
    if (instance >= INSTANCE_NUM) {
        return false;
    }
    return instances[instance].present;
}

bool tmag5273_change_addr(uint8_t i2c_addr)
{
    tmag5273_write_reg(0x0c, (i2c_addr << 1) | 0x01);
    INSTANCE.addr = i2c_addr;
    tmag5273_read_reg(0x0c); // Dummy read
    uint8_t new_addr = tmag5273_read_reg(0x0c) >> 1;
    return new_addr == i2c_addr;
}

bool tmag5273_init_sensor()
{
    tmag5273_write_reg(0x03, 0x01 << 2); // Enable angle calculation
    tmag5273_write_reg(0x02, 0x03 << 4); // X-Y
    tmag5273_write_reg(0x00, 0x03 << 2); // 8x average mode
    tmag5273_write_reg(0x01, 0x02); // Continuous mode
    return true;
}

uint8_t tmag5273_read_reg(uint8_t addr)
{
    return read_reg(addr);
}

void tmag5273_write_reg(uint8_t addr, uint8_t value)
{
    write_reg(addr, value);
}

uint16_t tmag5273_read_angle()
{
    uint8_t buf[3] = { 0 };
    read_many(0x18, buf, sizeof(buf));
    if (buf[0] & 0x01) {
        INSTANCE.cache = (buf[1] << 8 | buf[2]) & 0x1fff;
    }
    return INSTANCE.cache;
}
