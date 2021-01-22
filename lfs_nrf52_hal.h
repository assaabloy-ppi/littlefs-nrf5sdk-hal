#ifndef _littlefs_nrf52a_hal_H_
#define _littlefs_nrf52a_hal_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file littlefs_nrf52a_hal.h
 *
 * Description
 *
 */

/*======= Includes ==========================================================*/

#include "lfs.h"

/*======= Public macro definitions ==========================================*/


#ifndef LFS_NRF52_CACHE_SIZE
#define LFS_NRF52_CACHE_SIZE 16
#endif

/*======= Type Definitions and declarations =================================*/

typedef void (*wdt_feed)(void);

/*======= Public variable declarations ======================================*/
/*======= Public function declarations ======================================*/

/**
 * @brief Initialize the littlefs hal implementation.
 *
 * @note This function also populates the littlefs config struct which should be
 *       passed to the littlefs API.
 *
 * @param[out] c             littlefs config struct. This should be zero when
 *                           this function is called.
 * @param[in] wdt_feed_impl  Optional watchdog feed function that the API will
 *                           call when waiting for the fstorage to finish its
 *                           operation. Can be NULL.
 */
void littlefs_nrf52_init(struct lfs_config *c, wdt_feed wdt_feed_impl);

#ifdef __cplusplus
}
#endif

#endif /* _littlefs_nrf52a_hal_H_ */
