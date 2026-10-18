# Graveyard: Firmware Baud Rate & DTR/RTS Resets

**Failed Attempt:** Trying to upload the `esp32dev` firmware at the default `921600` baud rate.
**Result:** The ESP32 stub flasher hung and crashed: `A fatal error occurred: The chip stopped responding.`
**Fix:** Modified `sample-app/platformio.ini` to explicitly set `upload_speed = 460800` for the `esp32dev` environment.

**Failed Attempt:** Triggering hardware resets using manual python DTR/RTS scripts on macOS.
**Result:** Left the chip in an unresponsive `Download Mode` with empty serial logs.
**Fix:** Authorize the use of the native `pio run -t monitor` target, which cleanly and consistently handles DTR/RTS timing to reboot the board into application mode on macOS.
