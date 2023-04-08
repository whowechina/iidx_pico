/*
 * Controller Config Save and Load
 * WHowe <github.com/whowechina>
 * 
 * Config is stored in last sector of flash
 */

#include "config.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>


#include "bsp/board.h"
#include "pico/bootrom.h"
#include "pico/stdio.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

static struct {
    size_t size;
    size_t offset;
    void (*after_load)();
} modules[8] = {0};
static int module_num = 0;

#define CONFIG_PAGE_MAGIC 0x13424321
#define CONFIG_SECTOR_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct __attribute ((packed)) {
    uint32_t magic;
    uint8_t data[FLASH_PAGE_SIZE - 4];
} page_t;

static page_t old_cfg = {0};
static page_t new_cfg = {0};
static page_t default_cfg = {0};
static int cfg_page = -1;

static bool requesting_save = false;
static uint64_t requesting_time = 0;

static io_locker_func io_lock;

static void config_save()
{
    old_cfg = new_cfg;

    cfg_page = (cfg_page + 1) % (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE);
    printf("Program Flash %d %8lx\n", cfg_page, old_cfg.magic);
    io_lock(true);
    uint32_t ints = save_and_disable_interrupts();
    if (cfg_page == 0) {
        flash_range_erase(CONFIG_SECTOR_OFFSET, FLASH_SECTOR_SIZE);
    }
    flash_range_program(CONFIG_SECTOR_OFFSET + cfg_page * FLASH_PAGE_SIZE,
                        (uint8_t *)&old_cfg, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
    io_lock(false);
}

static void load_default()
{
    printf("Load Default\n");
    new_cfg = default_cfg;
    new_cfg.magic = CONFIG_PAGE_MAGIC;
}

static const page_t *get_page(int id)
{
    int addr = XIP_BASE + CONFIG_SECTOR_OFFSET;
    return (page_t *)(addr + FLASH_PAGE_SIZE * id);
}

static void config_load()
{
    for (int i = 0; i < FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE; i++) {
        if (get_page(i)->magic != CONFIG_PAGE_MAGIC) {
            break;
        }
        cfg_page = i;
    }

    if (cfg_page < 0) {
        load_default();
        config_request_save();
        return;
    }

    old_cfg = *get_page(cfg_page);
    new_cfg = old_cfg;
    printf("Page Loaded %d %8lx\n", cfg_page, new_cfg.magic);
}

static void config_loaded()
{
    for (int i = 0; i < module_num; i++) {
        modules[i].after_load();
    }
}

void config_init(io_locker_func locker)
{
    io_lock = locker;
    config_load();
    config_loop();
    config_loaded();
}

void config_loop()
{
    if ((requesting_save) && (time_us_64() - requesting_time > 1000000)) {
        requesting_save = false;
        /* only when data is actually changed */
        for (int i = 0; i < sizeof(old_cfg); i++) {
            if (((uint8_t *)&old_cfg)[i] != ((uint8_t *)&new_cfg)[i]) {
                config_save();
                return;
            }
        }
    }
}

void *config_alloc(size_t size, void *def, void (*after_load)())
{
    modules[module_num].size = size;
    size_t offset = module_num > 0 ? modules[module_num - 1].offset + size : 0;
    modules[module_num].offset = offset;
    modules[module_num].after_load = after_load;
    module_num++;
    memcpy(default_cfg.data + offset, def, size); // backup the default
    return new_cfg.data + offset;
}

void config_request_save()
{
    requesting_time = time_us_64();
    if (!requesting_save) {
        requesting_save = true;
        new_cfg.magic = CONFIG_PAGE_MAGIC;
    }
}
