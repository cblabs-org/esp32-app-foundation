# Graveyard: MCPServer Stubs

## Failed Attempt 1: Putting `millis()` in `Arduino.h`
Adding `inline unsigned long millis() { return 0; }` into `test/stubs/platform/Arduino.h` failed. Because `Arduino.h` is indirectly included by `Application.cpp` and `OTAUpdateManager.cpp`, which define their own local `millis()` mocks, it caused fatal ODR / redefinition conflicts.

## Failed Attempt 2: `#define millis esp_app_millis`
We attempted to circumvent the conflict by renaming the macro in `Arduino.h`:
```cpp
#define millis esp_app_millis
inline unsigned long esp_app_millis() { return 0; }
```
This failed because `OTAUpdateManager.cpp` had:
```cpp
unsigned long millis() __attribute__((weak));
unsigned long millis() { return 0; }
```
Which macro-expanded into a redefinition of `esp_app_millis()`.

## Failed Attempt 3: ArduinoJson `String` Conversion
The stub `String` class is not automatically convertible to `JsonVariant` by ArduinoJson because the library relies on specific type traits.
```cpp
result["wifi_ip"] = WiFi.softAPIP().toString();
```
We solved this by explicitly returning `std::string` from our `IPAddress` and `WiFi` class stubs, since `std::string` is natively supported by ArduinoJson.
