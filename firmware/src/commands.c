#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "savedata.h"
#include "cli.h"
#include "buttons.h"

static void handle_save()
{
    savedata_save(true);
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
    printf("  Level: tt %d keys %d\n", PROFILE.level.tt, PROFILE.level.keys);
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

static void handle_stat(int argc, char *argv[])
{
    printf("[STAT]\n");
    for (int i = 0; i < button_num(); i++) {
        printf("  Key %2d: %6d\n", i + 1, button_stat_keydown(i));
    }
    button_clear_stat();
}

void commands_init()
{
    cli_register("display", handle_display, "Display current config.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("stat", handle_stat, "Statistics.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
