# Graveyard: WO1 Dead-Ends and Pitfalls

## ❌ First Build Attempt Failed — managed_components shutil.Error

**What happened:** The very first `pio run -e esp32s3nano` failed in the CMake configure phase with a
Python `shutil.Error` from `idf_component_manager`. Specifically, it tried to copy `espressif__esp-dsp`
from the component manager cache into `managed_components/` and found the destination paths missing.

**Error trace:**
```
shutil.Error:
[('/Users/mcneillm/Library/Caches/Espressif/ComponentManager/service_d92d8f1e/espressif__esp-dsp_1.8.0_.../modules/fir/fixed/dsps_fird_s16_aes3.S',
  '/Users/mcneillm/Documents/Projects/esp32-app-foundation/managed_components/espressif__esp-dsp/modules/fir/fixed/dsps_fird_s16_aes3.S',
  "[Errno 2] No such file or directory: '...managed_components/espressif__esp-dsp/modules/fir/fixed/dsps_fird_s16_aes3.S'")]
```

**Root cause:** Pre-existing stale state in `managed_components/` — the directory existed but was
incomplete/corrupted. NOT caused by our sdkconfig changes.

**Fix:** `rm -rf managed_components/` and retry. The component manager re-downloaded and populated the
directory cleanly on the second attempt.

**IMPORTANT for future agents:** If you see this `shutil.Error` from `idf_component_manager`, always try
wiping `managed_components/` first. Do NOT suspect the sdkconfig changes.

---

## ⚠️ MCP task diagnostic "success": false during running builds

**What happened:** The `mcp_platformio_check_task_status` tool reports `"diagnostic": {"success": false}`
even while the build is actively running and succeeding. The diagnostic is computed from a snapshot of
recent log lines and matches against known failure patterns — not against the actual process exit code.

**Implication:** Do NOT interpret `"success": false` in the diagnostic as a build failure while
`"targetStatus": "running"`. Always wait until `"targetStatus": "success"` or `"targetStatus": "error"`
before concluding.

**Confirmed behaviour:** Both builds (`esp32s3nano` 69s, `esp32dev` 71s) showed `"success": false` in
diagnostics throughout compilation, then flipped to `"success": true` only at final status.

---

## ⚠️ `custom_sdkconfig` is a single-file mechanism — cannot layer

**Why `custom_sdkconfig` was removed:** This PlatformIO key only accepts a single file path. It cannot
be made to load two files in sequence. Attempting to use it for base+delta would require either:
- Maintaining the merged file manually (the exact problem we're solving)
- Or using a pre:script to concatenate files (fragile, non-standard)

The correct approach is `board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="file1;file2"` which passes
the semicolon-list directly to CMake/ESP-IDF as the upstream mechanism intended.

---

## ⚠️ gitignored `sdkconfig.*` files must be explicitly deleted before building

**Issue:** After writing the new `.defaults` files, simply running the build was insufficient to trigger
regeneration if the old `sdkconfig.esp32s3nano` existed. ESP-IDF only regenerates the `sdkconfig.*` if
its CMake dependency tracking detects staleness — and it does NOT track `.defaults` files as CMake inputs.

**Rule:** Always `rm -f sdkconfig.esp32s3nano sdkconfig.esp32dev` after restructuring `.defaults` files.
These are gitignored and safe to delete. The build will regenerate them from the new base+delta pair.
