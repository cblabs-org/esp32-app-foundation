# Graveyard: Native HomeAssistant

## Failed Stub Assumptions
Attempted to run native tests without expanding the `mqtt_client.h` and `PsychicHttp.h` stubs. This caused a cascade of build failures:
- `esp_mqtt_client_config_t` missing `broker.address`, `network.timeout_ms`, etc.
- `esp_mqtt_event_t` missing `error_type` and `connect_return_code`.
- `PsychicRequest` missing `body()` method.
- `PsychicRequest::body()` initially returned `PsychicRequestBody`, but `ArduinoJson` failed to deserialize this natively. We later modified it to return `std::string` which is natively supported by `ArduinoJson`.
- `Arduino.h` required an empty `#define PROGMEM` macro to prevent test failures with `homeassistant_js.hpp`.

## Failed Test Assumptions
`test_native/test_homeassistant` failed because the test suite hardcoded `"native_mac"` for the native MAC address, whereas the module natively outputs a formatted mac `aabbccddeeff` after the `NATIVE_BUILD` guards were removed. The tests in `test_native/test_homeassistant/test_main.cpp` were updated to expect `"aabbccddeeff"`.
