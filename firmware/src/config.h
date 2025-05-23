/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#include "rgb.h"

typedef struct __attribute ((packed)) {
    struct {
        struct {
            uint8_t main : 2;
            uint8_t tt : 2;
            uint8_t effect : 2;
            uint8_t not_used : 2;
        } format;
        struct {
            uint8_t reversed : 1;
            uint8_t start : 7;
            uint8_t num;
        } tt;
    } rgb;
    struct {
        uint8_t reversed : 1;
        uint8_t ppr : 4;
        uint8_t binary: 1;
        uint8_t not_used : 2;
        uint8_t res;
    } sensor;
    struct {
        bool konami; /* konami spoof */
        uint8_t res;
    } hid;
    uint8_t profile;
    struct {
        struct {
            uint8_t keys;
            uint8_t tt;
        } level;
        uint8_t tt_style;
        uint8_t tt_param;
        color_t key_on[11];
        color_t key_off[11];
    } profiles[4];
} iidx_cfg_t;

extern iidx_cfg_t *iidx_cfg;

#define PROFILE iidx_cfg->profiles[iidx_cfg->profile % 4]

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
