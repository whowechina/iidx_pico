/*
 * Controller Config Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 */

#include "config.h"
#include "save.h"

iidx_cfg_t *iidx_cfg;

static iidx_cfg_t default_cfg = {
    .key_off = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}},
    .key_on = { {40,40,40}, {40,40,40}, {40,40,40}, {40,40,40}, {40,40,40}, {40,40,40},
                {40,40,40}, {40,40,40}, {40,40,40}, {40,40,40}, {40,40,40},
    },
    .tt_led = {
        .start = 0,
        .num = 24,
        .effect = 0,
        .param = 0,
        .brightness = 5,
        .mode = 0,
    },
    .tt_sensor = {
        .mode = 0,
        .deadzone = 0,
        .ppr = 0,
    },
    .effects = {
        .e1 = 255,
        .e2 = 128,
        .e3 = 128,
        .e4 = 128,
    }
};

static void config_loaded()
{
    if (iidx_cfg->tt_led.num == 0) {
        iidx_cfg->tt_led.num = 24;
        config_changed();
    }
    if ((iidx_cfg->tt_led.start > 8) ||
        (iidx_cfg->tt_led.start + iidx_cfg->tt_led.num > 128)) {
        iidx_cfg->tt_led.start = 0;
        iidx_cfg->tt_led.num = 24;
        config_changed();
    }
    if (iidx_cfg->tt_sensor.deadzone > 2) {
        iidx_cfg->tt_sensor.deadzone = 0;
        config_changed();
    }
    if (iidx_cfg->tt_led.mode > 2) {
        iidx_cfg->tt_led.mode = 0;
        config_changed();
    }
    if (iidx_cfg->tt_sensor.mode > 3) {
        iidx_cfg->tt_sensor.mode = 0;
        config_changed();
    }
    if (iidx_cfg->tt_sensor.ppr > 3) {
        iidx_cfg->tt_sensor.ppr = 0;
        config_changed();
    }
}

void config_changed()
{
    save_request();
}

void config_init()
{
    iidx_cfg = (iidx_cfg_t *)save_alloc(sizeof(iidx_cfg), &default_cfg, config_loaded);
}
