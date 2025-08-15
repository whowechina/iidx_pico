/*
 * Hall Effect Button Reader
 * WHowe <github.com/whowechina>
 */

#ifndef HEBTN_H
#define HEBTN_H

void hebtn_init();
void hebtn_update();

uint8_t hebtn_keynum();

bool hebtn_present(uint8_t chn);
bool hebtn_actuated(uint8_t chn);

uint32_t hebtn_presence_map();
uint32_t hebtn_actuated_map();

uint16_t hebtn_range(uint8_t chn);
uint16_t hebtn_travel(uint8_t chn);
uint8_t hebtn_travel_byte(uint8_t chn);
uint16_t hebtn_raw(uint8_t chn);

void hebtn_calibrate();

#endif
