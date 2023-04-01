/*
 * Controller Buttons
 * WHowe <github.com/whowechina>
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/flash.h"

void button_init();
uint8_t button_num();
uint8_t button_gpio(uint8_t id);

uint16_t button_read();

void button_update();
void button_set_hid_light(uint8_t const *lights, uint8_t num);

#endif
