/*
 * Hall Effect Button Reader
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hebtn.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "board_defs.h"

#include "config.h"

#define KEY_NUM HALL_KEY_NUM

#define HALL_PRESENCE_THRESHOLD 500 // about 0.4V on 3.3V reference

static bool hebtn_presence[KEY_NUM];
static bool hebtn_any_presence = false;
static uint16_t reading[KEY_NUM];
static bool key_actuated[KEY_NUM];

static void hebtn_discovery()
{
    hebtn_update();
    hebtn_any_presence = false;
    for (int i = 0; i < KEY_NUM; i++) {
        hebtn_presence[i] = (reading[i] > HALL_PRESENCE_THRESHOLD);
        hebtn_any_presence |= hebtn_presence[i];
    }
}

void hebtn_init()
{
    gpio_init(HALL_MUX_EN);
    gpio_init(HALL_MUX_A0);
    gpio_init(HALL_MUX_A1);
    gpio_init(HALL_MUX_A2);
    gpio_set_dir(HALL_MUX_EN, GPIO_OUT);
    gpio_set_dir(HALL_MUX_A0, GPIO_OUT);
    gpio_set_dir(HALL_MUX_A1, GPIO_OUT);
    gpio_set_dir(HALL_MUX_A2, GPIO_OUT);
    gpio_put(HALL_MUX_EN, 1); 
    gpio_put(HALL_MUX_A0, 0);
    gpio_put(HALL_MUX_A1, 0);
    gpio_put(HALL_MUX_A2, 0);

    adc_init();
    adc_gpio_init(26 + HALL_ADC_CHANNEL);
    adc_select_input(HALL_ADC_CHANNEL);
    gpio_pull_down(26 + HALL_ADC_CHANNEL);

    // pwm mode for lower power ripple
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    hebtn_discovery();
}

uint8_t hebtn_keynum()
{
    return KEY_NUM;
}

bool hebtn_any_present()
{
    return hebtn_any_presence;
}

bool hebtn_present(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return false;
    }
    return hebtn_presence[chn];
}

uint32_t hebtn_presence_map()
{
    uint32_t bitmap = 0;
    for (int i = 0; i < KEY_NUM; i++) {
        bitmap |= (hebtn_presence[i] << i);
    }
    return bitmap;
}

static inline void select_channel(int chn)
{
    static const uint8_t key_map[KEY_NUM] = HALL_KEY_MUX_MAP;

    uint8_t mask = key_map[chn];
    gpio_put(HALL_MUX_A0, mask & 1);
    gpio_put(HALL_MUX_A1, mask & 2);
    gpio_put(HALL_MUX_A2, mask & 4);
}

static inline uint16_t read_avg(int count)
{
    uint32_t sum = 0;
    for (int i = 0; i < count; i++) {
        sum += adc_read();
    }
    return sum / count;
}

static void read_sensor(int chn, int avg)
{
    reading[chn] = read_avg(avg);
    if (iidx_runtime.debug.sensor) {
        if (chn == 0) {
            printf("\n");
        }
        printf(" %d:%4d,", chn, reading[chn]);
    }
}

static void do_triggering()
{
    for (int i = 0; i < KEY_NUM; i++) {
        if (!hebtn_presence[i]) {
            key_actuated[i] = false;
            continue;
        }

        int travel = hebtn_travel(i);

        int on_trig = PROFILE_EX.trigger.on[i] % 36 + 1;
        on_trig = on_trig * hebtn_range(i) / 37;

        int off_trig = (35 - PROFILE_EX.trigger.off[i] % 36) + 1;
        off_trig = off_trig * on_trig / 38; // 38 for just a bit more dead zone

        key_actuated[i] = key_actuated[i] ? (travel > off_trig)
                                          : (travel >= on_trig);
    }
}

void hebtn_update()
{
    for (int i = 0; i < KEY_NUM; i++) {
        select_channel(i);
        sleep_us(5);
        read_sensor(i, 20);
    }

    do_triggering();
}

bool hebtn_actuated(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return false;
    }
    return key_actuated[chn];
}

uint32_t hebtn_actuated_map()
{
    uint32_t bitmap = 0;
    for (int i = 0; i < KEY_NUM; i++) {
        bitmap |= (key_actuated[i] << i);
    }
    return bitmap;
}

uint16_t hebtn_raw(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return 0;
    }
    return reading[chn];
}

uint16_t hebtn_range(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return 0;
    }
    return abs(iidx_cfg->hall.calibrated.down[chn] - iidx_cfg->hall.calibrated.up[chn]);
}

uint16_t hebtn_travel(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return 0;
    }

    if (!hebtn_presence[chn]) {
        return 0;
    }

    int up = iidx_cfg->hall.calibrated.up[chn];
    int down = iidx_cfg->hall.calibrated.down[chn];
    int range = down - up;
    int travel = reading[chn] - up;
    if (range < 0) {
        travel = -travel;
        range = -range;
    }

    if (travel < 8) { // extra start-up dead zone
        travel = 0;
    } else if (travel > range) {
        travel = range;
    }

    return travel;
}

uint8_t hebtn_travel_byte(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return 0;
    }

    int range = hebtn_range(chn);
    int pos = hebtn_travel(chn);
    return (range != 0) ? pos * 255 / range : 0;
}

uint8_t hebtn_trigger_byte(uint8_t chn)
{
    if (chn >= KEY_NUM) {
        return 0;
    }
    int trig = PROFILE_EX.trigger.on[chn] % 36 + 1;

    return trig * 255 / 37;
}

static void read_sensors_avg(uint16_t avg[KEY_NUM])
{
    const int avg_count = 200;
    uint32_t sum[KEY_NUM] = {0};

    for (int i = 0; i < avg_count; i++) {
        for (int j = 0; j < KEY_NUM; j++) {
            select_channel(j);
            sleep_us(5);
            read_sensor(j, 32);
            sum[j] += reading[j];
        }
    }
    for (int i = 0; i < KEY_NUM; i++) {
        avg[i] = sum[i] / avg_count;
    }
}

void hebtn_calibrate()
{
    printf("Calibrating key RELEASED...\n");

    uint16_t up_val[KEY_NUM] = {0};
    read_sensors_avg(up_val);

    printf("Calibrating key PRESSED...\n");
    printf("Please press all keys down, not necessarily simultaneously.\n");

    uint16_t min[KEY_NUM] = {0};
    uint16_t max[KEY_NUM] = {0};
    for (int i = 0; i < KEY_NUM; i++) {
        min[i] = up_val[i];
        max[i] = up_val[i];
    }
    uint64_t stop = time_us_64() + 10000000;
    while (time_us_64() < stop) {
        hebtn_update();
        for (int i = 0; i < KEY_NUM; i++) {
            int val = hebtn_raw(i);
            if (val < min[i]) {
                min[i] = val;
            }
            if (val > max[i]) {
                max[i] = val;
            }
        }
    }

    uint16_t down_val[KEY_NUM] = {0};
    bool success = true;
    for (int i = 0; i < KEY_NUM; i++) {

        int trim = (max[i] - min[i]) / 50; // 2% dead zone at two sides
        max[i] -= trim;
        min[i] += trim;

        bool max_is_down = abs(max[i] - up_val[i]) > abs(min[i] - up_val[i]);
        down_val[i] = max_is_down ? max[i] : min[i];
        up_val[i] = max_is_down ? min[i] : max[i];
        if (abs(down_val[i] - up_val[i]) < 300) {
            printf("Key %d calibration failed. [%4d->%4d].\n",
                   i + 1, up_val[i], down_val[i]);
            success = false;
        }
    }

    printf("Calibration %s.\n", success ? "succeeded" : "failed");

    if (!success) {
        return;
    }

    for (int i = 0; i < KEY_NUM; i++) {
        iidx_cfg->hall.calibrated.up[i] = up_val[i];
        iidx_cfg->hall.calibrated.down[i] = down_val[i];
        printf("Key %d: %4d -> %4d.\n", i + 1, up_val[i], down_val[i]);
    }

    config_changed();
}