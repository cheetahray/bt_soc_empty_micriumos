/**
 * @file silabs_rtos_shim.h
 * @brief Zephyr kernel API → CMSIS-RTOS2 shim for Silicon Labs GSDK
 *
 * Maps the Zephyr kernel APIs used by ext_scr_svc_refactor.c to their
 * CMSIS-RTOS2 equivalents (compatible with both FreeRTOS and Micrium
 * configurations in GSDK).
 *
 * API coverage
 * ─────────────────────────────────────────────────────────────────────
 *  Threads  – k_thread_create, k_thread_name_set
 *  Queues   – K_MSGQ_DEFINE, k_msgq_put, k_msgq_get
 *  Timing   – k_sleep, k_uptime_get, k_uptime_delta
 *  Sched    – k_yield, k_can_yield
 *  Timeout  – k_timeout_t, K_MSEC, K_USEC, K_NO_WAIT, K_FOREVER
 *
 * Required GSDK components (Simplicity Studio component installer)
 * ─────────────────────────────────────────────────────────────────────
 *  - RTOS > CMSIS-RTOS2 (FreeRTOS or Micrium variant)
 *  - Services > Sleep Timer
 *
 * Build system note
 * ─────────────────────────────────────────────────────────────────────
 *  Add src/shim/ to the compiler include path so that
 *  `#include <zephyr/kernel.h>` resolves to the stub at
 *  src/shim/zephyr/kernel.h which in turn pulls in this file.
 */

#ifndef SILABS_RTOS_SHIM_H
#define SILABS_RTOS_SHIM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* CMSIS-RTOS2 (FreeRTOS or Micrium, provided by GSDK) */
#include "cmsis_os2.h"

/* Sleep timer – used for k_uptime_get() */
#include "sl_sleeptimer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Timeout type                                                         */
/* ------------------------------------------------------------------ */

typedef struct { uint32_t ticks; } k_timeout_t;

/**
 * ticks field is interpreted as **milliseconds** in K_MSEC/K_USEC and
 * passed directly to osDelay() / osMessageQueuePut/Get().
 * CMSIS-RTOS2 default tick period = 1 ms, so this is a 1:1 mapping.
 */
#define K_MSEC(ms)   ((k_timeout_t){ .ticks = (uint32_t)(ms) })
#define K_USEC(us)   ((k_timeout_t){ .ticks = (uint32_t)(((us) + 999u) / 1000u) })
#define K_NO_WAIT    ((k_timeout_t){ .ticks = 0u })
#define K_FOREVER    ((k_timeout_t){ .ticks = osWaitForever })

/* ------------------------------------------------------------------ */
/* Thread API                                                           */
/* ------------------------------------------------------------------ */

typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);
typedef osThreadId_t k_tid_t;
typedef uint8_t k_thread_stack_t;

/**
 * Stores thread fn + args for the CMSIS-RTOS2 entry trampoline.
 * The struct must remain alive for the entire lifetime of the thread
 * (typically allocated statically at file scope).
 */
struct k_thread {
    osThreadId_t     tid;
    k_thread_entry_t fn;
    void            *p1, *p2, *p3;
};

/**
 * Define a stack buffer aligned for ARM Cortex-M.
 * Expands to a static uint8_t array with 8-byte alignment.
 */
#define K_THREAD_STACK_DEFINE(name, size) \
    static uint8_t name[(size)] __attribute__((aligned(8)))

/** Size of a stack defined with K_THREAD_STACK_DEFINE(). */
#define K_THREAD_STACK_SIZEOF(name) sizeof(name)

/**
 * Trampoline: adapts the single-argument CMSIS-RTOS2 thread entry
 * to Zephyr's three-argument convention.
 */
static inline void _sl_k_thread_trampoline(void *arg)
{
    struct k_thread *t = (struct k_thread *)arg;
    t->fn(t->p1, t->p2, t->p3);
    osThreadTerminate(osThreadGetId());
}

/**
 * Create and start a new thread.
 *
 * @param new_thread  Persistent thread control block (caller owns storage).
 * @param stack       Stack buffer from K_THREAD_STACK_DEFINE().
 * @param stack_size  K_THREAD_STACK_SIZEOF(stack).
 * @param fn          Thread entry function (void fn(void*, void*, void*)).
 * @param p1,p2,p3    Arguments forwarded to fn.
 * @param prio        Zephyr priority (ignored; thread runs at osPriorityNormal).
 * @param opts        Zephyr option flags (ignored).
 * @param delay       Start delay (K_NO_WAIT or K_MSEC(n) — not supported;
 *                    thread starts immediately).
 * @return CMSIS-RTOS2 thread ID.
 */
static inline k_tid_t k_thread_create(
    struct k_thread  *new_thread,
    k_thread_stack_t *stack,
    size_t            stack_size,
    k_thread_entry_t  fn,
    void *p1, void *p2, void *p3,
    int prio, uint32_t opts,
    k_timeout_t delay)
{
    new_thread->fn = fn;
    new_thread->p1 = p1;
    new_thread->p2 = p2;
    new_thread->p3 = p3;

    const osThreadAttr_t attr = {
        .stack_mem  = stack,
        .stack_size = (uint32_t)stack_size,
        .priority   = osPriorityNormal,
    };

    new_thread->tid = osThreadNew(_sl_k_thread_trampoline, new_thread, &attr);
    (void)prio;
    (void)opts;
    (void)delay;
    return new_thread->tid;
}

/**
 * Set a debug name on a thread.
 * CMSIS-RTOS2 has no rename API — this is a no-op on SiLabs.
 */
static inline void k_thread_name_set(k_tid_t tid, const char *name)
{
    (void)tid;
    (void)name;
}

/* ------------------------------------------------------------------ */
/* Message queue                                                        */
/* ------------------------------------------------------------------ */

/**
 * Message queue control block.
 * The CMSIS-RTOS2 queue object is created lazily on first use so
 * that K_MSGQ_DEFINE() at file scope does not require a constructor.
 */
struct k_msgq {
    osMessageQueueId_t handle;   /**< Set to NULL until first use. */
    uint32_t           msg_size;
    uint32_t           max_msgs;
};

/**
 * Statically define a message queue.
 * Matches `extern struct k_msgq name` declarations in other TUs.
 *
 * @param name         Variable name.
 * @param msg_size_val Size of each message in bytes.
 * @param max_msgs_val Maximum number of messages in the queue.
 * @param align_val    Alignment hint (unused on SiLabs, kept for API compat).
 */
#define K_MSGQ_DEFINE(name, msg_size_val, max_msgs_val, align_val)   \
    struct k_msgq name = {                                            \
        .handle   = NULL,                                             \
        .msg_size = (uint32_t)(msg_size_val),                         \
        .max_msgs = (uint32_t)(max_msgs_val)                          \
    }

/**
 * Lazy initialisation: create the CMSIS-RTOS2 queue on first call.
 * Must be called from thread context (osMessageQueueNew uses heap).
 */
static inline void _sl_k_msgq_ensure_init(struct k_msgq *q)
{
    if (q->handle == NULL) {
        q->handle = osMessageQueueNew(q->max_msgs, q->msg_size, NULL);
    }
}

/**
 * Post a message to the tail of the queue.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int k_msgq_put(struct k_msgq *q,
                              const void *data, k_timeout_t timeout)
{
    _sl_k_msgq_ensure_init(q);
    uint32_t ms = (timeout.ticks == osWaitForever) ? osWaitForever
                                                   : timeout.ticks;
    osStatus_t s = osMessageQueuePut(q->handle, data, 0u, ms);
    return (s == osOK) ? 0 : -11; /* -EAGAIN */
}

/**
 * Receive a message from the head of the queue.
 *
 * @return 0 on success, negative errno if empty / timeout.
 */
static inline int k_msgq_get(struct k_msgq *q,
                              void *data, k_timeout_t timeout)
{
    _sl_k_msgq_ensure_init(q);
    uint32_t ms = (timeout.ticks == osWaitForever) ? osWaitForever
                                                   : timeout.ticks;
    osStatus_t s = osMessageQueueGet(q->handle, data, NULL, ms);
    return (s == osOK) ? 0 : -11; /* -EAGAIN */
}

/* ------------------------------------------------------------------ */
/* Timing                                                               */
/* ------------------------------------------------------------------ */

/**
 * Monotonic system uptime in milliseconds.
 * Wraps at ~49.7 days on a 32-bit tick counter at 32768 Hz.
 */
static inline int64_t k_uptime_get(void)
{
    return (int64_t)sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count());
}

/**
 * Return elapsed ms since *reftime, and update *reftime to now.
 * Equivalent to Zephyr's k_uptime_delta().
 */
static inline int64_t k_uptime_delta(int64_t *reftime)
{
    int64_t now   = k_uptime_get();
    int64_t delta = now - *reftime;
    *reftime = now;
    return delta;
}

/* ------------------------------------------------------------------ */
/* Scheduler                                                            */
/* ------------------------------------------------------------------ */

/**
 * Suspend the current thread for the given timeout.
 * K_MSEC(50) → osDelay(50) → ~50 ms (depends on RTOS tick rate).
 *
 * @return 0 (Zephyr returns remaining sleep ticks on wakeup; 0 is fine
 *         for the callers in ext_scr_svc_refactor.c).
 */
static inline int32_t k_sleep(k_timeout_t timeout)
{
    if (timeout.ticks == 0u) {
        return 0;
    }
    osDelay(timeout.ticks);
    return 0;
}

/** Yield the CPU to the next ready thread.
 *
 * IMPORTANT: osThreadYield() maps to OSSchedRoundRobinYield() which requires
 * OS_CFG_SCHED_ROUND_ROBIN_EN == DEF_ENABLED.  In this project it is disabled,
 * so calling osThreadYield() hits RTOS_ASSERT_CRITICAL and faults the kernel.
 * OSSched() is a private MicriumOS symbol not declared in any public header.
 * Use osDelay(1) instead — it yields for one tick (1 ms) and is safe
 * regardless of round-robin configuration.
 */
static inline void k_yield(void)
{
    osDelay(1);
}

/**
 * Returns true when scheduling is possible.
 * Under an RTOS the answer is always true; on bare-metal this shim
 * should not be used (ext_scr_svc_refactor.c requires an RTOS thread).
 */
static inline bool k_can_yield(void)
{
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* SILABS_RTOS_SHIM_H */
