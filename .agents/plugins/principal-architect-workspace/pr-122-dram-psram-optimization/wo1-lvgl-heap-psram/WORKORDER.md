---
complexity: simple
---

# WO1 — Move LVGL Widget Heap to PSRAM

## Role
You are a senior embedded C++ / ESP32 engineer. You are making a targeted configuration change to LVGL's memory allocator to reclaim 32 KB of internal DRAM.

## Parent
Project: `pr-122-dram-psram-optimization` — [PROJECT.md](../PROJECT.md)

## Objective
Move the LVGL built-in memory pool from internal DRAM (static `.bss` array) to PSRAM, reclaiming ~32 KB of scarce internal DRAM. Double the pool to 64 KB since PSRAM is abundant (8 MB). Gate on `CONFIG_SPIRAM` so non-PSRAM boards are unaffected.

## Context
- LVGL v9.5.0 supports `LV_MEM_POOL_ALLOC(size)` — a function-style macro that LVGL calls once at init to allocate the pool backing memory. When defined, LVGL's TLSF allocator uses the returned pointer instead of a static `.bss` array.
- The `LV_MEM_POOL_INCLUDE` macro specifies the header to include for the allocator function.
- DMA flush buffers (`_buf1`, `_buf2` in `LVGLDisplayModule.cpp:107-108`) are separately allocated with `MALLOC_CAP_DMA` and MUST stay in internal DRAM. This WO does NOT touch those.
- The `lv_conf.h` file is at `modules/LVGLDisplay/lv_conf.h`.

## Strict Boundary Box — do NOT touch
- Do NOT modify `LVGLDisplayModule.cpp` (DMA buffer allocation).
- Do NOT change `LV_MEM_CUSTOM` to 1 (we want to keep LVGL's TLSF allocator).
- Do NOT modify any test files — this is a runtime-only config change with no native test surface.
- Do NOT flash, tag, or open a PR.

## Line-Item Instructions

- [ ] **`modules/LVGLDisplay/lv_conf.h`** — Update the MEMORY SETTINGS section:
  ```c
  /*====================
     MEMORY SETTINGS
   *====================*/

  #if defined(CONFIG_SPIRAM) || defined(BOARD_HAS_PSRAM)
  /* PSRAM-equipped boards: allocate LVGL's widget heap pool from PSRAM.
   * This reclaims ~32 KB of internal DRAM (.bss) while giving LVGL more room.
   * DMA flush buffers remain in internal DRAM (allocated separately in LVGLDisplayModule). */
  #define LV_MEM_SIZE (64 * 1024)
  #define LV_MEM_POOL_INCLUDE <esp_heap_caps.h>
  #define LV_MEM_POOL_ALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  #else
  /* Non-PSRAM boards: keep the default internal SRAM pool */
  #define LV_MEM_SIZE (32 * 1024)
  #endif

  /* Use LVGL's built-in TLSF memory pool allocator (not a custom malloc/free) */
  #define LV_MEM_CUSTOM 0
  ```

- [ ] **`docs/lvgl-display-design.md`** — Update the DRAM budget table (line ~520) to reflect that the LVGL heap is now in PSRAM on PSRAM-equipped boards. Update the `lv_conf.h` excerpt (line ~1123) to show the new conditional config.

- [ ] **`.agents/skills/framework-expert/systems/web-portal/dram-tuning.md`** — If the LVGL heap is mentioned in the DRAM tuning guide, update to note it's now in PSRAM on equipped boards.

## Verification Requirement
- `pio run -e crowpanel-128-rotary` compiles successfully.
- `pio test -e native` — full suite green (the LVGL stubs in native tests don't exercise the real LVGL init path, so no test breakage expected).
- Confirm the `.bss` section in the build output shrinks by ~32 KB compared to the prior build (from ~60,888 B to ~28,000 B).

## Completion Report
<!-- Written by the worker at task completion -->
