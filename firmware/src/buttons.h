/*
 * Controller Buttons
 * WHowe <github.com/whowechina>
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

void button_init();
uint8_t button_num();
uint8_t button_gpio(uint8_t id);

uint16_t button_read();

uint32_t button_stat_keydown(uint8_t id);
uint32_t button_clear_stat();

#endif
