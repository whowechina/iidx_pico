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
#include "hebtn.h"
#include "rgb.h"
#include "turntable.h"

#include "tt_blade.h"
#include "tt_rainbow.h"
#include "tt_heatbar.h"

#include "savedata.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

struct __attribute__((packed)) {
    uint16_t buttons;
    uint16_t axis[2];
} hid_joy, hid_joy_sent;

void report_usb_hid()
{
    static uint64_t last_report_time = 0;
    if (tud_hid_ready()) {
        uint64_t now = time_us_64();
        if ((memcmp(&hid_joy, &hid_joy_sent, sizeof(hid_joy)) == 0) &&
            (now - last_report_time < 10000)) {
            return;
        }
        last_report_time = now;
        if (tud_hid_report(REPORT_ID_JOYSTICK, &hid_joy, sizeof(hid_joy))) {
            hid_joy_sent = hid_joy;
        }
    }
}

#define AUX_1_BIT 12
#define AUX_2_BIT 11

void boot_check()
{
    uint16_t key1 = (1 << AUX_1_BIT);
    uint16_t key2 = (1 << AUX_2_BIT);
    uint16_t buttons = button_read();
    if (!watchdog_caused_reboot() && (buttons & key1) && (buttons & key2)) {
        reset_usb_boot(0, 2);
    }
}
 
void mode_check()
{
    uint16_t key1 = (1 << AUX_1_BIT);
    uint16_t key2 = (1 << AUX_2_BIT);
    uint16_t buttons = button_read();
    if (buttons & key1) {
        iidx_cfg->hid.konami = true;
        savedata_save(false);
    } else if (buttons & key2) {
        iidx_cfg->hid.konami = false;
        savedata_save(false);
    }

    if (iidx_cfg->hid.konami) {
        konami_mode();
    }
}

static uint16_t latest_angle;
static uint16_t latest_buttons;

#define TT_HOLD_TIME_MS 200

static void gen_binary_tt()
{
    static uint16_t previous_angle = 0;
    static bool tt_active = false;
    static bool tt_dir_cw = false;
    static uint32_t tt_timeout = 0;

    int16_t delta = latest_angle - previous_angle;
    previous_angle = latest_angle;

    uint64_t now = time_us_32();

    if (delta != 0) {
        tt_active = true;
        tt_dir_cw = (delta > 0);
        tt_timeout = now + TT_HOLD_TIME_MS * 1000;
    } else if (tt_active && (now > tt_timeout)) {
        tt_active = false;
    }

    hid_joy.axis[0] = tt_active ? (tt_dir_cw ? 0x7ff : 0x00) : 0x400;
    hid_joy.axis[1] = 0;
}

static void gen_hid_report()
{
    uint16_t buttons = latest_buttons;
    if (iidx_cfg->hid.konami) {
        uint16_t aux_buttons = buttons & 0xff80;
        buttons = (buttons & 0x7f) | (aux_buttons << 1); // skips button 8
    }
    hid_joy.buttons = buttons;

    if (iidx_cfg->sensor.binary) {
        gen_binary_tt();
    } else {
        hid_joy.axis[0] = latest_angle;
        hid_joy.axis[1] = 4095 - hid_joy.axis[0];
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (true) {
        uint32_t raw_angle = turntable_raw();
        latest_angle = turntable_read();

        if (mutex_try_enter(&core1_io_lock, NULL)) {
            rgb_update(raw_angle, latest_buttons);
            mutex_exit(&core1_io_lock);
        }

        cli_fps_count(1);
        sleep_us(500);
    }
}

static uint16_t hybrid_button_read()
{
    uint16_t buttons = button_read();
    for (int i = 0; i < hebtn_keynum(); i++) {
        if (hebtn_present(i)) {
            buttons |= (hebtn_actuated(i) << i);
        }
    }
    return buttons;
}

static bool hall_version = false;

static void core0_loop()
{
    absolute_time_t next_frame = {0};

    while (true)
    {
        tud_task();
        cli_run();

        turntable_update();

        if (hall_version) {
            hebtn_update();
        }

        latest_buttons = hybrid_button_read();
        uint16_t angle = turntable_raw() >> 4;
        setup_run(latest_buttons, angle);

        bool ov_tt = setup_needs_tt_led();
        bool ov_btn = setup_needs_button_led();

        if (ov_tt) {
            rgb_override_tt(setup_led_tt);
        }
        if (ov_btn) {
            rgb_override_button(setup_led_button);
        } else {
            rgb_set_button_light(latest_buttons);
        }

        hid_joy.buttons = 0;
        if (!ov_tt && !ov_btn) {
            gen_hid_report();
            savedata_loop();
        }

        report_usb_hid();
        cli_fps_count(0);

        sleep_until(next_frame);
        next_frame += 1001;
    }
}

void init()
{
    board_init();
    tusb_init();
    
    button_init();

    bool tt_present = turntable_init();

    if ((tt_present) && (turntable_is_alternative())) {
        // identify hall version by sensor's i2c port
        hall_version = true;
        hebtn_init();
    }

    if (!tt_present) {
        // even if tt not available, we still try to detect hall sensor
        hebtn_init();
        hall_version = (hebtn_presence_map() > 0);
    }

    rgb_init(hall_version);

    tt_rainbow_init();
    tt_blade_init();
    tt_heatbar_init();

    boot_check();
    stdio_init_all();

    setup_init();
    config_init();
    mutex_init(&core1_io_lock);
    savedata_init(0xca341125, &core1_io_lock);

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
        rgb_set_hid_light(buffer, bufsize);
    }
}
