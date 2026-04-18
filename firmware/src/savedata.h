/*
 * Controller Savedata
 * WHowe <github.com/whowechina>
 */

#ifndef SAVEDATA_H
#define SAVEDATA_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/multicore.h"

uint32_t savedata_id_32();
uint64_t savedata_id_64();

void savedata_init(uint32_t magic);

void savedata_loop();

void *savedata_alloc(size_t size, void *def, void (*after_load)());
void savedata_save(bool immediately);
void savedata_save_clean();

#endif
