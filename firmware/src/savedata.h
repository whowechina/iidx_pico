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

/* It's safer to lock other I/O ops during saving, so we need a locker */
typedef void (*io_locker_func)(bool pause);
void savedata_init(uint32_t magic, mutex_t *lock);

void savedata_loop();

void *savedata_alloc(size_t size, void *def, void (*after_load)());
void savedata_save(bool immediately);

#endif
