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
/*======= Type Definitions and declarations =================================*/

typedef void (*wdt_feed)(void);

/*======= Public variable declarations ======================================*/
/*======= Public function declarations ======================================*/

/**
 * @brief Initialize the littlefs hal implementation.
 *
 * This function populates the low level flash operation function pointers of
 * the littlefs config struct.
 *
 * @param[out] c             littlefs config struct.
 * @param[in] wdt_feed_impl  Optional watchdog feed function that the API will
 *                           call when waiting for the fstorage to finish its
 *                           operation. Can be NULL.
 *
 * @retval  NRF_SUCCESS    No errors were encountered
 * @retval  NRF_ERROR_XYZ  Some error was encountered when initalizing the hal
 *                         implementation.
 */
uint32_t littlefs_nrf52_init(struct lfs_config *c, wdt_feed wdt_feed_impl);

#ifdef __cplusplus
}
#endif

#endif /* _littlefs_nrf52a_hal_H_ */
