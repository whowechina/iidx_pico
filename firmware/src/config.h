/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute ((packed)) {
    uint8_t h; // hue;
    uint8_t s; // saturation;
    uint8_t v; // value;
} hsv_t;

typedef struct __attribute ((packed)) {
    hsv_t key_off[11];
    hsv_t key_on[11];
    struct {
        uint8_t start;
        uint8_t num;
        uint8_t effect;
        uint8_t param;
        uint8_t mode; /* 0: on, 1: reversed, 2: off */
    } tt_led;
    struct {
        bool reversed;
        uint8_t ppr; /* 0: 256, 1: 128, 2: 96, 3: 64, other: 256 */
        uint8_t reserved;
    } tt_sensor;
    uint8_t level; /* led brightness limit */
    bool konami; /* konami spoof */
} iidx_cfg_t;

extern iidx_cfg_t *iidx_cfg;

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
