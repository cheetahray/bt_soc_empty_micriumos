/**
 * @file silabs_nordic_compat.h
 * @brief Nordic / Zephyr platform compatibility layer for Silicon Labs GSDK
 *
 * Single master header providing all Nordic/Zephyr platform API stubs
 * needed to compile ext_scr_svc_refactor.c on Silicon Labs GSDK without
 * modifying the source file.
 *
 * Covers
 * ─────────────────────────────────────────────────────────────────────
 *  - printk     → printf alias
 *  - bt_addr_le_t type, bt_addr_le_eq(), BT_ADDR_LE_ANY
 *  - Byte-order utilities  (sys_put_le16/32, sys_get_le16/32, …)
 *  - Iterable linker-section macros (GCC-portable, same mechanism as
 *    Zephyr — works unchanged on arm-none-eabi-gcc in Simplicity Studio)
 *  - mpsl_fem stub (GET_SYM_DECL / GET_SYM_IMPL only, no direct calls)
 *  - mpsl_temperature_get() stub (call site is commented-out in source)
 *  - settings_load() stub
 *  - All RTOS primitives via silabs_rtos_shim.h
 *
 * Build system setup (Simplicity Studio)
 * ─────────────────────────────────────────────────────────────────────
 *  1. Project > Properties > C/C++ Build > Settings >
 *       GNU ARM C Compiler > Includes > Include paths
 *     Add:  "${workspace_loc:/<project>/src/shim}"
 *     This makes `#include <zephyr/kernel.h>` resolve to
 *     src/shim/zephyr/kernel.h which redirects here.
 *
 *  2. Copy or merge sections-silabs.ld into the project linker script
 *     (see sections-silabs.ld in the workspace root).
 *
 * Do NOT add this header to the src/ compiler include paths as an
 * automatic force-include; instead let the stub headers pull it in.
 */

#ifndef SILABS_NORDIC_COMPAT_H
#define SILABS_NORDIC_COMPAT_H

/* Pull in RTOS primitives first (k_thread_create, k_msgq, k_sleep, …) */
#include "silabs_rtos_shim.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Console                                                              */
/* ------------------------------------------------------------------ */

/**
 * Zephyr printk() → standard printf().
 * Works with the same format strings; output goes to whichever
 * stdout the SiLabs project has configured (RTT / SWO / VCOM).
 */
#ifndef printk
#  define printk(...)  printf(__VA_ARGS__)
#endif

/* ------------------------------------------------------------------ */
/* Byte-order utilities  (zephyr/sys/byteorder.h)                      */
/* ------------------------------------------------------------------ */

static inline void sys_put_le16(uint16_t val, uint8_t *dst)
{
    dst[0] = (uint8_t)(val);
    dst[1] = (uint8_t)(val >> 8);
}

static inline void sys_put_le32(uint32_t val, uint8_t *dst)
{
    dst[0] = (uint8_t)(val);
    dst[1] = (uint8_t)(val >> 8);
    dst[2] = (uint8_t)(val >> 16);
    dst[3] = (uint8_t)(val >> 24);
}

static inline uint16_t sys_get_le16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static inline uint32_t sys_get_le32(const uint8_t *src)
{
    return (uint32_t)src[0]
         | ((uint32_t)src[1] << 8)
         | ((uint32_t)src[2] << 16)
         | ((uint32_t)src[3] << 24);
}

static inline void sys_put_be16(uint16_t val, uint8_t *dst)
{
    dst[0] = (uint8_t)(val >> 8);
    dst[1] = (uint8_t)(val);
}

static inline uint16_t sys_get_be16(const uint8_t *src)
{
    return ((uint16_t)src[0] << 8) | (uint16_t)src[1];
}

/* ------------------------------------------------------------------ */
/* BLE address types  (zephyr/bluetooth/bluetooth.h, conn.h)           */
/* ------------------------------------------------------------------ */

/** Raw 6-byte BLE hardware address (little-endian on wire). */
struct bt_addr {
    uint8_t val[6];
};

/**
 * BLE LE address: one address type byte + six address bytes.
 * Layout matches Zephyr's struct bt_addr_le_t exactly.
 */
typedef struct {
    uint8_t        type; /**< BT_ADDR_LE_PUBLIC or BT_ADDR_LE_RANDOM */
    struct bt_addr a;
} bt_addr_le_t;

#define BT_ADDR_LE_PUBLIC  0u
#define BT_ADDR_LE_RANDOM  1u

/**
 * All-zero "any" address constant.
 * BT_ADDR_LE_ANY is a pointer, mirroring Zephyr's API where callers do
 *   party_addr = *BT_ADDR_LE_ANY;
 * or
 *   bt_addr_le_eq(&addr, BT_ADDR_LE_ANY)
 */
static const bt_addr_le_t _bt_addr_le_any_val =
    { BT_ADDR_LE_PUBLIC, { { 0u, 0u, 0u, 0u, 0u, 0u } } };
#define BT_ADDR_LE_ANY (&_bt_addr_le_any_val)

/** Byte-exact equality (type + all 6 address bytes must match). */
static inline bool bt_addr_le_eq(const bt_addr_le_t *a,
                                  const bt_addr_le_t *b)
{
    return memcmp(a, b, sizeof(bt_addr_le_t)) == 0;
}

/** Opaque forward declaration — ext_scr_svc_refactor.c uses bt_conn*
 *  only as an opaque callback argument; no members are accessed. */
struct bt_conn;

/** Forward declaration for net_buf_simple — used only in sizeof() debug prints.
 *  On SiLabs GSDK this type is not available; provide a minimal stub. */
struct net_buf_simple { uint8_t placeholder; };

/* ------------------------------------------------------------------ */
/* Settings stub  (zephyr/settings/settings.h)                         */
/* ------------------------------------------------------------------ */

/** settings_load() is called during init; no NVS available — no-op. */
static inline int settings_load(void) { return 0; }

/* ------------------------------------------------------------------ */
/* Iterable linker-section macros  (zephyr/sys/iterable_sections.h,    */
/*                                   zephyr/linker/sections.h)         */
/*                                                                      */
/* GCC-portable: identical mechanism to Zephyr's implementation.       */
/* To activate at link time, merge sections-silabs.ld into the SiLabs  */
/* project's linker script (see workspace root).                       */
/* ------------------------------------------------------------------ */

/**
 * Declare the linker-symbol pair for the start of an iterable section.
 * Must appear in each translation unit that reads section contents.
 */
#define TYPE_SECTION_START_EXTERN(typ, secname) \
    extern typ _iterable_type_##secname##_list_start[]

/**
 * Declare the linker-symbol pair for the end of an iterable section.
 */
#define TYPE_SECTION_END_EXTERN(typ, secname) \
    extern typ _iterable_type_##secname##_list_end[]

/** Pointer to the first element of the named section. */
#define STRUCT_SECTION_START(secname) \
    (_iterable_type_##secname##_list_start)

/** Pointer one-past the last element of the named section. */
#define STRUCT_SECTION_END(secname) \
    (_iterable_type_##secname##_list_end)

/**
 * Place an instance of @p typ named @p name into the @p secname section.
 *
 * The per-instance sub-section name embeds @p name so that
 * SORT_BY_NAME() in the linker script produces deterministic ordering.
 * The trailing variadic argument (Zephyr uses it for an alignment hint)
 * is consumed silently.
 *
 * Usage example (from ui_resource_code.h):
 *   TYPE_SECTION_ITERABLE(RESOURCE_xST, RLST_my_item, scr_resource,)
 */
#define TYPE_SECTION_ITERABLE(typ, name, secname, ...)            \
    typ __attribute__((                                           \
        __section__("._iterable_type_" #secname                  \
                    "_list.static." #name),                      \
        used)) name

/* ------------------------------------------------------------------ */
/* MPSL stubs  (mpsl_fem_power_model.h, mpsl_fem_protocol_api.h)       */
/*                                                                      */
/* In ext_scr_svc_refactor.c these headers are used only via the       */
/* GET_SYM_DECL / GET_SYM_IMPL macros which emit __weak__ functions    */
/* that return the address of a Kconfig symbol.  On SiLabs the symbol  */
/* is absent; the weak function returns 0, which callers treat as      */
/* "feature not present".  No direct types or prototypes are needed.   */
/* ------------------------------------------------------------------ */

/* mpsl_temp.h — mpsl_temperature_get() call is commented-out in the  */
/* source (line 3240).  Provide a stub to satisfy the include + any    */
/* future un-comment.  MPSL units are 0.25 °C; return 0 = 0 °C.       */
static inline int32_t mpsl_temperature_get(void)
{
    return 0; /* Replace with TEMPDRV or SI7021 measurement if needed */
}

/* The GET_SYM_DECL / GET_SYM_IMPL macros reference these Kconfig      */
/* symbols as C identifiers via __weak__ linkage.  They do not need a  */
/* typedef or enum — the macros just take their address.  Providing    */
/* weak zero-initialised variables satisfies the linker.               */
extern __attribute__((weak)) uint32_t CONFIG_BT_CTLR_TX_PWR_ANTENNA;
extern __attribute__((weak)) uint32_t CONFIG_MPSL_FEM_POWER_MODEL;

/* ------------------------------------------------------------------ */
/* NRF_FICR compatibility                                               */
/* nRF52 NRF_FICR->DEVICEADDR[0] returns the lower 32 bits of the     */
/* factory-assigned 48-bit device address.  On EFR32MG27 the same     */
/* value is available in DEVINFO->EUI48L.                              */
/* ------------------------------------------------------------------ */
#include "em_device.h"

typedef struct {
    volatile const uint32_t *DEVICEADDR;
} NRF_FICR_compat_t;

static const NRF_FICR_compat_t _nrf_ficr_compat = {
    .DEVICEADDR = &DEVINFO->EUI48L
};

#define NRF_FICR (&_nrf_ficr_compat)

/* ------------------------------------------------------------------ */
/* Zephyr bit-manipulation macros                                       */
/* ------------------------------------------------------------------ */

/** BIT(n): create a bitmask with bit n set (unsigned 32-bit). */
#ifndef BIT
#  define BIT(n)   (1UL << (n))
#endif

/** BIT64(n): create a bitmask with bit n set (unsigned 64-bit). */
#ifndef BIT64
#  define BIT64(n) (1ULL << (n))
#endif

/** MAX(a, b): larger of two values (safe for any scalar type). */
#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/** MIN(a, b): smaller of two values. */
#ifndef MIN
#  define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __cplusplus
}
#endif

#endif /* SILABS_NORDIC_COMPAT_H */
