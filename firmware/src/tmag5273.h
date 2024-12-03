/*
 * TMAG5273 Angular Hall Sensor
 * WHowe <github.com/whowechina>
 * 
 */

#ifndef TMAG5273_H
#define TMAG5273_H

#include <stdint.h>
#include <stdbool.h>

#include "hardware/i2c.h"

void tmag5273_init(unsigned instance, i2c_inst_t *i2c_port);
void tmag5273_use(unsigned instance);
bool tmag5273_is_present(unsigned instance);
bool tmag5273_change_addr(uint8_t i2c_addr);

bool tmag5273_init_sensor();

uint8_t tmag5273_read_reg(uint8_t addr);
void tmag5273_write_reg(uint8_t addr, uint8_t value);

uint16_t tmag5273_read_angle();

#endif