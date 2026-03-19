/*
 * MT6701 Angular Hall Sensor
 * 14-bit, I2C Interface
 * WHowe <github.com/whowechina>
 */

#ifndef MT6701_H
#define MT6701_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

void mt6701_init(i2c_inst_t *i2c_port);
bool mt6701_is_present();
int mt6701_read(); // angle, 14-bit, [0..16383]

#endif
