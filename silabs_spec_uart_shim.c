/**
 * @file silabs_spec_uart_shim.c
 * @brief Silicon Labs implementation of the spec_uart_* API shim.
 *
 * Behavioural mapping from Nordic/Zephyr to Silicon Labs GSDK:
 *
 *  Nordic API              │ SiLabs equivalent
 * ─────────────────────────┼──────────────────────────────────────────
 *  spec_uart_write()       │ sl_iostream_write()  – blocking, full buf
 *  spec_uart_putc()        │ sl_iostream_putchar() – non-blocking
 *  spec_uart_xmt_drain()   │ no-op (IOSTREAM write is synchronous)
 *  spec_uart_getc()        │ sl_iostream_getchar() – non-blocking (timeout=0)
 *  spec_uart_throttle()    │ sl_sleeptimer timestamp comparison
 *
 * Throttle implementation detail
 * ───────────────────────────────
 * The Nordic throttle tracks rcv_tm_stamp/xmt_tm_stamp at interrupt
 * resolution. Here we approximate with sleeptimer ticks:
 *
 *   quiet_ticks = (uint64_t)tm_us * sl_sleeptimer_get_timer_frequency()
 *                 / 1000000UL
 *
 *   throttle() returns true when:
 *     (now - last_rx_tick) >= quiet_ticks  AND
 *     (now - last_tx_tick) >= quiet_ticks
 *
 * The sleeptimer default frequency is 32768 Hz → 1 tick ≈ 30.5 µs.
 * For tm_us = 100 000 (100 ms) this gives ~3277 ticks, which is
 * accurate enough for the screen-refresh gating in ext_scr_svc_refactor.c.
 *
 * If sub-millisecond resolution is required, replace get_tick_count()
 * calls with RAIL_GetTime() (µs resolution) and adjust accordingly.
 */

#include "silabs_spec_uart_shim.h"

/* Silicon Labs GSDK headers */
#include "sl_iostream.h"
#include "sl_iostream_usart.h"
#include "sl_iostream_init_usart_instances.h"  /* sl_iostream_exp_handle etc. */
#include "sl_iostream_handles.h"   /* sl_iostream_exp_handle */
#include "sl_sleeptimer.h"

#include <string.h>

/* Forward declaration for autogen-generated instance init (not in any header). */
sl_status_t sl_iostream_usart_init_exp(void);

/* ------------------------------------------------------------------ */
/* Internal state                                                       */
/* ------------------------------------------------------------------ */

/** Tick count recorded when the last RX byte was consumed by getc(). */
static volatile uint32_t s_last_rx_tick = 0;

/** Tick count recorded at the end of the last TX operation. */
static volatile uint32_t s_last_tx_tick = 0;

/** True once spec_uart_shim_init() has run. */
static bool s_initialized = false;

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

/**
 * Convert microseconds to sleeptimer ticks.
 * Result is clamped to UINT32_MAX to avoid overflow when tm_us is large.
 */
static inline uint32_t us_to_ticks(uint32_t tm_us)
{
    uint32_t freq = sl_sleeptimer_get_timer_frequency(); /* e.g. 32768 */
    uint64_t ticks = (uint64_t)tm_us * freq / 1000000UL;
    return (ticks > UINT32_MAX) ? UINT32_MAX : (uint32_t)ticks;
}

/**
 * Record "TX just finished" timestamp.
 * Called after every successful write or putc.
 */
static inline void mark_tx_done(void)
{
    s_last_tx_tick = sl_sleeptimer_get_tick_count();
}

/**
 * Record "RX byte just received" timestamp.
 * Called by getc() whenever it successfully returns a byte.
 */
static inline void mark_rx_activity(void)
{
    s_last_rx_tick = sl_sleeptimer_get_tick_count();
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void spec_uart_shim_init(void)
{
    if (s_initialized) {
        return;
    }

    /* Do NOT call sl_iostream_usart_init_exp() here — the SDK already calls
     * sl_iostream_usart_init_instances() (both exp + vcom) via sl_service_init()
     * before osKernelStart(). Double-init corrupts the iostream state and silently
     * breaks VCOM. Just mark as initialized and let the SDK handle it. */

    /* When an RTOS kernel is present, sl_iostream_uart defaults to BLOCKING read
     * mode: sl_iostream_getchar() will wait indefinitely for a byte.  We need
     * non-blocking behaviour so spec_uart_getc() returns -1 immediately when
     * the RX buffer is empty. */
    sl_iostream_uart_set_read_block(sl_iostream_uart_exp_handle, false);

    s_last_rx_tick = 0;
    s_last_tx_tick = 0;
    s_initialized  = true;
}


int spec_uart_write(void *buf_p, size_t len)
{
    if (buf_p == NULL) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }

    /*
     * sl_iostream_write() is blocking and transfers the full buffer.
     * It returns SL_STATUS_OK on success.
     * The return type (sl_status_t) is uint32_t; 0 == SL_STATUS_OK.
     */
    sl_status_t status = sl_iostream_write(sl_iostream_exp_handle,
                                           buf_p, len);
    if (status != SL_STATUS_OK) {
        return -1;
    }

    mark_tx_done();
    return (int)len;
}


int spec_uart_putc(int ch)
{
    uint8_t byte = (uint8_t)(ch & 0xFF);

    /*
     * sl_iostream_write() with len=1.
     * If the underlying buffer is full it will block briefly, but for a
     * single byte this is negligible.
     * Return -1 on any error to match Nordic's "buffer full" contract.
     */
    sl_status_t status = sl_iostream_write(sl_iostream_exp_handle,
                                           &byte, 1);
    if (status != SL_STATUS_OK) {
        return -1;
    }

    mark_tx_done();
    return ch;
}


void spec_uart_xmt_drain(void)
{
    /*
     * sl_iostream_write() on the USART backend is synchronous: it does
     * not return until all bytes have been physically transmitted (or
     * queued in a DMA descriptor that cannot be interrupted).
     *
     * Therefore this function is a no-op / compiler barrier.
     * If you switch to an async DMA-based IOSTREAM backend you will need
     * to add a flush/wait call here.
     */
    __asm__ volatile("" ::: "memory");
}


int spec_uart_getc(void)
{
    uint8_t byte;

    /* Non-blocking: sl_iostream_uart_set_read_block(..., false) was called in
     * spec_uart_shim_init(), so sl_iostream_getchar() returns
     * SL_STATUS_WOULD_BLOCK immediately when the RX buffer is empty. */
    sl_status_t status = sl_iostream_getchar(sl_iostream_exp_handle,
                                             (char *)&byte);
    if (status != SL_STATUS_OK) {
        return -1;
    }

    mark_rx_activity();
    return (int)byte;
}


bool spec_uart_throttle(uint32_t tm_us)
{
    uint32_t quiet = us_to_ticks(tm_us);
    uint32_t now   = sl_sleeptimer_get_tick_count();

    /*
     * Unsigned subtraction handles wrap-around correctly as long as
     * (now - stamp) < UINT32_MAX/2 — i.e. the stamp is at most ~36 hours
     * in the past at 32768 Hz, which is well beyond any realistic usage.
     */
    bool rx_quiet = (now - s_last_rx_tick) >= quiet;
    bool tx_quiet = (now - s_last_tx_tick) >= quiet;

    return rx_quiet && tx_quiet;
}
