/*
 * IIDX Controller Turntable
 * WHowe <github.com/whowechina>
 */

#ifndef TURNTABLE_H
#define TURNTABLE_H

#include <stdint.h>
#include <stdbool.h>

void turntable_init();
void turntable_set_hardware(bool sensor_reversed);
uint16_t turntable_read();
void turntable_update();

#endif
