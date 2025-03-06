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
    .rgb.format = { 0, 0, 0 },
    .rgb.level = { 128, 128 },
    .rgb.tt = { .start = 0, .num = 24, .reversed = 0 },

    .sensor = {
        .reversed = 0,
        .ppr = 1,
    },

    .hid = {
        .konami = 0,
    },

    .profile = 0,
    .profiles = {
        {
            .tt_theme = 0,
            .tt_param = 0,
            .key_off = { { 0, 0, 255, 32 }, { 0, 23, 255, 32 }, { 0, 46, 255, 32 }, { 0, 69, 255, 32 },
                     { 0, 92, 255, 32 }, { 0, 115, 255, 32 }, { 0, 138, 255, 32 },
                     { 0, 161, 255, 32 }, { 0, 184, 255, 32 }, { 0, 207, 255, 32 }, { 0, 230, 255, 32 },
            },
            .key_on = { { 0, 40, 0, 96 }, { 0, 40, 0, 96 }, { 0, 40, 0, 96 }, { 0, 40, 0, 96 },
                    { 0, 40, 0, 96 }, { 0, 40, 0, 96 }, { 0, 40, 0, 96 },
                    { 0, 40, 0, 96 }, { 0, 40, 0, 96 }, { 0, 40, 0, 96 }, { 0, 40, 0, 96 },
            },
        },
    }
};

static void config_loaded()
{
    if (iidx_cfg->rgb.tt.num == 0) {
        iidx_cfg->rgb.tt.num = 24;
        config_changed();
    }
    if ((iidx_cfg->rgb.tt.start > 8) ||
        (iidx_cfg->rgb.tt.start + iidx_cfg->rgb.tt.num > 128)) {
        iidx_cfg->rgb.tt.start = 0;
        iidx_cfg->rgb.tt.num = 24;
        config_changed();
    }
    if (iidx_cfg->sensor.ppr > 3) {
        iidx_cfg->sensor.ppr = 1;
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
