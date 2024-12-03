/*
 * IIDX Controller Turntable
 * WHowe <github.com/whowechina>
 */

#ifndef TURNTABLE_H
#define TURNTABLE_H

#include <stdint.h>
#include <stdbool.h>

void turntable_init();
uint8_t turntable_read();
uint16_t turntable_raw();
void turntable_update();

#endif
