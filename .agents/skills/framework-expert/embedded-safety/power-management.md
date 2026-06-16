# Power Management

Optimizing power consumption reduces thermal load, extends component lifespan, and is strictly required for battery-operated deployments.

## Hardware Power Draw

Typical ESP32 power consumption profiles (at 3.3V):
- **Deep Sleep:** ~10µA - 150µA (depending on RTC peripherals).
- **Modem Sleep (WiFi off, CPU on):** ~20mA - 30mA.
- **Active (WiFi TX/RX):** ~100mA - 240mA.
- **AP Mode (Access Point):** Consistently draws ~100mA-120mA as it must broadcast beacons continuously.

## Display Sleep & Wake Events

The display is often the most power-hungry component. The framework implements a [ScreensaverController](../systems/display/screensaver.md).

- **Inactivity Policy:** Displays should dim or turn off when inactive.
- **Event Hook:** Modules should publish `system.display.wake` events to the EventBus when user interaction (e.g., a button press or web portal command) occurs.

## Modem Sleep

For devices that only need to fetch data periodically (e.g., every 5 minutes):
- Do not leave WiFi in active RX mode constantly.
- Use ESP-IDF's automatic modem sleep features. When enabled, the ESP32 synchronizes with the router's DTIM beacon, turning off the RF module between beacons while maintaining the connection.

## Light Sleep Considerations

Light sleep suspends the CPU and RF, but retains RAM.
- Useful for battery operation, but breaks standard `ITickable` timing (the FreeRTOS tick count stops or jumps).
- Applications requiring light sleep must carefully manage peripherals (I2C/SPI) before sleeping and re-initialize or resume them upon waking.
