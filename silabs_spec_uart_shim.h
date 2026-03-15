/**
 * @file silabs_spec_uart_shim.h
 * @brief Silicon Labs shim for spec_uart_* API
 *
 * Provides the same function signatures as int_spec_uart_svc.h so that
 * ext_scr_svc_refactor.c can be compiled without modification on the
 * Silicon Labs platform.
 *
 * Implementation uses:
 *   - sl_iostream_exp_handle for TX and RX (caller pre-initialises the stream)
 *   - sl_sleeptimer for the throttle timing reference
 *
 * Prerequisites (Simplicity Studio component installer):
 *   - Services > IO Stream > IO Stream: USART  (instance name: exp)
 *   - Services > Sleep Timer
 *
 * Call spec_uart_shim_init() once after the RTOS starts to open the
 * exp iostream instance and configure non-blocking reads.
 */

#ifndef SILABS_SPEC_UART_SHIM_H
#define SILABS_SPEC_UART_SHIM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise the spec_uart shim.
 *
 * Opens the "exp" IO Stream USART instance and configures non-blocking
 * reads (timeout = 0) so spec_uart_getc() returns -1 immediately when
 * the RX buffer is empty.
 *
 * Must be called once after osKernelStart() / app init, before the
 * first spec_uart_* call.
 */
void spec_uart_shim_init(void);

/* ------------------------------------------------------------------ */
/* Public API — identical signatures to int_spec_uart_svc.h           */
/* ------------------------------------------------------------------ */

/**
 * @brief Write a buffer to the UART.
 *
 * Blocking: waits until all bytes have been handed to IOSTREAM.
 * Matches the Nordic ring-buffer implementation which yields and retries
 * until all bytes are queued.
 *
 * @param buf_p  Source buffer (must not be NULL).
 * @param len    Number of bytes to transmit.
 * @return       Number of bytes written, or -1 on error.
 */
int spec_uart_write(void *buf_p, size_t len);

/**
 * @brief Write a single character to the UART.
 *
 * Non-blocking attempt: returns -1 if the internal IOSTREAM TX is full.
 * Matches Nordic's putc which returns -1 if the TX ring buffer is full.
 *
 * @param ch  Character to send (cast from int, only low byte used).
 * @return    ch on success, -1 on failure.
 */
int spec_uart_putc(int ch);

/**
 * @brief Block until all pending TX data has been transmitted.
 *
 * Because sl_iostream_write() is synchronous on the USART IOSTREAM
 * implementation, this function is effectively a no-op / memory barrier.
 * Kept for API compatibility.
 */
void spec_uart_xmt_drain(void);

/**
 * @brief Non-blocking receive of one character.
 *
 * @return  Received byte (0–255) or -1 if the RX buffer is empty.
 */
int spec_uart_getc(void);

/**
 * @brief UART "quiet-line" detector.
 *
 * Returns true when the UART has had no RX activity AND no TX activity
 * for at least @p tm_us microseconds.  Used by ext_scr_svc_refactor.c
 * for cooperative multitasking and screen-refresh gating.
 *
 * Silicon Labs mapping:
 *   - RX activity: updated in spec_uart_getc() each time a byte is read.
 *   - TX activity: updated at the end of spec_uart_write() / spec_uart_putc().
 *   - Time reference: sl_sleeptimer_get_tick_count() with frequency lookup.
 *
 * @param tm_us  Silence window in microseconds.
 * @return       true if the line has been idle for >= tm_us, false otherwise.
 */
bool spec_uart_throttle(uint32_t tm_us);

#ifdef __cplusplus
}
#endif

#endif /* SILABS_SPEC_UART_SHIM_H */
