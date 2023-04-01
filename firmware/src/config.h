/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stdbool.h>

typedef void (*core2_locker)(bool pause);

void config_init(core2_locker locker);
void config_loop();

void *config_alloc(size_t size, void *def, void (*after_load)());
void config_request_save();

#endif
