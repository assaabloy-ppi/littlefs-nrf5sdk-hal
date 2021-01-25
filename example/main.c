/**
 * @file Untitled-1
 *
 * Description
 *
 */

/*======= Includes ==========================================================*/

#include <stdint.h>
#include <string.h>
#include "nrf.h"
#include "nordic_common.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#else
#include "nrf_drv_clock.h"
#endif

#include "app_timer.h"
#include "app_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "lfs_nrf52_hal.h"
#include "lfs.h"

/*======= Local Macro Definitions ===========================================*/

/* A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_CONN_CFG_TAG    1

/*======= Type Definitions ==================================================*/
/*======= Local function prototypes =========================================*/
/*======= Local variable declarations =======================================*/

static lfs_t lfs;
static lfs_file_t file;
static struct lfs_config cfg;

/*======= Global function implementations ===================================*/
/*======= Local function implementations ====================================*/

#ifdef SOFTDEVICE_PRESENT
    /**@brief   Function for initializing the SoftDevice and enabling the BLE stack. */
    static void ble_stack_init(void)
    {
        ret_code_t rc;
        uint32_t ram_start;

        /* Enable the SoftDevice. */
        rc = nrf_sdh_enable_request();
        APP_ERROR_CHECK(rc);

        rc = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
        APP_ERROR_CHECK(rc);

        rc = nrf_sdh_ble_enable(&ram_start);
        APP_ERROR_CHECK(rc);
    }
#else
    static void clock_init(void)
    {
        /* Initialize the clock. */
        ret_code_t rc = nrf_drv_clock_init();
        APP_ERROR_CHECK(rc);

        nrf_drv_clock_lfclk_request(NULL);

        /* Wait for the clock to be ready. */
        while (!nrf_clock_lf_is_running())
        {
            ;
        }
    }
#endif

int main(void)
{
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    APP_ERROR_CHECK(littlefs_nrf52_init(&cfg, NULL));

#ifdef SOFTDEVICE_PRESENT
    ble_stack_init();
#else
    clock_init();
#endif


    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // read current count
    uint32_t boot_count = 0;

#ifdef LFS_NO_MALLOC
    uint8_t file_cache_buffer[LFS_NRF52_CACHE_SIZE];
    const struct lfs_file_config file_config =
    {
        .buffer = file_cache_buffer,
    };

    lfs_file_opencfg(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT, &file_config);
#else
    int ret = lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    NRF_LOG_INFO("ret: %d", ret);
#endif
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    // print the boot count
    NRF_LOG_INFO("boot_count: %d\n", boot_count);

    for (;;);
}
