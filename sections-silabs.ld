/*
 * sections-silabs.ld — Linker script fragment for Silicon Labs GSDK
 *
 * Provides the iterable section grouping for "scr_resource" objects
 * defined via TYPE_SECTION_ITERABLE() in ui_resource_code.h.
 * The start/end linker symbols are referenced by ext_scr_svc_refactor.c
 * through the TYPE_SECTION_START_EXTERN / TYPE_SECTION_END_EXTERN macros.
 *
 * HOW TO USE
 * ──────────────────────────────────────────────────────────────────────
 * 1. Open your SiLabs project's .ld linker script
 *    (e.g., autogen/linkerfile.ld or a project-specific .ld file).
 *
 * 2. Inside the Flash output section (usually named .text or .rodata),
 *    paste the block below just before the closing brace:
 *
 *        /* scr_resource iterable section (from sections-silabs.ld) *\/
 *        ._iterable_type_scr_resource_list : {
 *            _iterable_type_scr_resource_list_start = .;
 *            KEEP(*(SORT_BY_NAME(._iterable_type_scr_resource_list*)));
 *            _iterable_type_scr_resource_list_end = .;
 *        }
 *
 *    The "from" region (> FLASH / > rom) must match how your project
 *    places .rodata.  On EFR32 targets it is typically:
 *
 *        ._iterable_type_scr_resource_list : ALIGN(4) {
 *            ...
 *        } > FLASH
 *
 * 3. (Optional) If you have multiple iterable section types beyond
 *    scr_resource, add analogous blocks for each.
 *
 * WHY IT WORKS
 * ──────────────────────────────────────────────────────────────────────
 * TYPE_SECTION_ITERABLE(typ, name, scr_resource,) expands to:
 *
 *   typ __attribute__((
 *       __section__("._iterable_type_scr_resource_list.static.<name>"),
 *       used)) name = { … };
 *
 * SORT_BY_NAME() collects all ._iterable_type_scr_resource_list.* input
 * sections in lexicographic order and merges them into one contiguous
 * output section.  The start/end symbols let the C code iterate over
 * the array using pointer arithmetic — identical to what Zephyr does
 * via ITERABLE_SECTION_ROM() in sections-rom.ld.
 *
 * This mechanism is GCC-portable and requires no Zephyr-specific tools.
 */

/*
 * ── Copy this block into your project's .ld file ────────────────────
 *
 * ._iterable_type_scr_resource_list : ALIGN(4) {
 *     _iterable_type_scr_resource_list_start = .;
 *     KEEP(*(SORT_BY_NAME(._iterable_type_scr_resource_list*)));
 *     _iterable_type_scr_resource_list_end   = .;
 * } > FLASH
 *
 * ────────────────────────────────────────────────────────────────────
 *
 * Do NOT link this file directly (it is not a standalone linker
 * script).  Use it as a copy-paste reference only.
 */
