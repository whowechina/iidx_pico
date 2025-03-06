/*
 * Controller Setup
 * WHowe <github.com/whowechina>
 */

#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include <stdbool.h>
#include "board_defs.h"

void setup_init();
void setup_run(uint16_t keys, uint16_t angle);
bool setup_needs_tt_led();
bool setup_needs_button_led();

extern uint32_t setup_led_tt[];
extern uint32_t setup_led_button[BUTTON_RGB_NUM];

#endif
