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
    struct {
        struct {
            uint16_t up[7];
            uint16_t down[7];
        } calibrated;
        bool suppressed;
        uint8_t not_used[3];
    } hall;
    struct {
        struct {
            uint8_t on[7];
            uint8_t off[7];
        } trigger;
        uint8_t key_light_mode;
        uint8_t not_used[17];
    } profile_ex[4];
} iidx_cfg_t;

typedef struct {
    struct {
        bool sensor;
    } debug;
} iidx_runtime_t;

extern iidx_cfg_t *iidx_cfg;
extern iidx_runtime_t iidx_runtime;

#define PROFILE iidx_cfg->profiles[iidx_cfg->profile % 4]
#define PROFILE_EX iidx_cfg->profile_ex[iidx_cfg->profile % 4]

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
