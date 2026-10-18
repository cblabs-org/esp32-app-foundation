# Graveyard: OTA Partition Locking & macOS Serial Contention

## macOS Serial Contention (`Resource Busy`)
- **Attempt**: Flashing `esp32s3nano` target natively while a background serial monitor was still attached.
- **Result**: `[Errno 35] Resource temporarily unavailable`.
- **Solution**: The PIO Manager MCP server can orphan daemon threads locking `/dev/cu.usbmodem1101`. Use `mcp_platformio_stop_monitor` to force-kill the active monitor before invoking `mcp_platformio_upload_firmware`.

## OTA Partition Stale Boot
- **Attempt**: Flashed new firmware using PIO Manager, but serial logs continued to output `v0.0.8-dev.3` instead of the injected `dev.6`.
- **Result**: Device was booting from `app1` (secondary OTA partition) because a previous OTA update had locked the `otadata` partition. PlatformIO's default `upload` writes strictly to `app0` (`0x10000`).
- **Solution**: Execute a full flash erasure using `pio run -e esp32s3nano -t erase` (which invokes `esptool.py erase_flash`). This clears the NVS and `otadata`, forcing the bootloader to revert to `app0` and allowing the newly flashed `dev.6` image to execute.

## Native CDC USB Boot Timing
- **Attempt**: Using `mcp_platformio_query_logs` to capture the first boot log of `ESP32-S3` with Native USB CDC.
- **Result**: Monitor files were empty because the ESP32-S3 disconnects the USB peripheral during a hard RTS reset, meaning the boot prints occur *before* macOS can enumerate the port and attach the monitor.
- **Solution**: Wrote a raw Python script (`read_boot_log.py`) to forcibly toggle DTR/RTS and read raw bytes synchronously, bypassing PIO's reconnection latency.
