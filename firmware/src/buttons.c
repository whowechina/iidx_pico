/*
 * Controller Buttons
 * WHowe <github.com/whowechina>
 * 
 * A button consists of a switch and an LED
 */

#include "buttons.h"

#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#include "rgb.h"
#include "config.h"
#include "board_defs.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const struct button {
    int8_t sw_gpio;
    int8_t led_gpio; /* led related to the button */
} BUTTON_DEFS[] = BUTTON_DEF;

#define BUTTON_NUM (ARRAY_SIZE(BUTTON_DEFS))

#define AUX_1 (BUTTON_NUM - 2)
#define AUX_2 (BUTTON_NUM - 1)

static bool sw_val[BUTTON_NUM]; /* true if pressed */
static uint64_t sw_freeze_time[BUTTON_NUM];

typedef struct {
    uint8_t led_level;
    uint8_t rgb_level;
    uint8_t up_speed;
    uint8_t down_speed;
} button_cfg_t;

static button_cfg_t *cfg;

#define LIMIT_MAX(a, max, def) { if (a > max) a = def; }

static void cfg_loaded()
{
    LIMIT_MAX(cfg->led_level, 8, 8);
    LIMIT_MAX(cfg->rgb_level, 8, 8);
    LIMIT_MAX(cfg->up_speed, 2, 0);
    LIMIT_MAX(cfg->down_speed, 3, 0);
    rgb_set_level(cfg->rgb_level);
    config_request_save();
}

static void init_cfg()
{
    button_cfg_t def = {8, 8, 0, 0};
    cfg = (button_cfg_t *)config_alloc(sizeof(def), &def, cfg_loaded);
}

static uint16_t alpha[256];
static void init_alpha()
{
    for (int i = 0; i < 256; i++) {
        alpha[i] = (i + 1) * (i + 1) - 1;
    }
}

static void init_switch()
{
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        sw_val[i] = false;
        sw_freeze_time[i] = 0;
        int8_t gpio = BUTTON_DEFS[i].sw_gpio;
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(BUTTON_DEFS[i].sw_gpio, GPIO_IN);
        gpio_pull_up(BUTTON_DEFS[i].sw_gpio);
    }
}

static void init_led()
{
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        int8_t gpio = BUTTON_DEFS[i].led_gpio; 
        if (gpio >= 0) {
            gpio_set_function(gpio, GPIO_FUNC_PWM);
            uint slice = pwm_gpio_to_slice_num(gpio);
            pwm_config cfg = pwm_get_default_config();
            pwm_config_set_clkdiv(&cfg, 4.f);
            pwm_init(slice, &cfg, true); 
        }
    }
}

void button_init()
{
    init_cfg();
    init_alpha();
    init_switch();
    init_led();
}

uint8_t button_num()
{
    return BUTTON_NUM;
}

uint8_t button_gpio(uint8_t id)
{
    return BUTTON_DEFS[id].sw_gpio;
}

typedef enum {
    NORMAL,
    SET_LED_LEVEL,
    SET_RGB_LEVEL,
    SET_FADING_LEVEL,
} mode_t;

mode_t run_mode = NORMAL;

/* If a switch flips, it freezes for a while */
#define DEBOUNCE_FREEZE_TIME_US 1000
uint16_t button_read()
{
    uint64_t now = time_us_64();
    uint16_t buttons = 0;

    for (int i = BUTTON_NUM - 1; i >= 0; i--) {
        bool sw_pressed = !gpio_get(BUTTON_DEFS[i].sw_gpio);
        
        if (now >= sw_freeze_time[i]) {
            if (sw_pressed != sw_val[i]) {
                sw_val[i] = sw_pressed;
                sw_freeze_time[i] = now + DEBOUNCE_FREEZE_TIME_US;
            }
        }

        buttons <<= 1;
        if (sw_val[i]) {
            buttons |= 1;
        }
    }

    /* Hide 9 main button signals while in setting mode */
    return run_mode == NORMAL ? buttons : (buttons & 0xFE00);
}

#define HID_EXPIRE_DURATION 1000000ULL
static uint32_t hid_expire_time = 0;
static bool hid_lights[BUTTON_NUM];

static bool led_on[BUTTON_NUM];
static uint8_t led_pwm[BUTTON_NUM];

static const uint8_t level_val[9] = {0, 33, 77, 117, 150, 180, 205, 230, 255};
static const uint8_t up_cycles[4] = {1, 20, 40, 60};
static const uint8_t down_cycles[5] = {1, 32, 64, 96, 128};

static void drive_led_pwm()
{
    for (int i = 0; i < BUTTON_NUM; i++) {
        if (BUTTON_DEFS[i].led_gpio >= 0) {
            pwm_set_gpio_level(BUTTON_DEFS[i].led_gpio, alpha[led_pwm[i]]);
        }
    }
}

static void run_led_fade()
{
    static uint32_t up_cnt = 0;
    static uint32_t down_cnt = 0;
    uint32_t up_cycle = up_cycles[cfg->up_speed] * (9 - cfg->led_level);
    uint32_t down_cycle = down_cycles[cfg->down_speed] * (9 - cfg->led_level);
    up_cnt = (up_cnt + 1) % up_cycle;
    down_cnt = (down_cnt + 1) % down_cycle;

    for (int i = 0; i < BUTTON_NUM; i++) {
        uint8_t target = led_on[i] ? level_val[cfg->led_level] : 0;
        if ((target > led_pwm[i]) && (up_cnt == 0)) {
            led_pwm[i]++;
        } else if ((target < led_pwm[i]) && (down_cnt == 0)) {
            led_pwm[i]--;
        }

        /* quickly limit led level */
        if (led_pwm[i] > level_val[cfg->led_level]) {
            led_pwm[i] = level_val[cfg->led_level];
        }
    }
}

static void led_demo(bool reset)
{
    static uint32_t loop = 0;
    if (reset) {
        loop = 0;
        return;
    }

    loop = (loop + 1) % 0x24000;

    bool is_on = (loop < 0x12000);
    for (int i = 0; i < 9; i++) {
        led_on[i] = is_on;
    }
}

static void led_indicate(uint8_t id)
{
    static uint32_t loop = 0;
    loop = (loop + 1) % 0x2000;
    led_pwm[id] = (loop & 0x1000) > 0 ? 200 : 0;
}

static void update_mode()
{
    mode_t new_mode;
    if (sw_val[AUX_1] && !sw_val[AUX_2]) {
        new_mode = SET_LED_LEVEL;
    } else if (!sw_val[AUX_1] && sw_val[AUX_2]) {
        new_mode = SET_RGB_LEVEL;
    } else if (sw_val[AUX_1] && sw_val[AUX_2]) {
        new_mode = SET_FADING_LEVEL;
    } else {
        new_mode = NORMAL;
    }
    if (new_mode != run_mode) {
        run_mode = new_mode;
        led_demo(true);
    }
}

static void run_normal()
{
    bool hid_active = (time_us_64() < hid_expire_time);
    for (int i = 0; i < BUTTON_NUM; i++) {
        led_on[i] = hid_active ? hid_lights[i] : sw_val[i];
    }
}

static void read_value(uint8_t *value)
{
    for (int i = 0; i < 9; i++) {
        if (sw_val[i]) {
            *value = i;
            config_request_save();
            led_demo(true);
            break;
        }
    }
}

static void run_led_setup()
{
    read_value(&cfg->led_level);
    led_demo(false);
    led_indicate(cfg->led_level);
}

static void run_rgb_setup()
{
    read_value(&cfg->rgb_level);
    rgb_set_level(cfg->rgb_level);
    led_indicate(cfg->rgb_level);
}

static void get_fade_value()
{
    for (int i = 0; i < 9; i++) {
        if (sw_val[i]) {
            if (i % 2 == 0) {
                cfg->down_speed = i / 2;
            } else {
                cfg->up_speed = (i - 1) / 2;
            }
            config_request_save();
            led_demo(true);
            return;
        }
    }
}

static void run_fading_setup()
{
    get_fade_value();
    led_demo(false);
    led_indicate(cfg->up_speed * 2 + 1);
    led_indicate(cfg->down_speed * 2);
}

static void proc_mode()
{
    switch(run_mode) {
        case NORMAL:
            run_normal();
            break;
        case SET_LED_LEVEL:
            run_led_setup();
            break;
        case SET_RGB_LEVEL:
            run_rgb_setup();
            break;
        case SET_FADING_LEVEL:
            run_fading_setup();
            break;
    }
}

void button_update()
{
    run_led_fade();
    update_mode();
    proc_mode();
    drive_led_pwm();
}

void button_set_hid_light(uint8_t const *lights, uint8_t num)
{
    for (int i = 0; (i < num) && (i < BUTTON_NUM); i++) {
        hid_lights[i] = (lights[i] > 0);
    }
    hid_expire_time = time_us_64() + HID_EXPIRE_DURATION;
}
