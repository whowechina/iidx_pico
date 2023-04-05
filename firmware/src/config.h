/*
 * Controller Config Save and Load
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stdbool.h>

/* It's safer to lock other I/O ops during saving, so we need a locker */
typedef void (*io_locker_func)(bool pause);
void config_init(io_locker_func locker);

void config_loop();

void *config_alloc(size_t size, void *def, void (*after_load)());
void config_request_save();

#endif
