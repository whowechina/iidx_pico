/*
 * IIDX Controller Turntable
 * WHowe <github.com/whowechina>
 */

#ifndef TURNTABLE_H
#define TURNTABLE_H

#include <stdint.h>
#include <stdbool.h>

bool turntable_init();
uint32_t turntable_read(uint8_t bits);
uint16_t turntable_read_abs(uint8_t bits);
void turntable_update();
bool turntable_is_alternative();
const char *turntable_sensor_name();

#endif
