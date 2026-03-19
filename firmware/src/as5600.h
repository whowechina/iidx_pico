/*
 * AS5600 Angular Hall Sensor
 * 12-bit, I2C Interface
 * WHowe <github.com/whowechina>
 */

#ifndef AS5600_H
#define AS5600_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

void as5600_init(i2c_inst_t *i2c_port);
bool as5600_is_present();
int as5600_read(); // angle, 12-bit, [0..4095]

#endif
