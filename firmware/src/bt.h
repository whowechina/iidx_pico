/*
 * Bluetooth (BLE) support
 * WHowe <github.com/whowechina>
 *
 * The same firmware runs on Pico and Pico W. The CYW43439 is probed once at
 * boot, if it does not answer, everything Bluetooth related stays disabled.
 *
 * The controller is a HID over GATT gamepad in Konami spoof mode, it uses the
 * same report descriptor as USB: input report 1 (buttons, turntable) and
 * output report 2 (lights).
 */

#ifndef BT_H
#define BT_H

#include <stdbool.h>
#include <stdint.h>

/* Probe the radio chip and start the BLE stack. Safe on boards without the
 * chip. Call once from core 0 during init(), after core 1 is prepared for
 * flash lockout (BTstack stores its bonds in flash). The chip firmware
 * download blocks for a while. Everything after that runs in the background,
 * there is nothing to poll.
 *
 * With pairing true, old bonds are erased and a new host is accepted for a
 * limited time (see bt_pairing_active()). Otherwise only bonded hosts are. */
void bt_init(bool pairing);

/* Hand the latest 4 bytes input report (hid_joy_t) over. Cheap, never blocks,
 * the report goes out when it differs from the last one and the host is
 * ready. Call it often from the main loop. */
void bt_hid_report(const void *report);

/* True while a new host can pair, the main loop shows it on the LEDs */
bool bt_pairing_active();

bool bt_hw_present();

/* "CYW43439", "Not present" or "Not built" (firmware built without support) */
const char *bt_hw_name();

/* "Off", "Failed", "Starting", "Advertising", "Pairing", "Connected",
 * "Active" (the host listens to input reports) or "Not built" */
const char *bt_state_name();

/* Connection interval in 0.01 ms, 0 if not connected */
uint32_t bt_conn_interval();

#endif
