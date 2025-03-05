/*
 * Controller Config Save and Load
 * WHowe <github.com/whowechina>
 * 
 * Config is stored in last sector of flash
 */

#include "save.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>


#include "bsp/board.h"
#include "pico/bootrom.h"
#include "pico/stdio.h"

#include "hardware/flash.h"
#include "pico/multicore.h"
#include "pico/unique_id.h"

#define SAVEDATA_MAX_PAGES 4

static struct {
    size_t size;
    size_t offset;
    void (*after_load)();
    void *def;
} modules[8] = {0};
static int module_num = 0;

static uint32_t savedata_size = 0;
static uint32_t pages_per_savedata = 1;
static uint32_t savedata_pkt_size = 0;

static uint32_t my_magic = 0xcafecafe;

#define SAVE_TIMEOUT_US 5000000

#define SAVE_SECTOR_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SAVE_TOTAL_PAGE_NUM (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)

typedef struct __attribute ((packed)) {
    uint32_t magic;
    uint8_t data[SAVEDATA_MAX_PAGES * FLASH_PAGE_SIZE - 4];
} savedata_t;

static savedata_t old_data = {0};
static savedata_t new_data = {0};
static int savedata_page = -1;

static bool requesting_save = false;
static uint64_t requesting_time = 0;

static mutex_t *io_lock;

static void save_program()
{
    memcpy(&old_data, &new_data, savedata_pkt_size);
    
    savedata_page++;
    if ((savedata_page < 0) ||
        ((savedata_page + 1) * pages_per_savedata >= SAVE_TOTAL_PAGE_NUM)) {
        savedata_page = 0;
    }

    int target_page_addr = savedata_page * pages_per_savedata * FLASH_PAGE_SIZE;

    if (mutex_enter_timeout_us(io_lock, 200000)) {
        sleep_ms(20); /* wait for all io operations to finish */
        uint32_t ints = save_and_disable_interrupts();
        if (savedata_page == 0) {
            flash_range_erase(SAVE_SECTOR_OFFSET, FLASH_SECTOR_SIZE);
        }
        flash_range_program(SAVE_SECTOR_OFFSET + target_page_addr,
                            (uint8_t *)&old_data, savedata_pkt_size);
        restore_interrupts(ints);
        mutex_exit(io_lock);
        printf("\nProgram Flash page %d (%8lx) done.\n", savedata_page, old_data.magic);
    } else {
        printf("Program Flash failed.\n");
    }
}

static void load_default()
{
    printf("Load Default\n");
    for (int i = 0; i < module_num; i++) {
        memcpy(new_data.data + modules[i].offset, modules[i].def, modules[i].size);
    }
    new_data.magic = my_magic;
}

static const savedata_t *get_savedata(int id)
{
    int addr = XIP_BASE + SAVE_SECTOR_OFFSET;
    return (savedata_t *)(addr + FLASH_PAGE_SIZE * pages_per_savedata * id);
}

static void save_prepare()
{
    savedata_pkt_size = 4 + savedata_size;
    pages_per_savedata = (savedata_pkt_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
}

static void save_load()
{
    for (int i = 0; i < SAVE_TOTAL_PAGE_NUM / pages_per_savedata; i++) {
        if (get_savedata(i)->magic != my_magic) {
            break;
        }
        savedata_page = i;
    }

    if (savedata_page < 0) {
        load_default();
        save_request(false);
        return;
    }

    memcpy(&old_data, get_savedata(savedata_page), savedata_pkt_size);
    memcpy(&new_data, &old_data, savedata_pkt_size);
    printf("Page Loaded %d %8lx\n", savedata_page, new_data.magic);
}

static void save_loaded()
{
    for (int i = 0; i < module_num; i++) {
        modules[i].after_load();
    }
}

static union __attribute__((packed)) {
    pico_unique_board_id_t id;
    struct {
        uint32_t id32h;
        uint32_t id32l;
    };
    uint64_t id64;
} board_id;

uint32_t board_id_32()
{
    pico_get_unique_board_id(&board_id.id);
    return board_id.id32h ^ board_id.id32l;
}

uint64_t board_id_64()
{
    pico_get_unique_board_id(&board_id.id);
    return board_id.id64;
}

void save_init(uint32_t magic, mutex_t *locker)
{
    my_magic = magic;
    io_lock = locker;
    save_prepare();
    save_load();
    save_loop();
    save_loaded();
}

void save_loop()
{
    if (requesting_save && (time_us_64() - requesting_time > SAVE_TIMEOUT_US)) {
        requesting_save = false;
        /* only when data is actually changed */
        if (memcmp(&old_data, &new_data, savedata_pkt_size) == 0) {
            return;
        }
        save_program();
    }
}

void *save_alloc(size_t size, void *def, void (*after_load)())
{
    modules[module_num].size = size;
    size_t offset = savedata_size;
    modules[module_num].offset = offset;
    modules[module_num].after_load = after_load;
    modules[module_num].def = def;
    savedata_size += size;
    module_num++;
    return new_data.data + offset;
}

void save_request(bool immediately)
{
    if (!requesting_save) {
        printf("Save requested, page %d.\n", savedata_page);
        requesting_save = true;
        new_data.magic = my_magic;
        requesting_time = time_us_64();
    }
    if (immediately) {
        requesting_time = 0;
        save_loop();
    }
}
