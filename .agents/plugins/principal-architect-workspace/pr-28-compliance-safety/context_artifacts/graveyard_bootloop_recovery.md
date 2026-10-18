# Graveyard: Bootloop Recovery and Runtime Crashes

## Bootloop #1 — TWDT Panic during TLS Handshake (v0.0.10-dev.5 → dev.6)

**Symptom:** Device bootlooped at 13 seconds after flashing. Hold-boot-button recovery
failed (button held for 15 seconds — nothing happened until release, then it booted
normally and immediately bootlooped again).

**Root cause:** WO2 registered the OTA task with TWDT for its *entire lifetime*
(`esp_task_wdt_add(NULL)` at task start). The OTA check involves multiple TLS handshakes
to GitHub's API (3-15+ seconds each). The 5-second TWDT timeout fired during the handshake,
triggering `CONFIG_ESP_TASK_WDT_PANIC=y` → panic → reboot → OTA check → panic → loop.

**Fix:** Scoped TWDT registration to only the download phase:
```cpp
esp_task_wdt_add(NULL);     // Register just before downloadAndFlash()
OTAError result = engine.downloadAndFlash(...);
esp_task_wdt_delete(NULL);  // Deregister immediately after
```
Pre-download phases (API queries, manifest fetch, signature fetch) are NOT under TWDT.
The download phase feeds the watchdog via the progress callback every ~4 KB.

**Recovery:** Used `esptool.py erase_region` to wipe the `otadata` partition, which
forced the bootloader back to factory app. Required knowing the exact partition offset.
Standard boot-button hold did NOT work to interrupt the bootloop.

**Lesson:** NEVER register long-running TLS tasks with TWDT for their full lifetime.
Scope TWDT to deterministic phases only.

## Crash #2 — MCP Server `std::bad_alloc` Abort (v0.0.10-dev.6)

**Symptom:** After fixing the TWDT bootloop, device crashed with `std::bad_alloc`
during serial monitor session.

**Root cause:** The MCPServer's input buffer (`_inputBuffer`) had no size cap. When
serial garbage or rapid input flooded the buffer, `std::string::append()` attempted
unbounded allocation until heap exhaustion, triggering `std::bad_alloc` (which is
`abort()` on ESP32 since exceptions are disabled).

**Fix:** Added 8 KB cap to `_inputBuffer` in `MCPServer::processSerialInput()`:
```cpp
if (_inputBuffer.size() > 8192) {
    ESP_LOGW(TAG, "Input buffer overflow (%zu bytes) — discarding", _inputBuffer.size());
    _inputBuffer.clear();
    return;
}
```

**Lesson:** Any unbounded buffer accumulation on an embedded device WILL eventually
cause an abort. Always cap input buffers with a reasonable maximum.

## Dead End: Boot Button Recovery

Holding the BOOT button during plug-in on the ESP32-S3 NanoS3 did NOT enter
download/recovery mode reliably during a fast bootloop. The TWDT panic triggered
too quickly (~5 seconds) for the button hold to take effect. Direct flash erase
via `esptool.py` connected during the brief window between resets was the only
reliable recovery method.

## Dead End: Serial Monitor During Bootloop

PlatformIO serial monitor could not capture useful logs during the bootloop because:
1. The panic happened before serial output was fully flushed
2. The reboot cycle was faster than the monitor reconnection
3. Only fragments of the backtrace were visible

Solution: Use `esptool.py` to erase otadata, boot cleanly, then inspect the previous
panic reason via `esp_reset_reason()` on the next clean boot.
