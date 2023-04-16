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
        .reversed = false,
    },
    .tt_sensor_analog = false,
    .tt_sensor_reversed = false,
    .effects = {
        .play_vol = 255,
        .filter = 128,
        .eq_low = 128,
        .eq_hi = 128,
    }
};

static void config_loaded()
{
    if ((iidx_cfg->tt_led.start > 8) ||
        (iidx_cfg->tt_led.start + iidx_cfg->tt_led.num > 128)) {
        iidx_cfg->tt_led.start = 0;
        iidx_cfg->tt_led.num = 24;
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
