/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "pico/stdio.h"
#include "hardware/watchdog.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "setup.h"

#include "buttons.h"
#include "rgb.h"
#include "turntable.h"

#include "tt_blade.h"
#include "tt_rainbow.h"

#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

struct {
    uint16_t buttons;
    uint8_t joy[2];
} hid_report;

void report_usb_hid()
{
    if (tud_hid_ready()) {
        tud_hid_n_report(0x00, REPORT_ID_JOYSTICK, &hid_report, sizeof(hid_report));
    }
}

void boot_check()
{
    uint16_t key1 = (1 << (button_num() - 1));
    uint16_t key2 = (1 << (button_num() - 2));
    uint16_t buttons = button_read();
    if (!watchdog_caused_reboot() && (buttons & key1) && (buttons & key2)) {
        reset_usb_boot(0, 2);
    }
}
 
void mode_check()
{
    uint16_t key1 = (1 << (button_num() - 1));
    uint16_t key2 = (1 << (button_num() - 2));
    uint16_t buttons = button_read();
    if (buttons & key1) {
        iidx_cfg->konami = true;
        save_request(false);
    } else if (buttons & key2) {
        iidx_cfg->konami = false;
        save_request(false);
    }

    if (iidx_cfg->konami) {
        konami_mode();
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (true) {
        uint32_t angle = turntable_raw();
        rgb_set_angle(angle);

        uint8_t angle8 = turntable_read();
        hid_report.joy[0] = angle8;
        hid_report.joy[1] = 255 - angle8;

        if (mutex_try_enter(&core1_io_lock, NULL)) {
            rgb_update();
            mutex_exit(&core1_io_lock);
        }

        cli_fps_count(1);
        sleep_us(500);
    }
}

static void core0_loop()
{
    absolute_time_t next_frame = {0};

    while (true)
    {
        tud_task();
        cli_run();

        turntable_update();

        uint16_t buttons = button_read();
        uint16_t angle = turntable_raw() >> 4;
        setup_run(buttons, angle);

        bool ov_tt = setup_needs_tt_led();
        bool ov_btn = setup_needs_button_led();

        if (ov_tt) {
            rgb_override_tt(setup_led_tt);
        }
        if (ov_btn) {
            rgb_override_button(setup_led_button);
        } else {
            rgb_set_button_light(buttons);
        }

        if (!ov_tt && !ov_btn) {
            save_loop();
        }

        report_usb_hid();
        cli_fps_count(0);

        sleep_until(next_frame);
        next_frame = make_timeout_time_us(1000);
    }
}

void init()
{
    board_init();
    tusb_init();

    button_init();
    turntable_init();

    rgb_init();
    tt_rainbow_init();
    tt_blade_init();

    boot_check();
    stdio_init_all();

    setup_init();
    config_init();
    mutex_init(&core1_io_lock);
    save_init(0xca341234, &core1_io_lock);

    cli_init("iidx_pico>", "\n   << IIDX Pico|Teeny Controller >>\n"
                            " https://github.com/whowechina\n\n");
    commands_init();
    mode_check();
}

void main(void)
{
    init();
    multicore_launch_core1(core1_loop);
    core0_loop();
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
    if ((report_id == REPORT_ID_LIGHTS) &&
        (report_type == HID_REPORT_TYPE_OUTPUT)) {
        if (bufsize >= rgb_hid_light_num()) {
           rgb_set_hid_light(buffer, bufsize);
        }
    }
}
