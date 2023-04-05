/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "pico/stdio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "buttons.h"
#include "rgb.h"
#include "turntable.h"

#include "tt_blade.h"
#include "tt_rainbow.h"

#include "config.h"


/* Measure the time of a function call */
#define RUN_TIME(func) \
   { uint64_t _t = time_us_64(); func; \
     printf(#func ":%lld\n", time_us_64() - _t); }

struct report
{
    uint16_t buttons;
    uint8_t joy0;
    uint8_t joy1;
} report;

void hid_report()
{
    if (tud_hid_ready())
    {
        report.joy1 = 0;
        tud_hid_n_report(0x00, REPORT_ID_JOYSTICK, &report, sizeof(report));
    }
}

void boot_check()
{
    uint64_t key1 = (1 << (button_num() - 1));
    uint64_t key2 = (1 << (button_num() - 2));
    uint64_t buttons = button_read();
    if ((buttons & key1) && (buttons & key2)) {
        reset_usb_boot(button_gpio(button_num() - 1), 2);
    }
}

static bool request_core1_pause = false;

static void pause_core1(bool pause)
{
    request_core1_pause = pause;
    if (pause) {
        sleep_ms(5); /* wait for any IO ops to finish */
    }
}

static void core1_loop()
{
#define RUN_EVERY_N_MS(a, ms) { if (frame % ms == 0) a; }
    uint32_t frame = 0;
    while (true) {
        uint32_t angle = turntable_read();
        rgb_set_angle(angle);
        report.joy0 = angle >> 4; // 12bit to 8bit
        RUN_EVERY_N_MS(rgb_update(), 2);
        turntable_update();
        frame++;
        do {
            sleep_ms(1);
        } while (request_core1_pause);
    }
}

static void core0_loop()
{
    while (true)
    {
        tud_task();
        report.buttons = button_read();
        hid_report();
        rgb_set_button_light(report.buttons);
        button_update();
        config_loop();
    }
}

void init()
{
    board_init();
    tusb_init();
    button_init();

    tt_rainbow_init();
    tt_blade_init();

    rgb_init();
    turntable_init();
    boot_check();
    stdio_init_all();
    config_init(pause_core1);
}

int main(void)
{
    init();
    multicore_launch_core1(core1_loop);

    core0_loop();

    return 0;
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
        if (bufsize >= button_num()) {
            button_set_hid_light(buffer, button_num());
        }
        if (bufsize >= rgb_button_num()) {
           rgb_set_hid_light(buffer, rgb_button_num());
        }
    }
}
