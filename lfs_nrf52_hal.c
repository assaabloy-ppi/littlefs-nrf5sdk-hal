/**
 * @file littlefs_nrf52_hal.c
 *
 * HAL implementation for littlefs for the nRF5 SDK. Uses the fstorage API for
 * low level flash operations.
 *
 */

// Copyright notice from littlefs:
/*
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */


/*======= Includes ==========================================================*/

#include "lfs_nrf52_hal.h"
#include "lfs.h"
#include "lfs_util.h"
#include "nrf_fstorage.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_fstorage_sd.h"
#else
#include "nrf_fstorage_nvmc.h"
#endif

#ifndef LFS_NO_MALLOC
#include "mem_manager.h"
#endif

/*======= Local Macro Definitions ===========================================*/

// littlefs parameters:

#ifndef LFS_NRF52_START_ADDR
#define LFS_NRF52_START_ADDR 0x3e000
#endif

#ifndef LFS_NRF52_END_ADDR
#define LFS_NRF52_END_ADDR 0x3ffff
#endif

#ifndef LFS_NRF52_READ_SIZE
#define LFS_NRF52_READ_SIZE 16
#endif

#ifndef LFS_NRF52_PROG_SIZE
#define LFS_NRF52_PROG_SIZE 16
#endif

#ifndef LFS_NRF52_PAGE_SIZE
#define LFS_NRF52_PAGE_SIZE 4096
#endif

#ifndef LFS_NRF52_PAGE_COUNT
#define LFS_NRF52_PAGE_COUNT 2
#endif

#ifndef LFS_NRF52_LOOKAHEAD_SIZE
#define LFS_NRF52_LOOKAHEAD_SIZE 16
#endif

#ifndef LFS_NRF52_BLOCK_CYCLES
#define LFS_NRF52_BLOCK_CYCLES 500
#endif

// Internal definitions

#define LFS_NRF52_ERR_WAIT_VALUE (-1)
#define N_PAGES_TO_ERASE 1

/*======= Type Definitions ==================================================*/
/*======= Local function prototypes =========================================*/

static int lfs_api_read(const struct lfs_config *c,
                        lfs_block_t block,
                        lfs_off_t off, void *buffer,
                        lfs_size_t size);

static int lfs_api_prog(const struct lfs_config *c,
                        lfs_block_t block,
                        lfs_off_t off, const void *buffer,
                        lfs_size_t size);

static int lfs_api_erase(const struct lfs_config *c, lfs_block_t block);

static int lfs_api_sync(const struct lfs_config *c);

static void fstorage_evt_handler(nrf_fstorage_evt_t *p_evt);

static int errno_to_lfs(uint32_t err);

static void wait_for_flash(void);

static void wait_for_cb(void);

/*======= Local variable declarations =======================================*/

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage_instance) =
{
    .evt_handler = fstorage_evt_handler,
    .start_addr = LFS_NRF52_START_ADDR,
    .end_addr   = LFS_NRF52_END_ADDR,
};

#ifdef LFS_NO_MALLOC
static uint8_t littlefs_read_buffer[LFS_NRF52_CACHE_SIZE];
static uint8_t littlefs_prog_buffer[LFS_NRF52_CACHE_SIZE];
static uint8_t littlefs_lookahead_buffer[LFS_NRF52_CACHE_SIZE];
#endif

static volatile int flash_op_ret = LFS_NRF52_ERR_WAIT_VALUE;

static wdt_feed hal_wdt_feed = NULL;

/*======= Global function implementations ===================================*/

void littlefs_nrf52_init(struct lfs_config *c, wdt_feed wdt_feed_impl)
{
    hal_wdt_feed = wdt_feed_impl;

    // Init nRF fstorage
#ifdef SOFTDEVICE_PRESENT
    uint32_t ret = nrf_fstorage_init(&fstorage_instance, &nrf_fstorage_sd, NULL);
#else
    uint32_t ret = nrf_fstorage_init(&fstorage_instance, &nrf_fstorage_nvmc, NULL);
#endif
    APP_ERROR_CHECK(ret);

    // Flash operations
    c->read = lfs_api_read;
    c->prog = lfs_api_prog;
    c->erase = lfs_api_erase;
    c->sync = lfs_api_sync;

    // Parameters:
    c->read_size = LFS_NRF52_READ_SIZE;
    c->prog_size = LFS_NRF52_PROG_SIZE;
    c->block_size = LFS_NRF52_PAGE_SIZE;
    c->block_count = LFS_NRF52_PAGE_COUNT;
    c->cache_size = LFS_NRF52_CACHE_SIZE;
    c->lookahead_size = LFS_NRF52_LOOKAHEAD_SIZE;
    c->block_cycles = LFS_NRF52_BLOCK_CYCLES;

#ifdef LFS_NO_MALLOC
    // Static buffers
    c->read_buffer = littlefs_read_buffer;
    c->prog_buffer = littlefs_prog_buffer;
    c->lookahead_buffer = littlefs_lookahead_buffer;
#else
    nrf_mem_init();
#endif

    // Optional parameters
#ifdef LFS_NRF52_NAME_MAX
    c->name_max = LFS_NRF52_NAME_MAX;
#endif
#ifdef LFS_NRF52_FILE_MAX
    c->file_max = LFS_NRF52_FILE_MAX;
#endif
#ifdef LFS_NRF52_ATTR_MAX
    c->attr_max = LFS_NRF52_ATTR_MAX;
#endif
#ifdef LFS_NRF52_METADATA_MAX
    c->metadata_max = LFS_NRF52_METADATA_MAX;
#endif
}

// From lfs_util.c
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size) {
    static const uint32_t rtable[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
    };

    const uint8_t *data = buffer;

    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
    }

    return crc;
}

/*======= Local function implementations ====================================*/

static int lfs_api_read(const struct lfs_config *c,
                        lfs_block_t block,
                        lfs_off_t off, void *buffer,
                        lfs_size_t size)
{
    uint32_t offset = (LFS_NRF52_START_ADDR) + (block * c->block_size) + off;
    uint32_t err = nrf_fstorage_read(&fstorage_instance, offset, buffer, size);
    if (!err)
    {
        wait_for_flash();
    }
    return errno_to_lfs(err);
}

static int lfs_api_prog(const struct lfs_config *c,
                        lfs_block_t block,
                        lfs_off_t off, const void *buffer,
                        lfs_size_t size)
{
    uint32_t offset = (LFS_NRF52_START_ADDR) + (block * c->block_size) + off;
    uint32_t err = nrf_fstorage_write(&fstorage_instance,
                                      offset,
                                      buffer,
                                      size,
                                      NULL);
    if (!err)
    {
        wait_for_flash();
        wait_for_cb();
    }
    int ret = (flash_op_ret > NRF_SUCCESS) ? (-flash_op_ret) : LFS_ERR_OK;
    flash_op_ret = LFS_NRF52_ERR_WAIT_VALUE;
    return errno_to_lfs(ret);
}

static int lfs_api_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t offset = (LFS_NRF52_START_ADDR) + (block * c->block_size);
    uint32_t err = nrf_fstorage_erase(&fstorage_instance,
                                      offset,
                                      N_PAGES_TO_ERASE,
                                      NULL);
    if (!err)
    {
        wait_for_flash();
        wait_for_cb();
    }
    int ret = (flash_op_ret != NRF_SUCCESS) ? (-flash_op_ret) : LFS_ERR_OK;
    flash_op_ret = LFS_NRF52_ERR_WAIT_VALUE;
    return errno_to_lfs(ret);
}

static int lfs_api_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

static int errno_to_lfs(uint32_t err)
{
    if (err != NRF_SUCCESS)
    {
        return -err;
    }
    return LFS_ERR_OK;
}

static void wait_for_flash(void)
{
    while (nrf_fstorage_is_busy(&fstorage_instance))
    {
        if (hal_wdt_feed != NULL)
        {
            hal_wdt_feed();
        }
    }
}

static void wait_for_cb(void)
{
    while (flash_op_ret < 0)
    {
        if (hal_wdt_feed != NULL)
        {
            hal_wdt_feed();
        }
    }
}

static void fstorage_evt_handler(nrf_fstorage_evt_t *p_evt)
{
    flash_op_ret = p_evt->result;
}
