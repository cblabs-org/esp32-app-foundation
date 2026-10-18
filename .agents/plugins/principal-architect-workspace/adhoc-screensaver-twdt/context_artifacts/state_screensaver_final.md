# State: Screensaver Final Implementation

## ScreensaverController.cpp — _blank() and _wake()

```cpp
/**
 * @brief Transition to blanked state — enable hardware power-save on the display.
 */
void ScreensaverController::_blank(U8G2& display) {
    // Use hardware power-save (setPowerSave(1) = display controller sleep).
    // This is the correct approach — it deenergises the OLED segments and
    // shuts down the DC-DC boost converter, unlike clearBuffer/sendBuffer
    // which only visually blanks while the controller keeps running.
    //
    // Previously this crashed (TWDT) but the root cause was the tight main
    // loop after blanking starving IDLE1 — not the SPI command itself.
    // The vTaskDelay(33ms) in U8G2DisplayModule::tick() fixes that.
    ESP_LOGI(TAG, "Display blanked (entering power save)");
#ifndef NATIVE_BUILD
    display.setPowerSave(1);
#else
    (void)display;
#endif
    _blanked = true;
}

/**
 * @brief Wake from any screensaver state — restore brightness and deactivate the board.
 */
void ScreensaverController::_wake(U8G2& display, uint8_t activeBrightness) {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Display waking from sleep");
    display.setPowerSave(0);  // Wake from hardware sleep before restoring contrast
    display.setContrast(activeBrightness);
#endif
    _dimmed = false;
    _blanked = false;
    // ... board deactivation follows
}
```

## U8G2DisplayModule.cpp — tick() blanked path

```cpp
// If sleeping, skip carousel and rendering.
// IMPORTANT: vTaskDelay ensures loopTask yields for ~1 frame even when blanked.
// Without this, tick() returns in microseconds (no rendering), loopTask spins
// at MHz speed, starves IDLE1 on CPU 1, and the Task Watchdog fires after 5s.
if (isBlanked) {
    vTaskDelay(pdMS_TO_TICKS(_targetFrameIntervalMs));
    return;
}
```

## sdkconfig.defaults.esp32s3nano — console config

```kconfig
# USB-JTAG console (replaces CONFIG_ESP_CONSOLE_NONE from PR-19 which silenced all output)
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
```

Monitor port: `/dev/cu.usbmodem101`

## Screensaver State Machine Lifecycle (confirmed working)

```
Boot
 └─ Active (carousel, full brightness)
     └─ 150s inactivity → Dimmed
         ├─ setContrast(_dimLevel)   ← ~30/255
         ├─ ScreensaverBoard activated (bouncing clock)
         └─ 300s inactivity → Blanked
             ├─ setPowerSave(1)      ← screen truly off
             └─ Any input → Wake
                 ├─ setPowerSave(0)
                 ├─ setContrast(activeBrightness)
                 └─ ScreensaverBoard deactivated
```

## Key Config Values (from display config)

```json
{
  "screensaver": true,
  "dim": 150000,
  "blank": 300000,
  "brightness": 255
}
```
