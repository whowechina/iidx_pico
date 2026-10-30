/*
 * Bluetooth (BLE) support
 * WHowe <github.com/whowechina>
 */

#include "ble.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/btstack_flash_bank.h"
#include "hardware/gpio.h"

#include "btstack.h"
#include "bt_profile.h"

#include "savedata.h"

/* Games look for this name, it is also in ble_profile.gatt */
#define BLE_NAME "IIDX Entry model"

/* Connection interval to ask for, units of 1.25ms: 7.5ms to 11.25ms. The host
 * usually picks the upper end (a 15..30ms request ended up at 30ms), so the
 * upper end is what matters. iOS only accepts a range whose maximum is at least
 * 15ms above the minimum (Apple QA1931) and would ignore this request, the
 * link then simply stays at its default. Latency 0 so no connection event is
 * skipped, timeout in 10ms. */
#define CONN_INTERVAL_MIN 6
#define CONN_INTERVAL_MAX 9
#define CONN_LATENCY 0
#define CONN_TIMEOUT 400

/* Notification pacing. Too many notifications flood the stack and inputs get
 * dropped (the reference controller waits 25ms), so a changed input goes out
 * after at least MIN_GAP_MS, which is about one per connection event, and an
 * unchanged one still repeats every HEARTBEAT_MS so the game keeps seeing
 * frames. */
#define MIN_GAP_MS 5
#define HEARTBEAT_MS 25

#define FRAME_SIZE 5
#define FRAMES_PER_NOTIFY 2

typedef enum {
    BLE_OFF,
    BLE_FAILED,
    BLE_STARTING,
    BLE_ADVERTISING,
    BLE_CONNECTED,
    BLE_ACTIVE,
} ble_state_t;

static bool present = false;
static volatile ble_state_t state = BLE_OFF;
static volatile uint16_t conn_interval = 0; /* 1.25ms */

static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static btstack_data_source_t input_source;
static btstack_timer_source_t send_timer;

/* tt | keys1 << 8 | keys2 << 16. One word, so the main loop and the BTstack
 * context (both on core 0) can hand it over without a lock. */
static volatile uint32_t latest_input = 0;
static volatile bool poke_pending = false;

/* Only touched in the BTstack context */
static uint32_t sent_input = 0;
static bool sent_valid = false;
static uint32_t last_send_ms = 0;
static uint8_t frame_counter = 0;
static bool notify_enabled = false;
static bool send_pending = false;

/* BTstack storage (two sectors) sits right below the savedata sector. Called
 * by the SDK's flash bank code, see pico_flash_bank_get_storage_offset_func */
uint32_t ble_flash_storage_offset(void)
{
    return get_savedata_offset() - PICO_FLASH_BANK_TOTAL_SIZE;
}

/* Flash sectors the firmware image must not grow into */
static bool flash_layout_ok()
{
    extern char __flash_binary_end;
    return (uintptr_t)&__flash_binary_end - XIP_BASE <= ble_flash_storage_offset();
}

/* BTstack storage is written with flash_safe_execute(), it only works when
 * core 1 has been prepared for the lockout, main.c does that early in init() */
static bool core1_ready()
{
    return multicore_lockout_victim_is_initialized(1);
}

static bool chip_responds()
{
    bool dummy;

    /* The first access powers the chip up and downloads its firmware, it
     * fails right away if there is no chip on the SPI bus. */
    cyw43_arch_lwip_begin();
    int ret = cyw43_gpio_get(&cyw43_state, CYW43_WL_GPIO_LED_PIN, &dummy);
    cyw43_arch_lwip_end();

    return ret == 0;
}

/* On a board without the chip these pins are something else (SMPS mode,
 * VBUS sense, LED, VSYS sense), put them back to their reset state. */
static void restore_wl_pins()
{
    const uint pins[] = { CYW43_PIN_WL_REG_ON, CYW43_PIN_WL_DATA_OUT,
                          CYW43_PIN_WL_CS, CYW43_PIN_WL_CLOCK };

    for (int i = 0; i < count_of(pins); i++) {
        gpio_deinit(pins[i]);
        gpio_set_pulls(pins[i], false, true);
    }
}

/* Same layout as the reference ESP32 controller: the service UUID as a 128 bit
 * UUID in the advertisement, the name in the scan response */
static const struct __attribute__((packed)) {
    uint8_t flags_len, flags_type, flags;
    uint8_t uuid_len, uuid_type;
    uint8_t uuid[16];
} adv_data = {
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06, /* general discoverable, no BR/EDR */
    17, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
    /* 0000ff00-0000-1000-8000-00805f9b34fb, little endian */
    { 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
      0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00 },
};

static const struct __attribute__((packed)) {
    uint8_t len, type;
    char name[sizeof(BLE_NAME) - 1];
} scan_rsp_data = {
    sizeof(BLE_NAME), BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_NAME,
};

static void start_advertising()
{
    bd_addr_t null_addr = { 0 };

    /* connectable undirected, 20ms, all 3 channels, no filter */
    gap_advertisements_set_params(0x20, 0x20, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)&adv_data);
    gap_scan_response_set_data(sizeof(scan_rsp_data), (uint8_t *)&scan_rsp_data);
    gap_advertisements_enable(1);
    state = BLE_ADVERTISING;
}

/* Two frames per notification, the counter runs per frame */
static void fill_frames(uint8_t *out, uint32_t input)
{
    for (int i = 0; i < FRAMES_PER_NOTIFY; i++) {
        out[0] = input & 0xff; /* turntable */
        out[1] = 0; /* sensor B, not used by IIDX */
        out[2] = (input >> 8) & 0xff; /* keys 1-7 */
        out[3] = (input >> 16) & 0xff; /* E1, E2 */
        out[4] = frame_counter + i;
        out += FRAME_SIZE;
    }
}

static void send_timer_handler(btstack_timer_source_t *timer);

/* Decide whether a notification is due, otherwise wait for it to become due */
static void send_poll()
{
    if (!notify_enabled || send_pending) {
        return;
    }

    uint32_t elapsed = btstack_run_loop_get_time_ms() - last_send_ms;
    bool changed = !sent_valid || (latest_input != sent_input);
    uint32_t due = changed ? MIN_GAP_MS : HEARTBEAT_MS;

    if (elapsed >= due) {
        send_pending = true;
        att_server_request_can_send_now_event(con_handle);
        return;
    }

    btstack_run_loop_remove_timer(&send_timer);
    btstack_run_loop_set_timer_handler(&send_timer, send_timer_handler);
    btstack_run_loop_set_timer(&send_timer, due - elapsed);
    btstack_run_loop_add_timer(&send_timer);
}

static void send_timer_handler(btstack_timer_source_t *timer)
{
    send_poll();
}

/* Runs in the BTstack context. Poll callbacks run on every pass of the run
 * loop, and arming a timer triggers another pass, so only act when
 * ble_input() asked for it, otherwise this spins in the IRQ and starves the
 * main loop. */
static void input_poll(btstack_data_source_t *source,
                       btstack_data_source_callback_type_t type)
{
    if (!poke_pending) {
        return;
    }
    poke_pending = false;
    send_poll();
}

static void send_notification()
{
    uint8_t frames[FRAME_SIZE * FRAMES_PER_NOTIFY];
    uint32_t input = latest_input;

    send_pending = false;
    /* Also on failure, otherwise a failing notify is retried back to back
     * and the BTstack context never gives core 0 a break */
    last_send_ms = btstack_run_loop_get_time_ms();
    fill_frames(frames, input);
    if (att_server_notify(con_handle, ATT_CHARACTERISTIC_FF01_01_VALUE_HANDLE,
                          frames, sizeof(frames)) == ERROR_CODE_SUCCESS) {
        frame_counter += FRAMES_PER_NOTIFY;
        sent_input = input;
        sent_valid = true;
    }
    send_poll();
}

static void connection_closed()
{
    btstack_run_loop_remove_timer(&send_timer);
    con_handle = HCI_CON_HANDLE_INVALID;
    conn_interval = 0;
    notify_enabled = false;
    send_pending = false;
}

static uint16_t att_read(hci_con_handle_t handle, uint16_t att_handle,
                         uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == ATT_CHARACTERISTIC_FF01_01_VALUE_HANDLE) {
        uint8_t frames[FRAME_SIZE * FRAMES_PER_NOTIFY];
        fill_frames(frames, latest_input);
        return att_read_callback_handle_blob(frames, sizeof(frames), offset,
                                             buffer, buffer_size);
    }
    return 0;
}

static int att_write(hci_con_handle_t handle, uint16_t att_handle,
                     uint16_t transaction_mode, uint16_t offset,
                     uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == ATT_CHARACTERISTIC_FF01_01_CLIENT_CONFIGURATION_HANDLE) {
        notify_enabled = (little_endian_read_16(buffer, 0) ==
                          GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
        if (notify_enabled) {
            con_handle = handle;
            state = BLE_ACTIVE;
            frame_counter = 0;
            sent_valid = false;
            gap_request_connection_parameter_update(handle, CONN_INTERVAL_MIN,
                CONN_INTERVAL_MAX, CONN_LATENCY, CONN_TIMEOUT);
            send_poll();
        } else {
            btstack_run_loop_remove_timer(&send_timer);
            state = BLE_CONNECTED;
        }
    }
    /* Writes to the value itself are accepted and ignored */
    return 0;
}

static void att_handler(uint8_t packet_type, uint16_t channel,
                        uint8_t *packet, uint16_t size)
{
    if ((packet_type == HCI_EVENT_PACKET) &&
        (hci_event_packet_get_type(packet) == ATT_EVENT_CAN_SEND_NOW)) {
        send_notification();
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel,
                           uint8_t *packet, uint16_t size)
{
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                start_advertising();
            }
            break;
        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) ==
                GAP_SUBEVENT_LE_CONNECTION_COMPLETE) {
                con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                conn_interval = gap_subevent_le_connection_complete_get_conn_interval(packet);
                state = BLE_CONNECTED;
            }
            break;
        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) ==
                HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE) {
                conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (hci_event_disconnection_complete_get_connection_handle(packet) == con_handle) {
                connection_closed();
                start_advertising();
            }
            break;
        default:
            break;
    }
}

static void start_stack()
{
    static btstack_packet_callback_registration_t hci_event_callback;

    cyw43_arch_lwip_begin();

    l2cap_init();
    sm_init();
    att_server_init(profile_data, att_read, att_write);
    att_server_register_packet_handler(att_handler);

    hci_event_callback.callback = packet_handler;
    hci_add_event_handler(&hci_event_callback);

    btstack_run_loop_set_data_source_handler(&input_source, input_poll);
    btstack_run_loop_enable_data_source_callbacks(&input_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&input_source);

    state = BLE_STARTING;
    /* Loads the BT firmware right here, the rest of the bring-up is event
     * driven and goes through packet_handler() */
    hci_power_control(HCI_POWER_ON);

    cyw43_arch_lwip_end();
}

void ble_init()
{
    present = false;
    state = BLE_OFF;

    if (!flash_layout_ok() || !core1_ready()) {
        printf("Bluetooth disabled, flash layout or core 1 not ready.\n");
        state = BLE_FAILED;
        return;
    }

    if (cyw43_arch_init() != 0) {
        state = BLE_FAILED;
        return;
    }

    present = chip_responds();
    if (!present) {
        /* A failed bus init leaves its PIO state machine and DMA claimed */
        cyw43_ll_deinit(&cyw43_state.cyw43_ll);
        cyw43_arch_deinit();
        restore_wl_pins();
        return;
    }

    start_stack();
}

void ble_input(uint8_t tt, uint8_t keys1, uint8_t keys2)
{
    uint32_t value = tt | (keys1 << 8) | (keys2 << 16);

    if (value == latest_input) {
        return;
    }
    latest_input = value;

    if (notify_enabled && !poke_pending) {
        poke_pending = true;
        btstack_run_loop_poll_data_sources_from_irq();
    }
}

bool ble_hw_present()
{
    return present;
}

void ble_update()
{
    cyw43_arch_poll();
}

const char *ble_hw_name()
{
    return present ? "CYW43439" : "Not present";
}

const char *ble_state_name()
{
    switch (state) {
        case BLE_FAILED:
            return "Failed";
        case BLE_STARTING:
            return "Starting";
        case BLE_ADVERTISING:
            return "Advertising";
        case BLE_CONNECTED:
            return "Connected";
        case BLE_ACTIVE:
            return "Active";
        default:
            return "Off";
    }
}

uint32_t ble_conn_interval()
{
    return conn_interval * 125;
}
