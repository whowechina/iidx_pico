/*
 * IIDX Controller Turntable
 * WHowe <github.com/whowechina>
 */

#ifndef TURNTABLE_H
#define TURNTABLE_H

#include <stdint.h>
#include <stdbool.h>

bool turntable_init();
uint8_t turntable_read();
uint16_t turntable_raw();
void turntable_update();
bool turntable_is_alternative();
const char *turntable_sensor_name();

#endif
