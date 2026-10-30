/*
 * Bluetooth (BLE) support
 * WHowe <github.com/whowechina>
 *
 * The same firmware runs on Pico and Pico W. The CYW43439 is probed once at
 * boot, if it does not answer, everything Bluetooth related stays disabled.
 *
 * The protocol is the one Konami's own BLE controllers use, it is not HID:
 * a GATT service 0xFF00 with one characteristic 0xFF01. The host enables
 * notifications and the controller pushes 10 bytes, two frames of
 *   { turntable, sensor B, keys 1-7, E1/E2, frame counter }
 * There is no pairing, no bonding and no encryption.
 */

#ifndef BLE_H
#define BLE_H

#include <stdbool.h>
#include <stdint.h>

/* Probe the radio chip and start the BLE stack. Safe on boards without the
 * chip. Call once from core 0 during init(), after core 1 is prepared for
 * flash lockout (BTstack's storage lives in flash). The chip firmware
 * download blocks for a while. Everything after that runs in the background,
 * there is nothing to poll. */
void ble_init(void);

/* Hand the latest input over. Cheap, never blocks, call it every loop.
 * tt: 8 bits turntable angle, keys1: bit 0-6 are keys 1-7, keys2: bit 0 is E1
 * and bit 1 is E2. */
void ble_input(uint8_t tt, uint8_t keys1, uint8_t keys2);

bool ble_hw_present();

void ble_update();

/* "CYW43439", "Not present" or "Not built" (firmware built without support) */
const char *ble_hw_name();

/* "Off", "Failed", "Starting", "Advertising", "Connected", "Active" (the host
 * enabled notifications) or "Not built" */
const char *ble_state_name();

/* Connection interval in 0.01 ms, 0 if not connected */
uint32_t ble_conn_interval();

#endif
