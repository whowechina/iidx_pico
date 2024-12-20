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
    .key_off = { { 0, 255, 32 }, { 23, 255, 32 }, { 46, 255, 32 }, { 69, 255, 32 },
                 { 92, 255, 32 }, { 115, 255, 32 }, { 138, 255, 32 },
                 { 161, 255, 32 }, { 184, 255, 32 }, { 207, 255, 32 }, { 230, 255, 32 },
    },
    .key_on = { { 40, 0, 96 }, { 40, 0, 96 }, { 40, 0, 96 }, { 40, 0, 96 },
                { 40, 0, 96 }, { 40, 0, 96 }, { 40, 0, 96 },
                { 40, 0, 96 }, { 40, 0, 96 }, { 40, 0, 96 }, { 40, 0, 96 },
    },
    .tt_led = {
        .start = 0,
        .num = 24,
        .effect = 0,
        .param = 0,
        .mode = 0,
    },
    .tt_sensor = {
        .reversed = false,
        .ppr = 1,
    },
    .level = 128,
    .konami = false,
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
    if (iidx_cfg->tt_led.mode > 2) {
        iidx_cfg->tt_led.mode = 0;
        config_changed();
    }
    if (iidx_cfg->tt_sensor.ppr > 3) {
        iidx_cfg->tt_sensor.ppr = 1;
        config_changed();
    }
}

void config_changed()
{
    save_request(false);
}

void config_factory_reset()
{
    *iidx_cfg = default_cfg;
    save_request(true);
}

void config_init()
{
    iidx_cfg = (iidx_cfg_t *)save_alloc(sizeof(*iidx_cfg), &default_cfg, config_loaded);
}
