#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "save.h"
#include "cli.h"

static void handle_save()
{
    save_request(true);
}

static void handle_factory_reset()
{
    config_factory_reset();
    printf("Factory reset done.\n");
}

static void disp_rgb()
{
    printf("[RGB]\n");
    printf("  Order: %d %d %d\n", iidx_cfg->rgb.format.main, iidx_cfg->rgb.format.tt, iidx_cfg->rgb.format.effect);
    printf("  TT: start %d num %d reversed: %d\n", iidx_cfg->rgb.tt.start, iidx_cfg->rgb.tt.num, iidx_cfg->rgb.tt.reversed);
    printf("  Level: tt %d keys %d\n", iidx_cfg->rgb.level.tt, iidx_cfg->rgb.level.keys);
}

static void disp_sensor()
{
    printf("[SENSOR]\n");
    printf("  PPR: %d reversed: %d\n", iidx_cfg->sensor.ppr, iidx_cfg->sensor.reversed);
}

static void handle_display()
{
    disp_rgb();
    disp_sensor();
}

void commands_init()
{
    cli_register("display", handle_display, "Display current config.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
