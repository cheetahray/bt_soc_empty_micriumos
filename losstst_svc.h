/**
 * @file losstst_svc.h
 * @brief Portable BLE Advertisement Update Module for Silicon Labs
 * 
 * This module provides portable BLE extended advertising functionality
 * that can be adapted from Nordic nRF52 to Silicon Labs platforms.
 */

#ifndef __losstst_svc_H__
#define __losstst_svc_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ================== Public Functions ================== */

/**
 * @brief Advertising sent callback (platform-specific)
 * 
 * This callback is invoked when an advertising packet is sent.
 * Platform implementations should call this from their event handlers.
 * 
 * @param adv_handle Advertising handle/set that sent the packet
 * @param num_sent Number of advertising packets sent
 */
//void adv_sent_callback(adv_handle_t *adv_handle, uint16_t num_sent);

/**
 * @brief Finalize sender (application-specific)
 * 
 * Application-defined function for sender finalization.
 */
void sender_finit(void);

#ifdef __cplusplus
}
#endif

#endif /* __losstst_svc_H__ */

int losstst_init(void);