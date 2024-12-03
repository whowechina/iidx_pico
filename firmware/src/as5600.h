/*
 * AS5600 Angular Hall Sensor
 * WHowe <github.com/whowechina>
 */

#ifndef AS5600_H
#define AS5600_H

#include <stdint.h>
#include <stdbool.h>

void as5600_init(i2c_inst_t *i2c_port);
bool as5600_init_sensor();
bool as5600_is_present();
int as5600_read_angle();

#endif
