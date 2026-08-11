# Error Handling

C++ exceptions are disabled on ESP32 (`-fno-exceptions` in sdkconfig). All error handling uses explicit return codes and diagnostic logging.

## Return Codes

Use typed return values for all fallible operations:
- `bool` for simple success/failure.
- `enum class` for operations with multiple failure modes (e.g., `UpdateStatus::Success`, `UpdateStatus::NetworkError`, `UpdateStatus::VerificationFailed`).
- Never use exceptions, `setjmp`/`longjmp`, or error codes stored in global variables.

## Diagnostic Logging

Use ESP-IDF logging macros for developer-facing diagnostics:
```cpp
ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure on heap");  // Error
ESP_LOGW(TAG, "Config schema version mismatch, migrating");    // Warning
ESP_LOGI(TAG, "OTA update check complete, no update available"); // Info
ESP_LOGD(TAG, "Tick duration: %lu µs", elapsed);               // Debug
```

Define `TAG` as a `static constexpr const char*` at the top of each `.cpp` file.

## User-Visible Errors

For errors that should be displayed to the user (on screen or web portal):
- **Display:** Use `IStatusReporter::reportError(errorMessage)` for fatal errors that should halt the device with a visible indicator. Use `reportStatus(message, progress)` with `progress < 0` for indeterminate status during recovery operations.
- **Web Portal:** Return appropriate HTTP status codes (400, 401, 404, 500) with JSON error bodies: `{"error": "description"}`.
- **MCP:** Return JSON-RPC error responses with descriptive messages.

## Cross-Module Error Notification

Use the `EventBus` to notify other modules of error conditions. Define error-specific topics in your module's namespace (e.g., `ota.error`, `wifi.disconnected`).

## Heap Allocation Failures

Always use `new (std::nothrow)` for heap allocations and check for `nullptr`:
```cpp
auto* client = new (std::nothrow) WiFiClientSecure();
if (!client) {
    ESP_LOGE(TAG, "Heap allocation failed");
    return false;
}
```
