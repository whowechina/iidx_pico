/*
 * Bluetooth (BLE) support
 * WHowe <github.com/whowechina>
 */

#include "bt.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef BT_SUPPORT

#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/btstack_flash_bank.h"
#include "hardware/gpio.h"

#include "btstack.h"
#include "bt_profile.h"

#include "savedata.h"
#include "usb_descriptors.h"
#include "rgb.h"

/* The name is in bt_profile.gatt, scan response only has room for a part */
#define BT_SHORT_NAME "beatmania IIDX controller"

/* How long a new host can pair after power on */
#define PAIRING_TIMEOUT_MS 120000

/* Connection interval to ask for, units of 1.25ms. iOS only goes below 15ms
 * for HID devices and no lower than 11.25ms (Apple QA1931), the host decides.
 * Latency 0 so no connection event is skipped, timeout in 10ms units. */
#define CONN_INTERVAL_MIN 9
#define CONN_INTERVAL_MAX 12
#define CONN_LATENCY 0
#define CONN_TIMEOUT 400

typedef enum {
    BT_OFF,
    BT_FAILED,
    BT_STARTING,
    BT_ADVERTISING,
    BT_PAIRING,
    BT_CONNECTED,
    BT_ACTIVE,
} bt_state_t;

static bool present = false;
static volatile bt_state_t state = BT_OFF;
static volatile bool pairing = false;
static volatile uint16_t conn_interval = 0; /* 1.25ms */

static btstack_timer_source_t pairing_timer;
static btstack_data_source_t report_source;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;

/* A 4 bytes report is one word, the main loop and the BT context can hand it
 * over without a lock. report_pending is only set while a send is requested. */
static volatile uint32_t latest_report = 0;
static uint32_t sent_report = 0;
static bool sent_valid = false;
static volatile bool report_pending = false;
static volatile bool input_enabled = false;

/* BTstack storage (two sectors) sits right below the savedata sector. Called
 * by the SDK's flash bank code, see pico_flash_bank_get_storage_offset_func */
uint32_t bt_flash_storage_offset(void)
{
    return get_savedata_offset() - PICO_FLASH_BANK_TOTAL_SIZE;
}

/* Flash sectors the firmware image must not grow into */
static bool flash_layout_ok()
{
    extern char __flash_binary_end;
    return (uintptr_t)&__flash_binary_end - XIP_BASE <= bt_flash_storage_offset();
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

static struct __attribute__((packed)) {
    uint8_t flags_len, flags_type, flags;
    uint8_t uuid_len, uuid_type;
    uint16_t uuid;
    uint8_t appearance_len, appearance_type;
    uint16_t appearance;
} adv_data = {
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x04, /* flags are set when advertising */
    3, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE,
    3, BLUETOOTH_DATA_TYPE_APPEARANCE, 0x03c4, /* HID Gamepad */
};

static const struct __attribute__((packed)) {
    uint8_t len, type;
    char name[sizeof(BT_SHORT_NAME) - 1];
} scan_rsp_data = {
    sizeof(BT_SHORT_NAME), BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME,
    BT_SHORT_NAME,
};

static void start_advertising()
{
    bd_addr_t null_addr = { 0 };

    /* only a host in pairing mode needs to see the device in its list */
    adv_data.flags = pairing ? 0x06 : 0x04;

    /* connectable undirected, 20ms, all 3 channels, no filter */
    gap_advertisements_set_params(0x20, 0x20, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)&adv_data);
    gap_scan_response_set_data(sizeof(scan_rsp_data), (uint8_t *)&scan_rsp_data);
    gap_advertisements_enable(1);
    state = pairing ? BT_PAIRING : BT_ADVERTISING;
}

static void erase_bonds()
{
    for (int i = 0; i < le_device_db_max_count(); i++) {
        le_device_db_remove(i);
    }
}

static void end_pairing()
{
    btstack_run_loop_remove_timer(&pairing_timer);
    pairing = false;
}

static void pairing_timeout(btstack_timer_source_t *timer)
{
    end_pairing();
    if (state == BT_PAIRING) {
        start_advertising();
    }
}

/* Runs in the BTstack context, the async context on core 0 */
static void report_poll(btstack_data_source_t *source,
                        btstack_data_source_callback_type_t type)
{
    if (!input_enabled || report_pending) {
        return;
    }
    if (sent_valid && (latest_report == sent_report)) {
        return;
    }

    report_pending = true;
    if (hids_device_request_can_send_now_event(con_handle) != ERROR_CODE_SUCCESS) {
        report_pending = false;
    }
}

static void send_report()
{
    /* Clear first, a report handed over after we read it triggers a new send */
    report_pending = false;

    uint32_t report = latest_report;
    hids_device_send_input_report_for_id(con_handle, REPORT_ID_JOYSTICK,
                                         (uint8_t *)&report, sizeof(report));
    sent_report = report;
    sent_valid = true;

    report_poll(NULL, DATA_SOURCE_CALLBACK_POLL);
}

static void get_report(hci_con_handle_t handle, hid_report_type_t type,
                       uint16_t id, uint16_t max_size, uint8_t *out)
{
    memset(out, 0, max_size);
    if ((type == HID_REPORT_TYPE_INPUT) && (id == REPORT_ID_JOYSTICK)) {
        uint32_t report = latest_report;
        memcpy(out, &report, btstack_min(max_size, sizeof(report)));
    }
}

static void hids_handler(uint8_t packet_type, uint16_t channel,
                         uint8_t *packet, uint16_t size)
{
    if ((packet_type != HCI_EVENT_PACKET) ||
        (hci_event_packet_get_type(packet) != HCI_EVENT_HIDS_META)) {
        return;
    }

    switch (hci_event_hids_meta_get_subevent_code(packet)) {
        case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
            input_enabled = hids_subevent_input_report_enable_get_enable(packet);
            if (input_enabled) {
                state = BT_ACTIVE;
                gap_request_connection_parameter_update(con_handle,
                    CONN_INTERVAL_MIN, CONN_INTERVAL_MAX, CONN_LATENCY, CONN_TIMEOUT);
                sent_valid = false; /* tell the host where we are */
                report_poll(NULL, DATA_SOURCE_CALLBACK_POLL);
            } else {
                state = BT_CONNECTED;
            }
            break;
        case HIDS_SUBEVENT_CAN_SEND_NOW:
            send_report();
            break;
        case HIDS_SUBEVENT_SET_REPORT:
            if ((hids_subevent_set_report_get_report_id(packet) == REPORT_ID_LIGHTS) &&
                (hids_subevent_set_report_get_report_type(packet) == HID_REPORT_TYPE_OUTPUT)) {
                rgb_set_hid_light(hids_subevent_set_report_get_report_data(packet),
                                  hids_subevent_set_report_get_report_length(packet));
            }
            break;
        default:
            break;
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
                state = BT_CONNECTED;
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
                con_handle = HCI_CON_HANDLE_INVALID;
                conn_interval = 0;
                input_enabled = false;
                report_pending = false;
                start_advertising();
            }
            break;
        case SM_EVENT_JUST_WORKS_REQUEST: {
            hci_con_handle_t handle = sm_event_just_works_request_get_handle(packet);
            if (pairing) {
                sm_just_works_confirm(handle);
            } else {
                sm_bonding_decline(handle);
            }
            break;
        }
        case SM_EVENT_PAIRING_COMPLETE:
            if (sm_event_pairing_complete_get_status(packet) == ERROR_CODE_SUCCESS) {
                end_pairing();
            }
            break;
        default:
            break;
    }
}

static void start_stack()
{
    static btstack_packet_callback_registration_t hci_event_callback;
    static btstack_packet_callback_registration_t sm_event_callback;

    cyw43_arch_lwip_begin();

    l2cap_init();
    sm_init();
    att_server_init(profile_data, NULL, NULL);

    /* Legacy Just Works pairing with bonding, a host that is not bonded is
     * turned away unless we are in pairing mode, see packet_handler() */
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    sm_set_request_security(true);
    /* Otherwise a host whose bond we erased can still encrypt with a rebuilt
     * LTK and auto-reconnect, so a pairing could never really be deleted */
    sm_allow_ltk_reconstruction_without_le_device_db_entry(0);

    hids_device_init(0, desc_hid_report_joy, desc_hid_report_joy_size);
    hids_device_register_packet_handler(hids_handler);
    hids_device_register_get_report_callback(get_report);

    hci_event_callback.callback = packet_handler;
    hci_add_event_handler(&hci_event_callback);
    sm_event_callback.callback = packet_handler;
    sm_add_event_handler(&sm_event_callback);

    btstack_run_loop_set_data_source_handler(&report_source, report_poll);
    btstack_run_loop_enable_data_source_callbacks(&report_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&report_source);

    if (pairing) {
        erase_bonds();
        btstack_run_loop_set_timer_handler(&pairing_timer, pairing_timeout);
        btstack_run_loop_set_timer(&pairing_timer, PAIRING_TIMEOUT_MS);
        btstack_run_loop_add_timer(&pairing_timer);
    }

    state = BT_STARTING;
    /* Loads the BT firmware right here, the rest of the bring-up is event
     * driven and goes through packet_handler() */
    hci_power_control(HCI_POWER_ON);

    cyw43_arch_lwip_end();
}

void bt_init(bool pairing_mode)
{
    present = false;
    state = BT_OFF;
    pairing = pairing_mode;

    if (!flash_layout_ok() || !core1_ready()) {
        printf("Bluetooth disabled, flash layout or core 1 not ready.\n");
        state = BT_FAILED;
        pairing = false;
        return;
    }

    if (cyw43_arch_init() != 0) {
        state = BT_FAILED;
        pairing = false;
        return;
    }

    present = chip_responds();
    if (!present) {
        /* A failed bus init leaves its PIO state machine and DMA claimed */
        cyw43_ll_deinit(&cyw43_state.cyw43_ll);
        cyw43_arch_deinit();
        restore_wl_pins();
        pairing = false;
        return;
    }

    start_stack();
}

void bt_hid_report(const void *report)
{
    uint32_t value;
    memcpy(&value, report, sizeof(value));

    if (value == latest_report) {
        return;
    }
    latest_report = value;

    if (input_enabled && !report_pending) {
        btstack_run_loop_poll_data_sources_from_irq();
    }
}

bool bt_pairing_active()
{
    return pairing;
}

bool bt_hw_present()
{
    return present;
}

const char *bt_hw_name()
{
    return present ? "CYW43439" : "Not present";
}

const char *bt_state_name()
{
    switch (state) {
        case BT_FAILED:
            return "Failed";
        case BT_STARTING:
            return "Starting";
        case BT_ADVERTISING:
            return "Advertising";
        case BT_PAIRING:
            return "Pairing";
        case BT_CONNECTED:
            return "Connected";
        case BT_ACTIVE:
            return "Active";
        default:
            return "Off";
    }
}

uint32_t bt_conn_interval()
{
    return conn_interval * 125;
}

#else

void bt_init(bool pairing)
{
}

void bt_hid_report(const void *report)
{
}

bool bt_pairing_active()
{
    return false;
}

bool bt_hw_present()
{
    return false;
}

const char *bt_hw_name()
{
    return "Not built";
}

const char *bt_state_name()
{
    return "Not built";
}

uint32_t bt_conn_interval()
{
    return 0;
}

#endif
