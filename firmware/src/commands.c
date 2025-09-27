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
#include "hebtn.h"

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
    const uint16_t ppr[8] = { 200, 128, 64, 32, 256, 160, 96, 48};
    printf("[TURNTABLE]\n");
    printf("  PPR: %d\n", ppr[iidx_cfg->sensor.ppr & 7]);
    printf("  Reversed: %d\n", iidx_cfg->sensor.reversed);
    printf("  Binary/LR Mode: %s\n", iidx_cfg->sensor.binary ? "Yes" : "No");
}

static void disp_hall()
{
    printf("[HALL EFFECT BUTTON]\n");
    for (int i = 0; i < hebtn_keynum(); i++) {
        if (!hebtn_present(i)) {
            printf("  Key %d: Not Present.\n", i + 1);
            continue;
        }
        printf("  Key %d: %4d->%4d, On: %2d, Off: %2d.\n",
               i + 1, iidx_cfg->hall.calibrated.up[i], iidx_cfg->hall.calibrated.down[i],
                PROFILE_EX.trigger.on[i] + 1, PROFILE_EX.trigger.off[i] + 1);
    }
}

static void handle_display()
{
    disp_rgb();
    disp_sensor();
    disp_hall();
}

static void handle_calibrate(int argc, char *argv[])
{
    hebtn_calibrate();
}

static void handle_trigger(int argc, char *argv[])
{
    const char *usage = "Usage: trigger <all|KEY> <ON> <OFF>\n"
                        "  KEY: 1..%d\n"
                        "   ON: 1..36, distance for actuation.\n"
                        "  OFF: 1..36, distance for reset.\n";
    if (argc != 3) {
        printf(usage, hebtn_keynum());
        return;
    }

    bool all_key = (strncasecmp(argv[0], "all", strlen(argv[0])) == 0);
    int key = cli_extract_non_neg_int(argv[0], 0) - 1;
    int on = cli_extract_non_neg_int(argv[1], 0) - 1;
    int off = cli_extract_non_neg_int(argv[2], 0) - 1;
    
    if ((!all_key && (key < 0)) || (key >= hebtn_keynum()) ||
        (on < 0) || (on > 35) || (off < 0) || (off > 35)) {
        printf(usage, hebtn_keynum());
        return;
    }

    for (int i = 0; i < hebtn_keynum(); i++) {
        if (all_key || (i == key)) {
            PROFILE_EX.trigger.on[i] = on;
            PROFILE_EX.trigger.off[i] = off;
        }
    }
    config_changed();

    disp_hall();
}

static void handle_debug(int argc, char *argv[])
{
    const char *usage = "Usage: debug <sensor>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }
    const char *choices[] = {"sensor"};
    switch (cli_match_prefix(choices, count_of(choices), argv[0])) {
        case 0:
            iidx_runtime.debug.sensor ^= true;
            break;
        default:
            printf(usage);
            break;
    }
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
    cli_register("calibrate", handle_calibrate, "Calibrate the key sensors.");
    cli_register("trigger", handle_trigger, "Set Hall effect switch triggering.");
    cli_register("debug", handle_debug, "Toggle debug features.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("stat", handle_stat, "Statistics.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
