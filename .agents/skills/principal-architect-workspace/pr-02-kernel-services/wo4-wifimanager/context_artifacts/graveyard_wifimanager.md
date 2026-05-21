# Graveyard: WiFi Manager

## Blocking Reconnect Loop
An early design iteration attempted to loop synchronously until connected, which blocked the main thread and caused the watchdog timer (WDT) to trigger and reset the device.
- **Lesson Learned**: Always implement the Wi-Fi connection logic using non-blocking asynchronous state checks or delegate the reconnect loop to an independent FreeRTOS task.
